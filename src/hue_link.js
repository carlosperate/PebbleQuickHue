/*******************************************************************************
* PebbleKit JS (runs on phone) code to trigger Hue light operations.
*
* Copyright (c) 2015 carlosperate https://github.com/carlosperate/
* Licensed under The MIT License (MIT), a copy can be found in the LICENSE file.
*******************************************************************************/

// This 3 variables can be changed by the developer for evern quicker startup
// If the IP and username are hardcoded, also set the flag to True
var HUE_BRIDGE_IP = "";
var HUE_BRIDGE_USER = "";
var bridgeDataHardcoded = false;

var HUE_LIGHT_ID = "4";


/*******************************************************************************
* PebbleKit JS functions
*******************************************************************************/
/** Retrieve the bridge data and toggle the light immediately asap. */
Pebble.addEventListener("ready", function(e) {
    if (bridgeDataHardcoded) {
        toggleLightState();
    } else {
        messageRequestBridgeData();
    }
});


/*******************************************************************************
* PebbleKit App Configuration functions
*******************************************************************************/
Pebble.addEventListener("showConfiguration", function(e) {
    Pebble.openURL("http://carlosperate.github.io/PebbleQuickHue/config/index.html");
});

Pebble.addEventListener("webviewclosed", function(e) {
    var bridgeConfig = JSON.parse(decodeURIComponent(e.response));
    var hue_ip = null;
    var hue_user = null;
    for (var i=0; i < bridgeConfig.length; i++) {
        if (bridgeConfig[i].name === "hue_ip") {
            hue_ip = bridgeConfig[i].value;
            HUE_BRIDGE_IP = hue_ip;
        } else if (bridgeConfig[i].name === "hue_user") {
            hue_user = bridgeConfig[i].value;
            HUE_BRIDGE_USER = hue_user;
        }
    }
    messageSetBridgeData(hue_ip, hue_user);
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
            HUE_BRIDGE_IP = e.payload.KEY_BRIDGE_IP;
        } else if (key == "KEY_BRIDGE_USER") {
            HUE_BRIDGE_USER = e.payload.KEY_BRIDGE_USER;
        } else {
            console.log("Unrecognised AppMessage key received in JS!");
        }
    }
});

/** Sends and AppMessage with the ON/OFF state of the light. */
var sendStateAttemps = 0; 
function messageSendLightState(on_state) {
    // No boolean type defined, so need to send a 0/1 value
    var state = 0;
    if (on_state === true) state = 1;

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
function messageSetBridgeData(ip, username) {
    // Values will be ignored
    var dictionary = {};
    if (ip !== null) {
        dictionary["KEY_BRIDGE_IP"] = ip;
    }
    if (username !== null) {
        dictionary["KEY_BRIDGE_USER"] = username;
    }
    // Send the message, if an error occurs try again up to 3 times
    Pebble.sendAppMessage(dictionary,
            function(e) { setBridgeDataAttemps = 0; },
            function(e) {
                reAttemp(e, setBridgeDataAttemps, messageSetBridgeData);
            });
}

/** Request the Hue Bridge IP and Username from the pebble app storage. */
var requestBridgeAttemps = 0;
function messageRequestBridgeData() {
    var dictionary = { "KEY_BRIDGE_REQUEST": 0 };  // Value will be ignored
    // Send the message, if an error occurs try again up to 3 times
    Pebble.sendAppMessage(dictionary,
            function(e) { requestBridgeAttemps = 0; },
            function(e) {
                reAttemp(e, requestBridgeAttemps, messageRequestBridgeData);
            });
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
    var toggleCallback = function(jsdonDataBack) {
        if (jsdonDataBack !== null) {
            var parsedJson = JSON.parse(jsdonDataBack);
            if (parsedJson.state.on === false) {
                turnLightOn();
            } else {
                turnLightOff();
            }
        }
    };
    ajaxRequest(getLightUrl(), "GET", null, toggleCallback);
}

function turnLightOn() {
    ajaxRequest(getLightUrl() + "/state", "PUT", "{\"on\": true}",
                turnLightCallback);
}

function turnLightOff() {
    ajaxRequest(getLightUrl() + "/state", "PUT", "{\"on\": false}",
                turnLightCallback);
}

/**
 * This callback checks the successfulness of the operation and sends the state
 * back to the pebble app.
 */
function turnLightCallback(jsdonDataBack) {
    if (jsdonDataBack !== null) {
        var parsedJson = JSON.parse(jsdonDataBack);
        var lightKey = "/lights/" + HUE_LIGHT_ID + "/state/on";
        if (parsedJson[0].success !== undefined) {
            messageSendLightState(parsedJson[0].success[lightKey]);
        } else {
            // Do nothing in case of error, user can try again
            console.log("Error in turn light callback: " + 
                        JSON.stringify(jsdonDataBack));
        }
    } 
}

function setLightBrightness(level) {
    var setLightBrightnessCallback = function (jsdonDataBack) {
        if (jsdonDataBack !== null) {
            var parsedJson = JSON.parse(jsdonDataBack);
            if (parsedJson[0].success === undefined) {
                // Do nothing in case of error, user can try again
                console.log("Error in set brightness callback: " +
                        JSON.stringify(jsdonDataBack));
            } // No else, as success does not require futher action
        } 
    };
    ajaxRequest(getLightUrl() + "/state", "PUT", "{\"bri\": " + level + "}",
                setLightBrightnessCallback);
}

function requestLightBrightness() {
    var requestLightBrightnessCallback = function (jsdonDataBack) {
        if (jsdonDataBack !== null) {
            var parsedJson = JSON.parse(jsdonDataBack);
            if (parsedJson.state.bri !== undefined) {
                messageSendLightBrightness(parsedJson.state.bri);
            } else {
                // Log error
                console.log("Error in getting brightness callback: " +
                        JSON.stringify(jsdonDataBack));
            }
        }
    };
    ajaxRequest(getLightUrl(), "GET", null, requestLightBrightnessCallback);
}

function getLightUrl() {
    return "http://" + HUE_BRIDGE_IP + "/api/" + HUE_BRIDGE_USER + "/lights/" +
           HUE_LIGHT_ID;
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
