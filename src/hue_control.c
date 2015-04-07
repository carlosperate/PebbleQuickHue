#include "hue_control.h"
#include "main.h"


/*******************************************************************************
* AppMessage Keys
*******************************************************************************/
enum {
    KEY_LIGHT_STATE = 0,
    KEY_BRIGHTNESS = 1,
};


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
            default:
                APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!",
                        (int)t->key);
            break;
        }
        t = dict_read_next(iterator);
    }
}


/**
 * Logg a message dropped, but there is not much else we can do.
 */
void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
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
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}


/**
 * Right now there is no need to trigger a callback with any of the messages.
 */
void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}


/*******************************************************************************
* Hue control functions
*******************************************************************************/
void toggle_light_state() {
    // Prepare dictionary
    DictionaryIterator *iterator;
    app_message_outbox_begin(&iterator);

    // Add a key-value pair
    int key = KEY_LIGHT_STATE;
    int8_t value = 0;  // Value ignored, will always toggle
    dict_write_int8(iterator, key, value);

    // Send the message!
    app_message_outbox_send();
}


void set_brightness(int8_t level) {
    // Prepare dictionary
    DictionaryIterator *iterator;
    app_message_outbox_begin(&iterator);

    // Add a key-value pair
    int key = KEY_BRIGHTNESS;
    int16_t hue_level = (int16_t)(level * 2.56 );
    dict_write_int16(iterator, key, hue_level);

    // Send the message!
    app_message_outbox_send();
}
