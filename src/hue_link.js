/*******************************************************************************
* PebbleKit JS (runs on phone) code to trigger Hue light operations.
*
* Copyright (c) 2015 carlosperate https://github.com/carlosperate/
* Licensed under The MIT License (MIT), a copy can be found in the LICENSE file.
*******************************************************************************/

// This variables are to be changed by the developer to fit their settings
// In the near future the phone settings will allow to edit this data 
var HUE_BRIDGE_IP = "";
var HUE_BRIDGE_USER = "";
var HUE_LIGHT_ID = "4";
var lightUrl = "http://" + HUE_BRIDGE_IP + "/api/" + HUE_BRIDGE_USER +
               "/lights/" + HUE_LIGHT_ID;


/*******************************************************************************
* AppMessage functions
*******************************************************************************/
/**
 * On ready state we want to toggle the light immediately.
 */
Pebble.addEventListener("ready", function(e) {
    toggleLightState();
});

/**
 * Listener for AppMessage received.
 */
Pebble.addEventListener("appmessage", function(e) {
    //console.log("AppMessage received! " + JSON.stringify(e.payload));
    for (var key in e.payload){
        if (key == "KEY_LIGHT_STATE") {
            toggleLightState();
        } else if (key == "KEY_BRIGHTNESS") {
            setLightBrightness(e.payload.KEY_BRIGHTNESS);
        } else {
            console.log("Unrecognised AppMessage key received in JS!");
        }
    }
});

/**
 * Sends and AppMessage with the ON/OFF state of the light.
 */
var sendStateAttemps = 0; 
function appMessageSendLightState(on_state) {
    // No boolean type defined, so need to send a 0/1 value
    var state;
    if (on_state === true) {
        state = 1;
    } else {
        state = 0;
    }
    // Assemble dictionary
    var dictionary = {
        "KEY_LIGHT_STATE": state,
    };
    // Send the message, if an error occurs try again up to 3 times
    Pebble.sendAppMessage(dictionary,
            function(e) {
                sendStateAttemps = 0;
            },
            function(e) {
                sendStateAttemps++;
                console.log("Unable to deliver message with Id=" +
                            e.data.transactionId +
                            ", attempt " + sendStateAttemps +
                            ", Error is: " + e.error.message);
                if (sendStateAttemps < 3) {
                    appMessageSendLightState();
                }
            });
    
    // Now that the ON/OFF data is on its way to the pebble, request the
    // current brightness as well so that it can be displayed too
    if (on_state === true) {
        requestLightBrightness();
    }
}

/**
 * Sends and AppMessage with the ON/OFF state of the light.
 */
var sendBrightnessAttemps = 0;
function appMessageSendLightBrightness(level) {
    // Convert level from 0-255 to 0-99
    //level = Math.round(level / 2.56);
    var dictionary = {
        "KEY_BRIGHTNESS": level,
    };
    // Send the message, if an error occurs try again up to 3 times
    Pebble.sendAppMessage(dictionary,
            function(e) {
                sendBrightnessAttemps = 0;
            },
            function(e) {
                console.log("Unable to deliver message with Id=" +
                            e.data.transactionId +
                            ", attempt " + sendBrightnessAttemps +
                            ", Error is: " + e.error.message);
                sendBrightnessAttemps++;
                if (sendBrightnessAttemps < 3) {
                    sendBrightnessAttemps();
                }
            });
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
    ajaxRequest(lightUrl, "GET", null, toggleCallback);
}

function turnLightOn() {
    // Send the ON command to the hue bridge
    ajaxRequest(lightUrl + "/state", "PUT", "{\"on\": true}",
                turnLightCallback);
}

function turnLightOff() {
    ajaxRequest(lightUrl + "/state", "PUT", "{\"on\": false}",
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
            appMessageSendLightState(parsedJson[0].success[lightKey]);
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

    // Convert the 0-99 level to 0-254ish
    //level = Math.round(level * 2.56);
    
    // Send the ON command to the hue bridge
    ajaxRequest(lightUrl + "/state", "PUT", "{\"bri\": " + level + "}",
                setLightBrightnessCallback);
}

function requestLightBrightness() {
    var requestLightBrightnessCallback = function (jsdonDataBack) {
        if (jsdonDataBack !== null) {
            var parsedJson = JSON.parse(jsdonDataBack);
            if (parsedJson.state.bri !== undefined) {
                appMessageSendLightBrightness(parsedJson.state.bri);
            } else {
                // Log error
                console.log("Error in getting brightness callback: " + 
                        JSON.stringify(jsdonDataBack));
            }
        } 
    };

    // Send the ON command to the hue bridge
    ajaxRequest(lightUrl, "GET", null, requestLightBrightnessCallback);
}


/*******************************************************************************
* General ajax functions
*******************************************************************************/
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

function logReturnedData(dataBack) {
    if (dataBack !== null) {
        console.log("Received data: " + dataBack);
    } else {
        console.log("Ajax data callback error");
    }
}
