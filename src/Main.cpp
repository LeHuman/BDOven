#include "Arduino.h"
#include "lv_conf.h"
#include <ILI9341_T4.h>
#include <lvgl.h>

#define PIN_SCK 13        // mandatory
#define PIN_MISO 12       // mandatory
#define PIN_MOSI 11       // mandatory
#define PIN_DC 10         // mandatory, can be any pin but using pin 10 (or 36 or 37 on T4.1) provides greater performance
#define PIN_CS 9          // mandatory when using the touchscreen on the same SPI bus, can be any pin.
#define PIN_RESET 14      // optional (but recommended), can be any pin.
#define PIN_BACKLIGHT 255 // optional. Set this only if the screen LED pin is connected directly to the Teensy
#define PIN_TOUCH_CS 8    // mandatory, can be any pin
#define PIN_TOUCH_IRQ 7   // (optional) can be any digital pin with interrupt capabilities

#define SPI_SPEED 80000000

static const uint32_t ScreenWidth = 320;
static const uint32_t ScreenHeight = 240;
const int Calibration[] = {399, 3769, 3812, 436};

ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);
ILI9341_T4::DiffBuffStatic<1 << 13> diff1;
ILI9341_T4::DiffBuffStatic<1 << 13> diff2;
DMAMEM uint16_t internal_fb[ScreenWidth * ScreenHeight]; // used by the library for buffering (in DMAMEM)

DMAMEM lv_color_t lvgl_buf[ScreenWidth * ScreenHeight]; // memory for lvgl draw buffer (25KB)
lv_disp_draw_buf_t draw_buf;                            // lvgl 'draw buffer' object
lv_disp_drv_t disp_drv;                                 // lvgl 'display driver'
lv_indev_drv_t indev_drv;                               // lvgl 'input device driver'
lv_disp_t *disp;                                        // pointer to lvgl display object

/** Callback to draw on the screen */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    const bool redraw_now = lv_disp_flush_is_last(disp);                                       // check if when should update the screen (or just buffer the changes).
    tft.updateRegion(redraw_now, (uint16_t *)color_p, area->x1, area->x2, area->y1, area->y2); // update the interval framebuffer and then redraw the screen if requested
    lv_disp_flush_ready(disp);                                                                 // tell lvgl that we are done and that the lvgl draw buffer can be reused immediately
}

/** Call back to read the touchpad */
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    int touchX, touchY, touchZ;
    bool touched = tft.readTouch(touchX, touchY, touchZ); // read the touchpad
    if (!touched) {                                       // nothing
        data->state = LV_INDEV_STATE_REL;
    } else { // pressed
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touchX;
        data->point.y = touchY;
    }
}

// event callback
void ta_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t *kb = (lv_obj_t *)lv_event_get_user_data(e);
    if (code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
    if (code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}

int main(void) {
    Serial.begin(115200);
    tft.begin(SPI_SPEED, 6000000);
    tft.setRotation(3);                 // landscape mode 320x240
    tft.setFramebuffer(internal_fb);    // set the internal framebuffer (enables double buffering)
    tft.setDiffBuffers(&diff1, &diff2); // set the 2 diff buffers => activate differential updates.
    tft.setDiffGap(1);                  // use a small gap for the diff buffers (useful because cells are small...)
    tft.setRefreshRate(120);            // Set the refresh rate to around 120Hz
    tft.setVSyncSpacing(1);             // set framerate = refreshrate (we must draw the fast for this to works: ok in our case).
    tft.setTouchCalibration((int *)Calibration);
    lv_init();

    // // initialize lvgl drawing buffer
    lv_disp_draw_buf_init(&draw_buf, lvgl_buf, nullptr, ScreenWidth * ScreenHeight);

    // // Initialize lvgl display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = ScreenWidth;
    disp_drv.ver_res = ScreenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    disp = lv_disp_drv_register(&disp_drv);
    disp->refr_timer->period = 11; // set refresh rate around 66FPS.

    // // Initialize lvgl input device driver (the touch screen)
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    lv_obj_t *kb = lv_keyboard_create(lv_scr_act());
    lv_obj_t *ta = lv_textarea_create(lv_scr_act());
    lv_obj_align(ta, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_event_cb(ta, ta_event_cb, LV_EVENT_ALL, kb);
    lv_textarea_set_placeholder_text(ta, "Hello");
    lv_obj_set_size(ta, 220, 100);

    // pinMode(15, OUTPUT);
    while (true) {
        // static bool on = false;
        // digitalWriteFast(15, (on = !on));
        // delay(100);
        lv_task_handler();
    }

    return 0;
}
