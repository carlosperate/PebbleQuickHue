/*******************************************************************************
* Header file for the main entry point.
*
* Copyright (c) 2015 carlosperate https://github.com/carlosperate/
* Licensed under The MIT License (MIT), a copy can be found in the LICENSE file.
*
* The main.c file creates and controls all the GUI elements and Pebble hardware
* handlers.
*******************************************************************************/
#ifndef MAIN_H_
#define MAIN_H_

/*******************************************************************************
* Includes
*******************************************************************************/
#include <pebble.h>
#include "hue_control.h"


/*******************************************************************************
* Public function definitions
*******************************************************************************/
void gui_light_state(light_t on_state);
void gui_brightness_level(int8_t level);

#endif  // MAIN_H_
