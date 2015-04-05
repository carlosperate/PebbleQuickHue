#include <pebble.h>


/*******************************************************************************
* Local globals
*******************************************************************************/
static Window *window;
static TextLayer *title_text_layer;
static GBitmap *lightbulb_bitmap;
static BitmapLayer *lightbulb_bitmap_layer;


/*******************************************************************************
* Function definitions
*******************************************************************************/
static void select_click_handler(ClickRecognizerRef recognizer, void *context);
static void up_click_handler(ClickRecognizerRef recognizer, void *context);
static void down_click_handler(ClickRecognizerRef recognizer, void *context);
static void click_config_provider(void *context);
static void window_load(Window *window);
static void window_unload(Window *window);
static void deinit(void);


/*******************************************************************************
* Life cycle functions
*******************************************************************************/
static void init(void) {
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
    
    // Set up the text layer
    title_text_layer = text_layer_create((GRect) {
        .origin = { 0, 4 },
        .size = { bounds.size.w, 20 }
    });
    text_layer_set_text(title_text_layer, "Light ON/OFF");
    text_layer_set_text_alignment(title_text_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(title_text_layer));

    // Set up the image layer
    lightbulb_bitmap = gbitmap_create_with_resource(RESOURCE_ID_LIGHTBULB);
    lightbulb_bitmap_layer = bitmap_layer_create((GRect) {
        .origin = { 38, 26 },
        .size = { 68, 120 }
    });
    bitmap_layer_set_bitmap(lightbulb_bitmap_layer, lightbulb_bitmap);
    layer_add_child(
            window_layer, bitmap_layer_get_layer(lightbulb_bitmap_layer));
}


static void window_unload(Window *window) {
    text_layer_destroy(title_text_layer);
}


static void deinit(void) {
    window_destroy(window);
}


/*******************************************************************************
* Click handler functions
*******************************************************************************/
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    text_layer_set_text(title_text_layer, "Light ON/OFF (Select Pressed");
}


static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    text_layer_set_text(title_text_layer, "Light ON/OFF (Up Pressed)");
}


static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    text_layer_set_text(title_text_layer, "Light ON/OFF (Down Pressed)");
}


static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}


/*******************************************************************************
* Main
*******************************************************************************/
int main(void) {
    init();
    app_event_loop();
    deinit();
}
