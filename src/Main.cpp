#include "Arduino.h"
#include "lv_conf.h"
#include "spline.h"
#include <ILI9341_T4.h>
#include <lvgl.h>
#include <vector>

#include "graph.h"
#include "reflow.h"

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

    Graph::Graph g = Graph::Graph();

    g.setSize(230, 150);
    g.setPos(0, 0);
    g.setTitle("Amogus");

    // lv_coord_t Xs[Reflow::REFLOW_PROFILE_CHIPQUIK_SNAGCU.len];
    // lv_coord_t Ys[Reflow::REFLOW_PROFILE_CHIPQUIK_SNAGCU.len];

    // for (int i = 0; i < Reflow::REFLOW_PROFILE_CHIPQUIK_SNAGCU.len; i++) {
    //     Xs[i] = Reflow::REFLOW_PROFILE_CHIPQUIK_SNAGCU.timing[i][1];
    //     Ys[i] = Reflow::REFLOW_PROFILE_CHIPQUIK_SNAGCU.timing[i][2];
    // }

    // g.setMainData(Xs, Ys, Reflow::REFLOW_PROFILE_CHIPQUIK_SNAGCU.len);

    std::vector<double> Xs = {
        0,
        90,
        180,
        210,
        240,
        270,
        281,
    };

    std::vector<double> Ys = {
        25,
        150,
        175,
        217,
        249,
        217,
        175,
    };

    g.setMainData(Xs, Ys);

    tk::spline spliner(Xs, Ys, tk::spline::cspline);
    double time = 0;
    long rnd = 0;

    elapsedMillis ems;

    while (true) {
        if (ems > 5) {
            g.updateData(time, max(spliner(time) + (rnd += random(-8, 9)), 0));
            time += 0.125;
            ems = 0;
            if (time > Xs.back())
                time = random(283);
        }
        lv_task_handler();
    }

    return 0;
}
