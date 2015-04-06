#include <pebble.h>
#include "main.h"
#include "hue_control.h"


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

static unsigned char brightness_level = 0;


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
    // Window
    window = window_create();
    window_set_click_config_provider(window, click_config_provider);
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
     });
    window_stack_push(window, true);

    // Register AppMessage handlers
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);
    
    // Open AppMessage
    // TODO: Once appmessage code is finish determine maximum size required
    app_message_open(app_message_inbox_size_maximum(),
                     app_message_outbox_size_maximum());
}


static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    
    // First set up the side bar to recalculate width left
    icon_plus = gbitmap_create_with_resource(RESOURCE_ID_ACTION_ICON_PLUS_WHITE);
    icon_minus = gbitmap_create_with_resource(RESOURCE_ID_ACTION_ICON_MINUS_WHITE);
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
    text_layer_set_text(title_text_layer, "Light ON/OFF");
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
        .origin = { ((width - 68) / 2), 32 },
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
* Tick / Click handler functions
*******************************************************************************/
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    toggle_light_state();
}


static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (brightness_level < 99) {
        brightness_level++;
        gui_update_brightness();
        set_brightness(brightness_level);
    }
}


static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (brightness_level > 0) {
        brightness_level--;
        gui_update_brightness();
        set_brightness(brightness_level);
    }
}


static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
    // Set up the repeating click for UP and DOWN with 50ms interval
    window_single_repeating_click_subscribe(BUTTON_ID_UP, 50, up_click_handler);
    window_single_repeating_click_subscribe(
            BUTTON_ID_DOWN, 50, down_click_handler);
}

void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  //Allocate long-lived storage (required by TextLayer)
  //static char buffer[] = "00:00";
  
  //Write the time to the buffer in a safe manner
  //strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  
  //Set the TextLayer to display the buffer
  //text_layer_set_text(g_text_layer, buffer);
}


/*******************************************************************************
* GUI update functions
*******************************************************************************/
void gui_light_state(bool on_state) {
    if (on_state == true) {
        text_layer_set_text(title_text_layer, "Light ON");
        // In a future update the image will change to show a bright light bulb
    } else {
        text_layer_set_text(title_text_layer, "Light OFF");
        // In a future update the image will change to show a dimm light bulb
    }
}

void gui_brightness_level(signed char level) {
    if ((level>=0) && (level<100)) {
        brightness_level = level;
        gui_update_brightness();
    }
}

static void gui_update_brightness() {
    // Set a static buffer for this permanent text
    static char brightness_text[4];
    snprintf(brightness_text, sizeof(brightness_text), "%u", brightness_level);
    text_layer_set_text(brightness_text_layer, brightness_text);
}


/*******************************************************************************
* Main
*******************************************************************************/
int main(void) {
    init();
    app_event_loop();
    deinit();
}
