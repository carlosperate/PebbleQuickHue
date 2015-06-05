/*******************************************************************************
* Code file for Hue Control
*
* Copyright (c) 2015 carlosperate https://github.com/carlosperate/
* Licensed under The MIT License (MIT), a copy can be found in the LICENSE file.
*******************************************************************************/
#include <string.h>
#include "hue_control.h"
#include "main.h"


/*******************************************************************************
* Defines
*******************************************************************************/
#define LIGHT_ID_ERROR          -1
#define STORAGE_IP_LENGTH       16
// Saved as 32bit for alignment
#define STORAGE_LIGHT_LENGTH     4
// Currently sdk PERSIST_DATA_MAX_LENGTH is 256
#define STORAGE_USER_LENGTH    (PERSIST_DATA_MAX_LENGTH - STORAGE_IP_LENGTH - \
                                STORAGE_LIGHT_LENGTH - 4)


/*******************************************************************************
* AppMessage Keys
*******************************************************************************/
enum {
    KEY_LIGHT_STATE = 0,
    KEY_BRIGHTNESS = 1,
    KEY_BRIDGE_IP = 2,
    KEY_BRIDGE_USER = 3,
    KEY_LIGHT_ID = 4,
    KEY_SETT_REQUEST = 5
};


/*******************************************************************************
* Private function definitions
*******************************************************************************/
static char * translate_error(AppMessageResult result);
static void store_bridge_ip(const char *cstring);
static void store_bridge_username(const char *cstring);
static void store_light_id(const int8_t light_id);
static char * get_stored_bridge_ip();
static char * get_stored_bridge_username();
static int8_t get_stored_light_id();


/*******************************************************************************
* AppMessage functions
*******************************************************************************/
void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    int8_t level = 0;

    // 1st we are going to check for the presence of KEY_SETT_REQUEST. This is
    // a special case, this key can be sent with an operation retry request
    // (only two options designed), and the program response is different:
    Tuple *t = dict_read_first(iterator);
    while (t != NULL) {
        switch (t->key) {
            case KEY_SETT_REQUEST:
                // Retrieve bridge data from storage and send it back
                send_bridge_settings();
                //APP_LOG(APP_LOG_LEVEL_INFO, "Settings requested.");

                // Start from the begging to look for the other key
                t = dict_read_first(iterator);
                while (t != NULL) {
                    switch (t->key) {
                        case KEY_LIGHT_STATE:
                            // Try to toggle once again
                            toggle_light_state();
                            //APP_LOG(APP_LOG_LEVEL_INFO,
                            //       "Toggle light with settings request.");
                        break;
                        case KEY_BRIGHTNESS:
                            // Try to set brightness again with sent back data
                            level =  (int8_t)(t->value->int16 / 2.56 );
                            set_brightness(level);
                            //APP_LOG(APP_LOG_LEVEL_INFO,
                            //        "Set brightness with settings request: %d",
                            //        t->value->int16);
                        break;
                        case KEY_SETT_REQUEST:  // Expected, already dealt with
                            break;
                        default:
                            APP_LOG(APP_LOG_LEVEL_ERROR,
                                    "Key %d supplied with KEY_SETT_REQUEST error!",
                                    (int)t->key);
                        break;
                    }
                    t = dict_read_next(iterator);
                }
                // Found and processed the KEY_SETT_REQUEST, can exit function
                return;
            default:  // If KEY_SETT_REQUEST not found, then just continue.
                break;
        }
        if (t != NULL ) {
            t = dict_read_next(iterator);
        }
    }

    // Back to the first item and go through the keys as normal
    t = dict_read_first(iterator);
    while (t != NULL) {
        switch (t->key) {
            case KEY_LIGHT_STATE:
                // Indicate to the GUI that the light is ON/OFF
                gui_light_state((light_t)t->value->int8);
                //APP_LOG(APP_LOG_LEVEL_INFO, "Light on is %d", t->value->int8);
                break;
            case KEY_BRIGHTNESS:
                // Indicate to the GUI the new light brightness value
                level =  (int8_t)(t->value->int16 / 2.56 );
                gui_brightness_level(level);
                //APP_LOG(APP_LOG_LEVEL_INFO, "Brightness is %d", level);
                break;
            case KEY_BRIDGE_IP:
                // Set the new IP address into storage
                store_bridge_ip(t->value->cstring);
                //APP_LOG(APP_LOG_LEVEL_INFO, "Ip set to %s", t->value->cstring);
                break;
            case KEY_BRIDGE_USER:
                // Set the new bridge username into storage
                store_bridge_username(t->value->cstring);
                //APP_LOG(APP_LOG_LEVEL_INFO, "User set to %s",
                //        t->value->cstring);
                break;
            case KEY_LIGHT_ID:
                // Set the new bridge username into storage
                store_light_id(t->value->int8);
                //APP_LOG(APP_LOG_LEVEL_INFO, "Light ID set to %d",
                //        t->value->int8);
                break;
            case KEY_SETT_REQUEST:
                // Already check for this key, so this should no happen
            default:
                APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!",
                        (int)t->key);
            break;
        }
        t = dict_read_next(iterator);
    }
}


/**
 * Right now there is no need to trigger a callback for any of the messages.
 */
void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}


/** Logg a message dropped, but there is not much else we can do. */
void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped! %s",
            translate_error(reason));
}


/** Log a message failed with reason, right now not much else we can do. */
void outbox_failed_callback(
        DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed! %s",
            translate_error(reason));
}


/** 
 * Translates a error enum into a string that can easily be identified during
 * debugging sessions.
 * @return A string with the error title.
 */
static char * translate_error(AppMessageResult result) {
    switch (result) {
        case APP_MSG_OK:
            return "OK";
        case APP_MSG_SEND_TIMEOUT:
            return "SEND_TIMEOUT";
        case APP_MSG_SEND_REJECTED:
            return "SEND_REJECTED";
        case APP_MSG_NOT_CONNECTED:
            return "NOT_CONNECTED";
        case APP_MSG_APP_NOT_RUNNING:
            return "APP_NOT_RUNNING";
        case APP_MSG_INVALID_ARGS:
            return "INVALID_ARGS";
        case APP_MSG_BUSY:
            return "BUSY";
        case APP_MSG_BUFFER_OVERFLOW:
            return "BUFFER_OVERFLOW";
        case APP_MSG_ALREADY_RELEASED:
            return "ALREADY_RELEASED";
        case APP_MSG_CALLBACK_ALREADY_REGISTERED:
            return "CALLBACK_ALREADY_REGISTERED";
        case APP_MSG_CALLBACK_NOT_REGISTERED:
            return "CALLBACK_NOT_REGISTERED";
        case APP_MSG_OUT_OF_MEMORY:
            return "OUT_OF_MEMORY";
        case APP_MSG_CLOSED:
            return "CLOSED";
        case APP_MSG_INTERNAL_ERROR:
            return "INTERNAL_ERROR";
        default:
            return "UNKNOWN ERROR";
    }
}


/*******************************************************************************
* Data storage for Hue Bridge settings
*******************************************************************************/
static void store_bridge_ip(const char *cstring) {
    status_t status;

    // Any string > STORAGE_IP_LENGTH (including terminator) will be truncated
    if (strlen(cstring) > (STORAGE_IP_LENGTH - 1)) {
        char bridge_ip[STORAGE_IP_LENGTH];
        strncpy(bridge_ip, cstring, (STORAGE_IP_LENGTH - 1));
        bridge_ip[STORAGE_IP_LENGTH - 1] = '\0';
        status = persist_write_string(KEY_BRIDGE_IP, bridge_ip);
    } else {
        status = persist_write_string(KEY_BRIDGE_IP, cstring);
    }

    if (status != S_SUCCESS) {
        APP_LOG(APP_LOG_LEVEL_INFO, "Error trying to store the Bridge IP!");
    }
}


static void store_bridge_username(const char *cstring) {
    status_t status;

    // Any string > STORAGE_USER_LENGTH (including terminator) will be truncated
    if (strlen(cstring) > (STORAGE_USER_LENGTH - 1)) {
        char bridge_user[STORAGE_USER_LENGTH];
        strncpy(bridge_user, cstring, (STORAGE_USER_LENGTH - 1));
        bridge_user[STORAGE_USER_LENGTH - 1] = '\0';
        status = persist_write_string(KEY_BRIDGE_IP, bridge_user);
    } else {
        status = persist_write_string(KEY_BRIDGE_USER, cstring);
    }

    if (status != S_SUCCESS) {
        APP_LOG(APP_LOG_LEVEL_INFO, "Error trying to store Bridge username!");
    }
}


static void store_light_id(const int8_t light_id) {
    status_t status = persist_write_int(KEY_LIGHT_ID, (const int32_t)light_id);
    if (status != S_SUCCESS) {
        APP_LOG(APP_LOG_LEVEL_INFO, "Error trying to store the Light ID!");
    }
}


/**
 * Retrieves the Hue Bridge IP from the pebble storage.
 * @return Char pointer to the IP address in string format. This pointer can be
 *         null if there was a memory allocation error, or if the IP data has 
 *         never been saved before.
 */
static char * get_stored_bridge_ip() {
    char *bridge_ip = (char *)malloc(sizeof(char) * 16);
    if (bridge_ip == NULL) {
        // We'll deal with a null pointer in the calling function
        APP_LOG(APP_LOG_LEVEL_ERROR, "Mem alloc failure for bridge IP!");
    } else {
        int8_t buffer_len = persist_read_string(KEY_BRIDGE_IP, bridge_ip,
                                                (sizeof(char) * 16));
        if (buffer_len == E_DOES_NOT_EXIST) {
            free(bridge_ip);
            APP_LOG(APP_LOG_LEVEL_INFO, "Bridge IP not found in Pebble");
            return NULL;
        }
    }
    return bridge_ip;
}


/**
 * Retrieves the Hue Bridge developer username from the pebble storage.
 * @return Char pointer to the bridge username in string format. This pointer
 *         can be null if there was a memory allocation error, or if the
 *         username data has never been saved before.
 */
static char * get_stored_bridge_username() {
    // Currently PERSIST_DATA_MAX_LENGTH is 256, to future proof use 16bit var
    int16_t username_len = STORAGE_USER_LENGTH;
    char *bridge_username = (char *)malloc(sizeof(char) * username_len);

    if (bridge_username == NULL) {
        // Maybe we were being a bit too greedy, try smaller before giving up
        username_len = 120;
        bridge_username = (char *)malloc(sizeof(char) * username_len);
        if (bridge_username == NULL) {
             APP_LOG(APP_LOG_LEVEL_ERROR,
                     "Mem alloc failure for bridge username!");
            return NULL;
        }
    }

    int16_t buffer_len = persist_read_string(KEY_BRIDGE_USER, bridge_username,
                                             (sizeof(char) * username_len));
    if (buffer_len == E_DOES_NOT_EXIST) {
        free(bridge_username);
        APP_LOG(APP_LOG_LEVEL_INFO, "Bridge dev username not found in Pebble");
        return NULL;
    }

    // bridge_username is rather large, so let's reduce it
    char * bridge_shortname = malloc(sizeof(char) * (buffer_len + 1));
    if (bridge_shortname == NULL) {
        // Well, I guess we'll have to keep this large memory block
        return bridge_username;
    }
    strncpy(bridge_shortname, bridge_username, buffer_len);
    free(bridge_username);

    return bridge_shortname;
}


/**
 * Retrieves from storage the Light ID.
 * The return value of persist_read_int is a signed 32-bit integer, but only
 * int8s are allowed to be saved in store_light_id.
 * If the value has not yet been set, persist_read_int will return 0, in which
 * case we convert it to LIGHT_ID_ERROR. Thankfully! the HUE system starts
 * counting lights from value 1, so 0 should not be a use case.
 * @return The hue light bulb ID stored. 
 */
static int8_t get_stored_light_id() {
    int8_t light_id = (int8_t)persist_read_int(KEY_LIGHT_ID);
    if (0 == light_id) {
        APP_LOG(APP_LOG_LEVEL_INFO, "Light ID not found in Pebble");
        light_id = LIGHT_ID_ERROR;
    }
    return light_id;
}


/*******************************************************************************
* Hue control functions
*******************************************************************************/
/**
 * Request the light to be toggle by sending the message to the PebbleKit JS
 * app to send it to the bridge using JSON.
 * This is the most important operation of PebbleQuickHue, and because the
 * configuration settings are sent right before the first toggle on startup,
 * there are high chances for the app_message_outbox_send response to be
 * APP_MSG_BUSY. So, it be retried up to 5 times.
 */
void toggle_light_state() {
    // Prepare dictionary, add key-value pair and send it
    DictionaryIterator *iterator;
    app_message_outbox_begin(&iterator);
    // Value ignored, will always toggle
    dict_write_int8(iterator, KEY_LIGHT_STATE, 0);
    AppMessageResult result = app_message_outbox_send();
    if (result != APP_MSG_OK) {
        uint8_t counter = 0;
        while ((counter < 5) && (result == APP_MSG_BUSY)) {
            psleep(75);
            app_message_outbox_begin(&iterator);
            dict_write_int8(iterator, KEY_LIGHT_STATE, 0);
            result = app_message_outbox_send();
            counter++;
        }
        if (result != APP_MSG_OK) {
            APP_LOG(APP_LOG_LEVEL_ERROR, "Toggle light message error! %s",
                    translate_error(result));
        }
    } 
}


/**
 * Sends the brightness level to the PebbleKit JS app to send it to the bridge
 * using JSON.
 * Because of the continuous messages sent when the UP or DOWN button are
 * pressed, it is likely that some of these messages will be missed. We are not
 * retrying to send them more than 3 times to reduce UI hangs. 
 * @param level Brightness level from 0-99.
 */
void set_brightness(int8_t level) {
    // Prepare dictionary, add key-value pair and send it
    DictionaryIterator *iterator;
    app_message_outbox_begin(&iterator);
    dict_write_int16(iterator, KEY_BRIGHTNESS, (int16_t)(level * 2.56));
    AppMessageResult result = app_message_outbox_send();
    if (result != APP_MSG_OK) {
        uint8_t counter = 0;
        while ((counter < 3) && (result == APP_MSG_BUSY)) {
            psleep(50);
            app_message_outbox_begin(&iterator);
            dict_write_int16(iterator, KEY_BRIGHTNESS, (int16_t)(level * 2.56));
            result = app_message_outbox_send();
            counter++;
        }
        if (result != APP_MSG_OK) {
            APP_LOG(APP_LOG_LEVEL_ERROR, "Set brightness message error! %s",
                    translate_error(result));
        }
    }
}


/**
 * Retrieves the bridge data from storage and sends it to the PebbleKit JS
 * phone app.
 */
void send_bridge_settings() {
    char *ip = get_stored_bridge_ip();
    char *username = get_stored_bridge_username();
    int8_t light_id = get_stored_light_id();

    // Only send the data if it was retrieved
    if ((ip != NULL) && (username != NULL) && (light_id != LIGHT_ID_ERROR)) {
        //APP_LOG(APP_LOG_LEVEL_INFO, "Ip to send %s", ip);
        //APP_LOG(APP_LOG_LEVEL_INFO, "User to send %s", username);
        //APP_LOG(APP_LOG_LEVEL_INFO, "Light ID to send %d", light_id);

        // Prepare dictionary, Add key-value pairs and send it
        DictionaryIterator *iterator;
        app_message_outbox_begin(&iterator);
        dict_write_cstring(iterator, KEY_BRIDGE_IP, ip);
        dict_write_cstring(iterator, KEY_BRIDGE_USER, username);
        dict_write_int8(iterator, KEY_LIGHT_ID, light_id);
        AppMessageResult result = app_message_outbox_send();
        if (result != APP_MSG_OK) {
            APP_LOG(APP_LOG_LEVEL_ERROR, "Bridge settings message error! %s",
                    translate_error(result));
        }
    }

    if (ip       != NULL) free(ip);
    if (username != NULL) free(username);
}
