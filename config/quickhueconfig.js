// Converts all the form data into a JSON string
function jsonifyForm() {
    return JSON.stringify($("#config_form").serializeArray());
}

// On page load and listener and fill the form with URL data
$(document).ready(function() {
    // Prepare the button click listeners
    $("#cancel_button").click(function() {
        document.location = "pebblejs://close";
    });
    $("#submit_button").click(function() {
        document.location = "pebblejs://close#" + encodeURIComponent(jsonifyForm());
    });
    // Load the pre-filled data from PebbleKit JS
    if (window.location.search) {
        var jsonFormData = $.parseJSON(
                decodeURIComponent(window.location.search.substring(1)));
        for (key in jsonFormData) {
            $("#"+[key]).val(jsonFormData[key]);
        }
    }
});

// Register new or set username inserted in the HUE_BRIDGE_USER field
function HueRegistration() {
    // First check that the Bridge IP has been entered
    var ip = $("#HUE_BRIDGE_IP").val();
    if (!ip) {
        alert("Please insert the HUE bridge IP into the form first.");
        return;
    }
    // Prepare and launch modal
    $("#ip_checking").show();
    $("#ip_correct").hide();
    $("#registering").hide();
    $("#registered_already").hide();
    $("#register_ok").hide();
    $("#register_modal").openModal({in_duration: 0});
    // Check if IP belongs to bridge, freaking materialize needs timeout
    setTimeout(function() {
        if (!checkBridgeIpCorrectness(ip)) {
            alert("The IP in the form is not the correct Hue Bridge IP.");
            $("#register_modal").closeModal();
            return;
        }
        $("#ip_checking").hide();
        $("#ip_correct").show();
        $("#registering").show();
        // Callback function to indicate registration success
        var setSuccessfulUsername = function(newUsername) {
            $("#registering").hide();
            $("#register_ok").show();
            $("#HUE_BRIDGE_USER").val(newUsername);
            $("#HUE_BRIDGE_USER_label").addClass("active");
        }
        var username = $("#HUE_BRIDGE_USER").val();
        if (username) {
            if (checkBridgeUserExistance(ip, username)) {
                $("#registering").hide();
                $("#registered_already").show();
            } else {
                registerBridgeUser(ip, username, setSuccessfulUsername);
            }
        } else {
            registerBridgeUser(ip, null, setSuccessfulUsername);
        }
    }, (100));
}

// Synchronous function to check if IP belongs to Hue Bridge
function checkBridgeIpCorrectness(ip) {
    var correctIp = false;
    $.ajax({
        url: "http://" + ip + "/api/",
        async: false,
    }).done(function(data) { correctIp = true; });
    return correctIp;
}

// Synchronous function to check if user already exists in Hue Bridge
function checkBridgeUserExistance(ip, username) {
    var userExists = false;
    $.ajax({
        url: "http://" + ip + "/api/" + username + "/",
        async: false
    }).done(function(data) {
        if (data.config !== undefined) {
            userExists = true;
        } 
    });
    return userExists;
}

// Register a username as part of the QuickHue app into the bridge
function registerBridgeUser(ip, username, successCallback) {
    var processResponse = function(data) {
        if (data[0].success !== undefined) {
            if (typeof(successCallback) === typeof(Function)) {
                successCallback(data[0].success.username);
            }
        } else if (data[0].error !== undefined && (data[0].error.type == 101)) {
            // Keep recursion waiting for button in bridge to be pressed
            console.log("Press the sync button in the Hue Bridge.");
            registerBridgeUser(ip, username, successCallback);
        } else {
            alert("Unexpected error registering user into Bridge:\n" +
                  JSON.stringify(data));
        }
    };
    var apiUrl = "http://" + ip + "/api/";
    if (username) {
        var createUserJsonStr = '{"devicetype": "QuickHue",' +
                                ' "username": "' + username +'"}';
        $.post(apiUrl, createUserJsonStr, processResponse, "json");
    } else {
        var createUserJsonStr = '{"devicetype": "QuickHue"}';
        $.post(apiUrl, createUserJsonStr, processResponse, "json");
    }
}
