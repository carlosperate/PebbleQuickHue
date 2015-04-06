/*******************************************************************************
* Header file for Hue Control
*
* Copyright (c) 2015 carlosperate https://github.com/carlosperate/
* Licensed under The MIT License (MIT), a copy can be found in the LICENSE file.
*
* Full description here.
*******************************************************************************/
#ifndef HUE_CONTROL_H_
#define HUE_CONTROL_H_

/*******************************************************************************
* Includes
*******************************************************************************/
#include <pebble.h>

    
/*******************************************************************************
* Public function definitions
*******************************************************************************/
void inbox_received_callback(DictionaryIterator *iterator, void *context);
void inbox_dropped_callback(AppMessageResult reason, void *context);
void outbox_sent_callback(DictionaryIterator *iterator, void *context);
void outbox_failed_callback(
        DictionaryIterator *iterator, AppMessageResult reason, void *context);
void toggle_light_state();
void set_brightness(char level);

#endif  // HUE_CONTROL_H_
