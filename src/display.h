#include "lv_conf.h"
#include <AnimatedGIF.h>
#include <ILI9341_T4.h>
#include <lvgl.h>

#include "splash.h"

#pragma once

#define PIN_SCK 13        // mandatory
#define PIN_MISO 12       // mandatory
#define PIN_MOSI 11       // mandatory
#define PIN_DC 10         // mandatory, can be any pin but using pin 10 (or 36 or 37 on T4.1) provides greater performance
#define PIN_CS 9          // mandatory when using the touchscreen on the same SPI bus, can be any pin.
#define PIN_RESET 14      // optional (but recommended), can be any pin.
#define PIN_BACKLIGHT 255 // optional. Set this only if the screen LED pin is connected directly to the Teensy
#define PIN_TOUCH_CS 8    // mandatory, can be any pin
#define PIN_TOUCH_IRQ 7   // (optional) can be any digital pin with interrupt capabilities

// #define SPI_SPEED 80000000
#define SPI_SPEED 75000000
#define TRGT_FPS 120

namespace Display {

static const uint32_t Width = 320;
static const uint32_t Height = 240;
static const uint32_t mspf = 1.0 / (TRGT_FPS / 1000.0);
static const uint32_t fps = 1.0 / mspf * 1000.0;
// const int Calibration[] = {399, 3769, 3812, 436};
const int Calibration[] = {390, 3750, 3600, 800};

ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);
ILI9341_T4::DiffBuffStatic<1 << 13> diff1;
ILI9341_T4::DiffBuffStatic<1 << 13> diff2;
DMAMEM uint16_t internal_fb[Width * Height]; // used by the library for buffering (in DMAMEM)

DMAMEM lv_color_t lvgl_buf[Width * Height]; // memory for lvgl draw buffer (25KB)
lv_disp_draw_buf_t draw_buf;                // lvgl 'draw buffer' object
lv_disp_drv_t disp_drv;                     // lvgl 'display driver'
lv_indev_drv_t indev_drv;                   // lvgl 'input device driver'
lv_disp_t *disp;                            // pointer to lvgl display object

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

void *GIFAlloc(uint32_t u32Size) {
    return malloc(u32Size);
}

void GIFFree(void *p) {
    free(p);
}

void GIFDraw(GIFDRAW *pDraw) {
    memcpy((lvgl_buf + pDraw->iX + ((pDraw->iY + pDraw->y) * Width)), pDraw->pPixels, sizeof(uint16_t) * Width);
}

void splashPlay(uint32_t max_fps, u_int32_t hold_ms) {
    AnimatedGIF gif;
    gif.begin();
    if (gif.open((uint8_t *)Splash_map, sizeof(Splash_map), Display::GIFDraw)) {
        if (gif.allocFrameBuf(GIFAlloc) == GIF_SUCCESS) {
            gif.setDrawType(GIF_DRAW_COOKED);

            elapsedMicros em;
            max_fps = 1000000 / max_fps;

            while (gif.playFrame(false, NULL)) {
                tft.update((const uint16_t *)lvgl_buf);
                while (em < max_fps) {
                }
                em = 0;
            }

            gif.freeFrameBuf(GIFFree);
            while (em < hold_ms * 1000) {
            }
        }
        gif.close();
    }
}

void init() {
    tft.begin(SPI_SPEED, 6000000);
    tft.setRotation(1);                 // landscape mode 320x240
    tft.setFramebuffer(internal_fb);    // set the internal framebuffer (enables double buffering)
    tft.setDiffBuffers(&diff1, &diff2); // set the 2 diff buffers => activate differential updates.
    tft.setDiffGap(4);                  // use a small gap for the diff buffers (useful because cells are small...)
    tft.setRefreshRate(30);             // Set the refresh rate to around 120Hz
    tft.setVSyncSpacing(1);             // set framerate = refreshrate (we must draw the fast for this to works: ok in our case).
    tft.setTouchCalibration((int *)Calibration);

    splashPlay(30, 1000);
    tft.setRefreshRate(fps);

    lv_init();

    // // initialize lvgl drawing buffer
    lv_disp_draw_buf_init(&draw_buf, lvgl_buf, nullptr, Width * Height);

    // // Initialize lvgl display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = Width;
    disp_drv.ver_res = Height;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    disp = lv_disp_drv_register(&disp_drv);
    disp->refr_timer->period = mspf; // set refresh rate around 66FPS.

    // // Initialize lvgl input device driver (the touch screen)
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
}

} // namespace Display