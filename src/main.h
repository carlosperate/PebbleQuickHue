/*******************************************************************************
* Header file for the main entry point.
*
* Copyright (c) 2015 carlosperate https://github.com/carlosperate/
* Licensed under The MIT License (MIT), a copy can be found in the LICENSE file.
*
* Full description here.
*******************************************************************************/
#ifndef MAIN_H_
#define MAIN_H_

/*******************************************************************************
* Includes
*******************************************************************************/
#include <pebble.h>

    
/*******************************************************************************
* Public function definitions
*******************************************************************************/
void gui_light_state(bool on_state);
void gui_brightness_level(signed char level);

#endif  // MAIN_H_
