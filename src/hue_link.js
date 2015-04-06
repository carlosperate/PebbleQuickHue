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
 * On ready state we want to toggle the light inmediatly.
 */
Pebble.addEventListener("ready", function(e) {
    //console.log('PebbleKit JS ready!');
    toggleLightState();
});

/**
 * Listener for AppMessage received.
 */
Pebble.addEventListener("appmessage", function(e) {
    console.log("AppMessage received! " + JSON.stringify(e.payload));
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
function appMessageSendLightState(on_state) {
    // No boolean type defined, so need to send a 0/1 value
    var state;
    if (on_state === true) {
        state = 1;
    } else {
        state = 0;
    }
    // Assemble dictionary and send
    var dictionary = {
        "KEY_LIGHT_STATE": state,
    };
    Pebble.sendAppMessage(dictionary,logReturnedData, logReturnedData);
}

/**
 * Sends and AppMessage with the ON/OFF state of the light.
 */
function appMessageSendLightBrightness(level) {
    var dictionary = {
        "KEY_BRIGHTNESS": level,
    };
    Pebble.sendAppMessage(dictionary,logReturnedData, logReturnedData);
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
    ajaxRequest(lightUrl + "/state", "PUT", "{\"on\": true}", turnLightCallback);
}

function turnLightOff() {
    ajaxRequest(lightUrl + "/state", "PUT", "{\"on\": false}", turnLightCallback);
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
            //console.log(lightKey + " = " + parsedJson[0].success[lightKey]);
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
            var lightKey = "/lights/" + HUE_LIGHT_ID + "/state/on";
            if (parsedJson[0].success !== undefined) {
                appMessageSendLightBrightness(parsedJson[0].success[lightKey]);
            } else {
                // Do nothing in case of error, user can try again
                console.log("Error in set brightness callback: " + 
                        JSON.stringify(jsdonDataBack));
            }
        } 
    };

    // Convert the 0-99 level to 0-254ish
    level = Math.round(level * 2.56);
    
    // Send the ON command to the hue bridge
    ajaxRequest(lightUrl + "/state", "PUT", "{\"bri\": " + level + "}",
                setLightBrightnessCallback);
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
