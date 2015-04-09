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
    "HUE_LIGHT_ID": 0      // There is no ID 0 in the Hue system
};


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
    Pebble.openURL("http://carlosperate.github.io/PebbleQuickHue/config/index.html?" + 
                   encodeURIComponent(JSON.stringify(OPTIONS)));
});

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
var setBridgeDataAttemps = 0;
function messageSetBridgeData(ip, user, lightId) {
    // Values will be ignored
    var dictionary = {};
    if (ip      !== null) dictionary["KEY_BRIDGE_IP"] = ip;
    if (user    !== null) dictionary["KEY_BRIDGE_USER"] = user;
    if (lightId !== null) dictionary["KEY_LIGHT_ID"] = lightId;

    // Send the message, if an error occurs try again up to 3 times
    Pebble.sendAppMessage(dictionary,
            function(e) { setBridgeDataAttemps = 0; },
            function(e) {
                reAttemp(e, setBridgeDataAttemps, messageSetBridgeData);
            });
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
    var toggleCallback = function(jsdonStrDataBack) {
        if (jsdonStrDataBack !== null) {
            var parsedJson = JSON.parse(jsdonStrDataBack);
            if ((parsedJson.state !== undefined) &&
                (parsedJson.state.on !== undefined)) {
                if (parsedJson.state.on === false) {
                    turnLightOn();
                } else {
                    turnLightOff();
                }
            } else {
                messageSendLightState(-1);
                console.log("Error in getting light state: " + 
                            jsdonStrDataBack);
            }

        }
    };
    ajaxRequest(getLightUrl(), "GET", null, toggleCallback);
}

/** If this function is called, bridge data is present, no need to check */
function turnLightOn() {
    ajaxRequest(getLightUrl() + "/state", "PUT", "{\"on\": true}",
                turnLightCallback);
}

/** If this function is called, bridge data is present, no need to check */
function turnLightOff() {
    ajaxRequest(getLightUrl() + "/state", "PUT", "{\"on\": false}",
                turnLightCallback);
}

/**
 * This callback checks the successfulness of the operation and sends the state
 * back to the pebble app.
 */
function turnLightCallback(jsdonStrDataBack) {
    if (jsdonStrDataBack !== null) {
        var parsedJson = JSON.parse(jsdonStrDataBack);
        var lightKey = "/lights/" + OPTIONS.HUE_LIGHT_ID + "/state/on";
        if (parsedJson[0].success !== undefined) {
            messageSendLightState(parsedJson[0].success[lightKey]);
        } else {
            messageSendLightState(-1);
            console.log("Error in turn light callback: " + 
                        jsdonStrDataBack);
        }
    } 
}

function setLightBrightness(level) {
    if (!areSettingSet()) {
        messageRequestBridgeData("KEY_BRIGHTNESS", level);
        return;
    }
    var setLightBrightnessCallback = function (jsdonStrDataBack) {
        if (jsdonStrDataBack !== null) {
            var parsedJson = JSON.parse(jsdonStrDataBack);
            if (parsedJson[0].success === undefined) {
                messageSendLightState(-1);
                console.log("Error in set brightness callback: " +
                            jsdonStrDataBack);
            } // No else, as success does not require futher action
        } 
    };
    ajaxRequest(getLightUrl() + "/state", "PUT", "{\"bri\": " + level + "}",
                setLightBrightnessCallback);
}

function requestLightBrightness() {
    var requestLightBrightnessCallback = function (jsdonStrDataBack) {
        if (jsdonStrDataBack !== null) {
            var parsedJson = JSON.parse(jsdonStrDataBack);
            if (parsedJson.state.bri !== undefined) {
                messageSendLightBrightness(parsedJson.state.bri);
            } else {
                messageSendLightState(-1);
                console.log("Error in getting brightness callback: " +
                            jsdonStrDataBack);
            }
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
