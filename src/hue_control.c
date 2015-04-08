#include <string.h>
#include "hue_control.h"
#include "main.h"


/*******************************************************************************
* Defines
*******************************************************************************/
#define STORAGE_IP_LENGTH       16
// Currently sdk PERSIST_DATA_MAX_LENGTH is 256
#define STORAGE_USER_LENGTH    (PERSIST_DATA_MAX_LENGTH - STORAGE_IP_LENGTH)


/*******************************************************************************
* AppMessage Keys
*******************************************************************************/
enum {
    KEY_LIGHT_STATE = 0,
    KEY_BRIGHTNESS = 1,
    KEY_BRIDGE_IP = 2,
    KEY_BRIDGE_USER = 3,
    KEY_BRIDGE_REQUEST = 4
};


/*******************************************************************************
* Private function definitions
*******************************************************************************/
static char * translate_error(AppMessageResult result);
static void store_bridge_ip(const char *cstring);
static void store_bridge_username(const char *cstring);
static char * get_stored_bridge_ip();
static char * get_stored_bridge_username();


/*******************************************************************************
* AppMessage functions
*******************************************************************************/
void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    // Read first item
    Tuple *t = dict_read_first(iterator);
    
    // For all items
    while(t != NULL) {
        int8_t level = 0;
        switch(t->key) {
            case KEY_LIGHT_STATE:
                // Indicate to the GUI that the light is ON/OFF
                gui_light_state((bool)t->value->uint8);
                break;
            case KEY_BRIGHTNESS:
                // Indicate to the GUI the new light brightness value
                level =  (int8_t)(t->value->int16 / 2.56 );
                gui_brightness_level(level);
                break;
            case KEY_BRIDGE_REQUEST:
                // Retrieve bridge data from storage and send it back
                send_bridge_settings();
                break;
            case KEY_BRIDGE_IP:
                // Set the new IP address into storage
                store_bridge_ip(t->value->cstring);
                APP_LOG(APP_LOG_LEVEL_INFO, "Ip set to %s", t->value->cstring);
                break;
            case KEY_BRIDGE_USER:
                // Set the new bridge username into storage
                store_bridge_username(t->value->cstring);
                APP_LOG(APP_LOG_LEVEL_INFO, "User set to %s", t->value->cstring);
                break;
            default:
                APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!",
                        (int)t->key);
            break;
        }
        t = dict_read_next(iterator);
    }
}


/**
 * Right now there is no need to trigger a callback with any of the messages.
 */
void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}


/**
 * Logg a message dropped, but there is not much else we can do.
 */
void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped! %s",
            translate_error(reason));
}


/**
 * Right now there are 2 messages that can fail:
 *   Toggle fail will not cause any changes on GUI or bridge so can be ignored.
 *   Set brightness is more likely to fail when quickly changing value, so
 *   retrying would set an old value after it is no longer relevant.
 * Therefore, both ignored.
 */
void outbox_failed_callback(
        DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed! %s",
            translate_error(reason));
}


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
    // Any string > STORAGE_IP_LENGTH (including terminator) will be truncated
    if (strlen(cstring) > (STORAGE_IP_LENGTH - 1)) {
        char bridge_ip[STORAGE_IP_LENGTH];
        strncpy(bridge_ip, cstring, (STORAGE_IP_LENGTH - 1));
        bridge_ip[STORAGE_IP_LENGTH - 1] = '\0';
        persist_write_string(KEY_BRIDGE_IP, bridge_ip);
    } else {
        persist_write_string(KEY_BRIDGE_IP, cstring);
    }
}


static void store_bridge_username(const char *cstring) {
    // Any string > STORAGE_USER_LENGTH (including terminator) will be truncated
    if (strlen(cstring) > (STORAGE_USER_LENGTH - 1)) {
        char bridge_user[STORAGE_USER_LENGTH];
        strncpy(bridge_user, cstring, (STORAGE_USER_LENGTH - 1));
        bridge_user[STORAGE_USER_LENGTH - 1] = '\0';
        persist_write_string(KEY_BRIDGE_IP, bridge_user);
    } else {
        persist_write_string(KEY_BRIDGE_USER, cstring);
    }
}


/**
 *
 * :return: Char pointer to the IP address in string format. This pointer can be
 *          null if there was a memory allocation error.
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
            return NULL;
        }
    }
    return bridge_ip;
}


/**
 *
 * :return: Char pointer to the IP address in string format. This pointer can be
 *          null if there was a memory allocation error.
 */
static char * get_stored_bridge_username() {
    // Currently PERSIST_DATA_MAX_LENGTH is 256, to future proof 16bit var
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
        
    }
    // bridge_username is rather large, so let's reduce it
    //username_len = strlen(bridge_username);
    char * bridge_shortname = malloc(sizeof(char) * (username_len + 1));
    strncpy(bridge_shortname, bridge_username, username_len);
    free(bridge_username);
    return bridge_shortname;
}


/*******************************************************************************
* Hue control functions
*******************************************************************************/
/**
 * Request the light to be toggle by sending the message to the PebbleKit JS
 *  app to send it to the bridge using JSON.
 */
void toggle_light_state() {
    // Prepare dictionary, add key-value pair and send it
    DictionaryIterator *iterator;
    app_message_outbox_begin(&iterator);
    // Value ignored, will always toggle
    dict_write_int8(iterator, KEY_LIGHT_STATE, 0);
    app_message_outbox_send();
}

/**
 * Sends the brightness level to the PebbleKit JS app to send it to the bridge
 * using JSON.
 */
void set_brightness(int8_t level) {
    // Prepare dictionary, add key-value pair and send it
    DictionaryIterator *iterator;
    app_message_outbox_begin(&iterator);
    dict_write_int16(iterator, KEY_BRIGHTNESS, (int16_t)(level * 2.56 ));
    app_message_outbox_send();
}


/**
 * Retrieves the bridge data from storage and sends it to the PebbleKit JS
 * phone app. Because this function is requested "on ready" of the phone
 * portion of the app, we also send a toggle light request to be served
 * with the newly retrieved bridge data.
 */
void send_bridge_settings() {
    char *ip = get_stored_bridge_ip();
    char *username = get_stored_bridge_username();
    
    // Only send the data if it was retrieved
    if ((ip != NULL) && (username != NULL)) {
        APP_LOG(APP_LOG_LEVEL_INFO, "Ip to send %s", ip);
        APP_LOG(APP_LOG_LEVEL_INFO, "User to send %s", username);
        
        // Prepare dictionary, Add key-value pairs and send it
        DictionaryIterator *iterator;
        app_message_outbox_begin(&iterator);
        dict_write_cstring(iterator, KEY_BRIDGE_IP, ip);
        dict_write_cstring(iterator, KEY_BRIDGE_USER, username);
        dict_write_cstring(iterator, KEY_LIGHT_STATE, 0);
        app_message_outbox_send();
    }
}
