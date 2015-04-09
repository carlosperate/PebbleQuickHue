/*******************************************************************************
* Header file for Hue Control
*
* Copyright (c) 2015 carlosperate https://github.com/carlosperate/
* Licensed under The MIT License (MIT), a copy can be found in the LICENSE file.
*
* The Hue Control module controls the AppMessage data between the Pebble and the
* PebbleKit JS (phone component), to request and respond to the Hue light
* changes.
*******************************************************************************/
#ifndef HUE_CONTROL_H_
#define HUE_CONTROL_H_

/*******************************************************************************
* Includes
*******************************************************************************/
#include <pebble.h>

typedef enum {
    LIGHT_STATE_ERROR = -1,
    LIGHT_STATE_OFF = 0,
    LIGHT_STATE_ON = 1,
    LIGHT_STATE_POWER_OFF = 2
} light_t;
    
/*******************************************************************************
* Public function definitions
*******************************************************************************/
void inbox_received_callback(DictionaryIterator *iterator, void *context);
void inbox_dropped_callback(AppMessageResult reason, void *context);
void outbox_sent_callback(DictionaryIterator *iterator, void *context);
void outbox_failed_callback(
        DictionaryIterator *iterator, AppMessageResult reason, void *context);
void toggle_light_state();
void set_brightness(int8_t level);
void send_bridge_settings();

#endif  // HUE_CONTROL_H_
