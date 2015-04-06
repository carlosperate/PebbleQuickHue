/*******************************************************************************
* Javascript (runs on phone) to trigger Hue light operations
*
* Copyright (c) 2015 carlosperate https://github.com/carlosperate/
* Licensed under The MIT License (MIT), a copy can be found in the LICENSE file.
*******************************************************************************/

// This variables are to be changed by the developer to fit their settings
// In the near future the phone settings will allow to edit this data 
var HUE_BRIDGE_IP = "192.168.0.10";
var HUE_BRIDGE_USER = "31ef239e77af967203e11b21f5b878b";
var HUE_LIGHT_ID = "4";
var lightUrl = "http://" + HUE_BRIDGE_IP + "/api/" + HUE_BRIDGE_USER +
               "/lights/" + HUE_LIGHT_ID;

/*******************************************************************************
* AppMessage functions
*******************************************************************************/
Pebble.addEventListener("ready", function(e) {
    console.log('PebbleKit JS ready!');
    toggleLightState();
});

/**
 * Listener for AppMessage received
 */
Pebble.addEventListener("appmessage", function(e) {
    console.log("AppMessage received!");
    toggleLightState();
});


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

function turnLightCallback(jsdonDataBack) {
    if (jsdonDataBack !== null) {
        var parsedJson = JSON.parse(jsdonDataBack);
        var lightKey = "/lights/" + HUE_LIGHT_ID + "/state/on";
        if (parsedJson[0].success !== undefined) {
            var state;
            console.log(lightKey + " = " + parsedJson[0].success[lightKey]);
            if (parsedJson[0].success[lightKey] === true) {
                state = 1;
            } else {
                state = 0;
            }
            // Send an AppMessage packet to the phone indicating the light will be on
            // Assemble dictionary using app keys
            var dictionary = {
                "KEY_LIGHT_STATE": state,
            };
            Pebble.sendAppMessage(dictionary,logReturnedData, logReturnedData);
        } else {
            // Do nothing in case of error, user can try again
        }
    } 
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
