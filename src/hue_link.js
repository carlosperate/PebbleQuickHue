/*******************************************************************************
* PebbleKit JS (runs on phone) code to trigger Hue light operations.
*
* Copyright (c) 2015 carlosperate https://github.com/carlosperate/
* Licensed under The MIT License (MIT), a copy can be found in the LICENSE file.
*******************************************************************************/

// These variables can be hardcoded by the developer for quicker startup. If so
// keep in mind that new data from the settings will be ignored on app restart
var OPTIONS = {
    "HUE_BRIDGE_IP": "",
    "HUE_BRIDGE_USER": "",
    "HUE_LIGHT_ID": 0      // Conveniently, there is no ID 0 in the Hue system
};

// URL of the config page, used when the user clicks the "Configure" button
// in the Pebble app. For dev it can be changed to a local  network URL.
const CONFIG_URL = "https://carlosperate.github.io/PebbleQuickHue/config/index.html";


/*******************************************************************************
* PebbleKit JS functions
*******************************************************************************/
/** Retrieve the bridge data and toggle the light immediately. */
Pebble.addEventListener("ready", function(e) {
    toggleLightState();
});


/*******************************************************************************
* PebbleKit App Configuration functions
*******************************************************************************/
Pebble.addEventListener("showConfiguration", function(e) {
    // On a fresh JS runtime (e.g., after phone reboot or JS being reclaimed) the
    // in-memory OPTIONS is empty and the form would open blank even though the
    // watch still has the settings persisted. Pull them back from the watch before
    // opening the URL so the form is pre-filled with the saved values.
    loadSavedSettings(function() {
        // Run N-UPnP discovery on every config open and pass the result as a separate
        // param. The saved IP (if any) still populates the form; the detected IP is only
        // applied when the user clicks the "Detect Bridge IP" button in the config page.
        // Must be done here (phone-side JS) because the discovery endpoint's CORS policy
        // blocks all browser origins.
        discoverBridgeIp(function(detectedIp) {
            const params = {
                "HUE_BRIDGE_IP":   OPTIONS.HUE_BRIDGE_IP,
                "HUE_BRIDGE_USER": OPTIONS.HUE_BRIDGE_USER,
                "HUE_LIGHT_ID":    OPTIONS.HUE_LIGHT_ID,
                "DETECTED_IP":     detectedIp || ""
            };
            const fullUrl = CONFIG_URL + "?" + encodeURIComponent(JSON.stringify(params));
            console.log("Opening URL: " + fullUrl);
            Pebble.openURL(fullUrl);
        });
    });
});

/**
 * Ensures OPTIONS is populated with the watch's persisted settings before the
 * callback fires. Requests them via KEY_SETT_REQUEST and polls until the
 * appmessage listener fills OPTIONS, or falls through after a 3s timeout.
 */
function loadSavedSettings(callback) {
    if (areSettingSet()) {
        callback();
        return;
    }
    var finished = false;
    var done = function() {
        if (finished) return;
        finished = true;
        clearInterval(poll);
        callback();
    };
    var poll = setInterval(function() {
        if (areSettingSet()) done();
    }, 200);
    setTimeout(done, 3000);
    // Reset so prior exhausted retries (from the ready-handler toggle) don't
    // block us from asking again here.
    requestBridgeAttemps = 0;
    messageRequestBridgeData(null, null);
}

/** Queries Signify's discovery service and returns the first bridge's local IP. */
function discoverBridgeIp(callback) {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "https://discovery.meethue.com/", true);
    xhr.timeout = 5000;
    var finished = false;
    var done = function(ip) {
        if (finished) return;
        finished = true;
        callback(ip);
    };
    xhr.onload = function() {
        if (xhr.status === 200) {
            try {
                var bridges = JSON.parse(xhr.responseText);
                if (bridges && bridges.length > 0 && bridges[0].internalipaddress) {
                    console.log("Discovered Hue Bridge at " + bridges[0].internalipaddress);
                    done(bridges[0].internalipaddress);
                    return;
                }
                console.log("Discovery returned no bridges.");
            } catch (err) {
                console.log("Discovery parse error: " + err);
            }
        } else {
            console.log("Discovery HTTP " + xhr.status);
        }
        done(null);
    };
    xhr.onerror = function() {
        console.log("Discovery network error");
        done(null);
    };
    xhr.ontimeout = function() {
        console.log("Discovery timed out");
        done(null);
    };
    xhr.send();
}

Pebble.addEventListener("webviewclosed", function(e) {
    var setHueIp = null;
    var setHueUser = null;
    var setHueLightId = null;
    var bridgeConfig = JSON.parse(decodeURIComponent(e.response));
    for (var i=0; i < bridgeConfig.length; i++) {
        if (bridgeConfig[i].name === "HUE_BRIDGE_IP") {
            setHueIp = bridgeConfig[i].value;
            OPTIONS.HUE_BRIDGE_IP = setHueIp;
        } else if (bridgeConfig[i].name === "HUE_BRIDGE_USER") {
            setHueUser = bridgeConfig[i].value;
            OPTIONS.HUE_BRIDGE_USER = setHueUser;
        } else if (bridgeConfig[i].name === "HUE_LIGHT_ID") {
            setHueLightId = parseInt(bridgeConfig[i].value);
            OPTIONS.HUE_LIGHT_ID = setHueLightId;
        } else {
            console.log("Unrecognised Setting name: " + bridgeConfig[i].name);
        }
    }
    messageSetBridgeData(setHueIp, setHueUser, setHueLightId);
});


/*******************************************************************************
* AppMessage functions
*******************************************************************************/
/** Listener for AppMessage received. */
Pebble.addEventListener("appmessage", function(e) {
    //console.log("AppMessage received! " + JSON.stringify(e.payload));
    for (var key in e.payload){
        if (key == "KEY_LIGHT_STATE") {
            toggleLightState();
        } else if (key == "KEY_BRIGHTNESS") {
            setLightBrightness(e.payload.KEY_BRIGHTNESS);
        } else if (key == "KEY_BRIDGE_IP") {
            OPTIONS.HUE_BRIDGE_IP = e.payload.KEY_BRIDGE_IP;
        } else if (key == "KEY_BRIDGE_USER") {
            OPTIONS.HUE_BRIDGE_USER = e.payload.KEY_BRIDGE_USER;
        } else if (key == "KEY_LIGHT_ID") {
            OPTIONS.HUE_LIGHT_ID = e.payload.KEY_LIGHT_ID;
        } else {
            console.log("Unrecognised AppMessage key received in JS: " + key);
        }
    }
});

/** Sends and AppMessage with the ON/OFF state of the light. */
var sendStateAttemps = 0; 
function messageSendLightState(on_state) {
    // No boolean type defined, so need to send a 0/1 value
    var state = -1;
    if (on_state === true) {
        state = 1;  
    } else if (on_state === false) {
        state = 0;
    }

    // Assemble dictionary
    var dictionary = { "KEY_LIGHT_STATE": state };
    // Send the message, if an error occurs try again up to 3 times
    Pebble.sendAppMessage(dictionary,
            function(e) { sendStateAttemps = 0; },
            function(e) {
                reAttemp(e, sendStateAttemps, messageSendLightState);
            });

    // Now that the ON/OFF data is on its way to the pebble, request the
    // current brightness as well so that it can be displayed too
    if (on_state === true) requestLightBrightness();
}

/** Sends and AppMessage with the ON/OFF state of the light. */
var sendBrightnessAttemps = 0;
function messageSendLightBrightness(level) {
    var dictionary = { "KEY_BRIGHTNESS": level };
    // Send the message, if an error occurs try again up to 3 times
    Pebble.sendAppMessage(dictionary,
            function(e) { sendBrightnessAttemps = 0; },
            function(e) {
                reAttemp(e, sendBrightnessAttemps, messageSendLightBrightness);
            });
}

/** Send the new Hue Bridge IP and Username to the pebble for app storage */
function messageSetBridgeData(ip, user, lightId) {
    // Skip empty/falsy values so a partially-filled form save doesn't
    // overwrite a previously-good stored setting with a blank one (e.g.,
    // saving before the Register QuickHue flow has filled in the username).
    var dictionary = {};
    if (ip)      dictionary["KEY_BRIDGE_IP"]   = ip;
    if (user)    dictionary["KEY_BRIDGE_USER"] = user;
    if (lightId) dictionary["KEY_LIGHT_ID"]    = lightId;
    if (Object.keys(dictionary).length === 0) return;

    // Retry up to 3 times on nack. Uses a local counter + closure so the
    // original ip/user/lightId are resent on retry (the shared reAttemp
    // helper calls its callback with no args, which would send undefined
    // values and wipe the stored settings).
    var attempts = 0;
    var send = function() {
        Pebble.sendAppMessage(dictionary,
            function(e) { /* delivered */ },
            function(e) {
                attempts++;
                console.log("Set bridge data failed (attempt " + attempts +
                            "): " + e.error.message);
                if (attempts < 3) send();
            });
    };
    send();
}

/**
 * Request the Hue Bridge IP and Username from the pebble app storage.
 * If an additional request and value are provided it also sends those to be
 * resent back to the PebbleKit JS to re-try the operation.
 */
var requestBridgeAttemps = 0;
function messageRequestBridgeData(additionaRequest, additionalValue) {
    // We only do 3 attemps triggered by function calls rather than nacks
    if (requestBridgeAttemps < 3) {
        var dictionary = { "KEY_SETT_REQUEST": 0 };  
        if (additionaRequest !== null) {
            dictionary[additionaRequest] = additionalValue;
        }
        Pebble.sendAppMessage(dictionary);
        requestBridgeAttemps++;
    }
}

function reAttemp(e, tracker, callback) {
    console.log("Unable to deliver message Id=" + e.data.transactionId +
                ", attempt " + tracker + ", Error is: " + e.error.message);
    tracker++;
    if (tracker < 3) callback();
}


/*******************************************************************************
* Hue control functions
*******************************************************************************/
function toggleLightState() {
    if (!areSettingSet()) {
        messageRequestBridgeData("KEY_LIGHT_STATE", 0);
        return;
    }
    const toggleCallback = function(jsonStrDataBack) {
        if (!jsonStrDataBack) return;
        const parsedJson = JSON.parse(jsonStrDataBack);
        if (parsedJson.state && (parsedJson.state.on !== undefined)) {
            setLightState(!parsedJson.state.on);
        } else {
            messageSendLightState(-1);
            console.log("Error in getting light state: " +  jsonStrDataBack);
        }
    };
    ajaxRequest(getLightUrl(), "GET", null, toggleCallback);
}

/**
 * Sends a request to the Hue Bridge to set the light ON/OFF.
 *
 * Callback checks successfulness of the operation and sends the state
 * back to the pebble app.
 *
 * If this function has been called, bridge data is present, no need to check 
 */
function setLightState(on_state) {
    const turnLightCallback = function(jsonStrDataBack) {
        if (!jsonStrDataBack) return;
        const parsedJson = JSON.parse(jsonStrDataBack);
        const lightKey = "/lights/" + OPTIONS.HUE_LIGHT_ID + "/state/on";
        if (parsedJson[0].success !== undefined) {
            messageSendLightState(parsedJson[0].success[lightKey]);
        } else {
            messageSendLightState(-1);
            console.log("Error in turn light callback: " + jsonStrDataBack);
        }
    };
    ajaxRequest(getLightUrl() + "/state",
                "PUT",
                "{\"on\": " + on_state + "}",
                turnLightCallback);
}

function setLightBrightness(level) {
    if (!areSettingSet()) {
        messageRequestBridgeData("KEY_BRIGHTNESS", level);
        return;
    }
    const setLightBrightnessCallback = function (jsonStrDataBack) {
        if (!jsonStrDataBack) return;
        const parsedJson = JSON.parse(jsonStrDataBack);
        if (parsedJson[0].success === undefined) {
            messageSendLightState(-1);
            console.log("Error in set brightness callback: " +
                        jsonStrDataBack);
        } // No else, as success does not require further action
    };
    ajaxRequest(getLightUrl() + "/state", "PUT", "{\"bri\": " + level + "}",
                setLightBrightnessCallback);
}

function requestLightBrightness() {
    const requestLightBrightnessCallback = function (jsonStrDataBack) {
        if (!jsonStrDataBack) return;
        const parsedJson = JSON.parse(jsonStrDataBack);
        if (parsedJson.state.bri !== undefined) {
            messageSendLightBrightness(parsedJson.state.bri);
        } else {
            messageSendLightState(-1);
            console.log("Error in getting brightness callback: " +
                        jsonStrDataBack);
        }
    };
    ajaxRequest(getLightUrl(), "GET", null, requestLightBrightnessCallback);
}

function getLightUrl() {
    return "http://" + OPTIONS.HUE_BRIDGE_IP + "/api/" +
           OPTIONS.HUE_BRIDGE_USER + "/lights/" + OPTIONS.HUE_LIGHT_ID;
}

function areSettingSet() {
    if ((OPTIONS.HUE_BRIDGE_IP !== "") && (OPTIONS.HUE_BRIDGE_USER !== "") && 
        (OPTIONS.HUE_LIGHT_ID !== 0)) {
        return true;
    }
    return false;
}

function ajaxRequest(url, type, data, callback) {
    var xhRequest = new XMLHttpRequest();
    xhRequest.open(type, url, true);
    // The data received is JSON, so it needs to be converted
    xhRequest.onreadystatechange = function() {
        if (xhRequest.readyState == 4) {
            if (xhRequest.status == 200) {
                //logReturnedData(this.responseText);
                callback(this.responseText);
            } else {
                // return a null element, will be dealt with in callback
                callback(null);
            }
        }
    };
    xhRequest.send(data);
}
