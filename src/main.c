/*******************************************************************************
* Code file for the main entry point.
*
* Copyright (c) 2015 carlosperate https://github.com/carlosperate/
* Licensed under The MIT License (MIT), a copy can be found in the LICENSE file.
*******************************************************************************/
#include <pebble.h>
#include <pebble_fonts.h>
#include "main.h"
#include "hue_control.h"


/*******************************************************************************
* Defines
*******************************************************************************/
#define LIGHT_STATE_ERROR -1
// Used for the brightness text
#define LIGHT_OFF         -1
#define MIN_BRIGHTNESS     1
#define MAX_BRIGHTNESS    99


/*******************************************************************************
* Local globals
*******************************************************************************/
static Window *window;
static ActionBarLayer *side_bar;
static TextLayer *title_text_layer;
static TextLayer *brightness_text_layer;
static BitmapLayer *lightbulb_bitmap_layer;
static GBitmap *lightbulb_bitmap;
static GBitmap *icon_plus;
static GBitmap *icon_minus;

static int8_t brightness_level = LIGHT_OFF;


/*******************************************************************************
* Private function definitions
*******************************************************************************/
static void init(void);
static void window_load(Window *window);
static void window_unload(Window *window);
static void deinit(void);
static void select_click_handler(ClickRecognizerRef recognizer, void *context);
static void up_click_handler(ClickRecognizerRef recognizer, void *context);
static void down_click_handler(ClickRecognizerRef recognizer, void *context);
static void click_config_provider(void *context);
static void gui_update_brightness();


/*******************************************************************************
* Life cycle functions
*******************************************************************************/
static void init(void) {
    // Register AppMessage handlers
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);

    // Open AppMessage
    // TODO: Once AppMessage code is finish determine maximum size required
    app_message_open(app_message_inbox_size_maximum(),
                     app_message_outbox_size_maximum());

    // Window
    window = window_create();
    window_set_click_config_provider(window, click_config_provider);
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
     });
    window_stack_push(window, true);
}


static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // First set up the side bar to recalculate width left
    icon_plus = gbitmap_create_with_resource(
            RESOURCE_ID_ACTION_ICON_PLUS_WHITE);
    icon_minus = gbitmap_create_with_resource(
            RESOURCE_ID_ACTION_ICON_MINUS_WHITE);
    side_bar = action_bar_layer_create();
    action_bar_layer_add_to_window(side_bar, window);
    action_bar_layer_set_click_config_provider(side_bar, click_config_provider);

    action_bar_layer_set_icon(side_bar, BUTTON_ID_UP, icon_plus);
    action_bar_layer_set_icon(side_bar, BUTTON_ID_DOWN, icon_minus);

    int width = bounds.size.w - ACTION_BAR_WIDTH;

    // Set up the title text layer
    title_text_layer = text_layer_create((GRect) {
        .origin = { 0, 0 },
        .size = { width, 30 }
    });
    text_layer_set_font(
        title_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    // This default text will be quickly overwritten by the light state. If
    // the bridge settings were never set (virgin installation) no data will 
    // come back and this text will still be displayed to alert the user.
    text_layer_set_text(title_text_layer, "Edit Settings");
    text_layer_set_text_alignment(title_text_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(title_text_layer));

    // Set up the brightness text layer
    brightness_text_layer = text_layer_create((GRect) {
        .origin = { width, ((bounds.size.h/2) - 12)  },
        .size = { 20, 24 }
    });
    text_layer_set_font(
        brightness_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text(brightness_text_layer, "NA");
    text_layer_set_text_alignment(brightness_text_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(brightness_text_layer));

    // Set up the image layer
    lightbulb_bitmap = gbitmap_create_with_resource(RESOURCE_ID_LIGHTBULB);
    lightbulb_bitmap_layer = bitmap_layer_create((GRect) {
        .origin = { ((width - 68) / 2), 30 },
        .size = { 68, 120 }
    });
    bitmap_layer_set_bitmap(lightbulb_bitmap_layer, lightbulb_bitmap);
    layer_add_child(
            window_layer, bitmap_layer_get_layer(lightbulb_bitmap_layer));
}


static void window_unload(Window *window) {
    text_layer_destroy(title_text_layer);
    text_layer_destroy(brightness_text_layer);
    gbitmap_destroy(lightbulb_bitmap);
    bitmap_layer_destroy(lightbulb_bitmap_layer);
    gbitmap_destroy(icon_plus);
    gbitmap_destroy(icon_minus);
    action_bar_layer_destroy(side_bar);
}


static void deinit(void) {
    window_destroy(window);
}


/*******************************************************************************
* Click handler functions
*******************************************************************************/
/**
 * Select button requests the light to be toggled.
 */
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    toggle_light_state();
}


/**
 * Up button increments the brightness level in the GUI and request the same 
 * level to the hue bridge.
 * Not implemented with click_number_of_clicks_counted due to count speed.
 */
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    if ((brightness_level != LIGHT_OFF)) {
        brightness_level++;
        if (brightness_level > MAX_BRIGHTNESS) {
            brightness_level = MAX_BRIGHTNESS;
        }
        gui_update_brightness();
        set_brightness(brightness_level);
    }
}


/**
 * Down button decrements the brightness level in the GUI and request the same 
 * level to the hue bridge.
 * Not implemented with click_number_of_clicks_counted due to count speed.
 */
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    if ((brightness_level != LIGHT_OFF)) {
        brightness_level--;
        if (brightness_level < MIN_BRIGHTNESS) {
            brightness_level = MIN_BRIGHTNESS;
        }
        gui_update_brightness();
        set_brightness(brightness_level);
    }
}


static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
    // Set up the repeating click for UP and DOWN with 200 ms interval
    window_single_repeating_click_subscribe(
            BUTTON_ID_UP, 100, up_click_handler);
    window_single_repeating_click_subscribe(
            BUTTON_ID_DOWN, 100, down_click_handler);
}


/*******************************************************************************
* GUI update functions
*******************************************************************************/
void gui_light_state(light_t on_state) {
    switch (on_state) {
        case LIGHT_STATE_ON:
            text_layer_set_text(title_text_layer, "Light ON");
            // Brightness back to editable, upcoming AppMessage will set value
            brightness_level = 0;
            // Future update the image will change to show a bright light bulb
            break;
        case LIGHT_STATE_OFF:
            text_layer_set_text(title_text_layer, "Light OFF");
            // Set the brightness level uneditable
            brightness_level = LIGHT_OFF;
            gui_update_brightness();
            // Future update the image will change to show a dim light bulb 
            break;
        case LIGHT_STATE_POWER_OFF:
            // Inform the user the light switch is OFF
            text_layer_set_text(title_text_layer, "Power OFF");
            brightness_level = LIGHT_OFF;
        break;
        case LIGHT_STATE_ERROR:
            // No bridge contact, most likely incorrect settings  
            text_layer_set_text(title_text_layer, "Edit Settings");
            brightness_level = LIGHT_OFF;
        default:
            text_layer_set_text(title_text_layer, "Read ERROR");
            brightness_level = LIGHT_OFF;
            APP_LOG(APP_LOG_LEVEL_ERROR, "Unexpected Light state %d",
                    on_state);
            break;
    }
}


void gui_brightness_level(int8_t level) {
    if ((level>=0) && (level<100)) {
        brightness_level = level;
        gui_update_brightness();
    }
}


static void gui_update_brightness() {
    // Set a static buffer for this permanent text
    static char brightness_text[3];
    if (brightness_level == LIGHT_OFF) {
        text_layer_set_text(brightness_text_layer, "NA");
    } else {
        snprintf(brightness_text, sizeof(brightness_text), "%u",
                 brightness_level);
        text_layer_set_text(brightness_text_layer, brightness_text);
    }
}


/*******************************************************************************
* Main
*******************************************************************************/
int main(void) {
    init();
    app_event_loop();
    deinit();
}
