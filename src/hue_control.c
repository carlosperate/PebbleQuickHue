#include "main.h"
#include "hue_control.h"


/*******************************************************************************
* Types
*******************************************************************************/
enum {
    KEY_LIGHT_STATE = 0,
};


struct lightbulb_t {
    bool on;
    unsigned char brightness;
};


/*******************************************************************************
* Private globals
*******************************************************************************/
// None.


/*******************************************************************************
* Private function definitions
*******************************************************************************/
// None.


/*******************************************************************************
* AppMessage functions
*******************************************************************************/
void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    // Read first item
    Tuple *t = dict_read_first(iterator);
    
    // For all items
    while(t != NULL) {
        switch(t->key) {
            case KEY_LIGHT_STATE:
                // Indicate to the GUI that the light is ON/OFF
                gui_light_state((bool)t->value->uint8);
                break;
            default:
                APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
            break;
        }
        t = dict_read_next(iterator);
    }
}


void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}


void outbox_failed_callback(
        DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}


void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
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
    unsigned char value = 0;  // Value ignored, going to toggle
    dict_write_int(iterator, key, &value, sizeof(char), false /* signed */);

    // Send the message!
    app_message_outbox_send();
}


void increase_brightness() {
    
}


void decrease_brightness() {
    
}
