#include "Arduino.h"
#include "T4_PXP.h"
#include "tft_draw.hpp"
#include <ILI9341_T4.h>

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

// Screen size in landscape mode
#define LX 320
#define LY 240

ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);
ILI9341_T4::DiffBuffStatic<1 << 13> diff1;
ILI9341_T4::DiffBuffStatic<1 << 13> diff2;
uint16_t internal_fb[LX * LY]; // used by the library for buffering (in DMAMEM)
DMAMEM uint16_t fb[LX * LY];   // the final framebuffer we draw onto.
DMAMEM uint16_t mb[LX * LY];   // the main framebuffer we draw onto.
DMAMEM uint16_t gb[LX * LY];   // the graph framebuffer
TFTDraw canvas;
TFTDraw graph;

const int Calibration[] = {399, 3769, 3812, 436};
int tx = 0, ty = 0, tz = 0;

bool init() {
    pinMode(13, OUTPUT);
    Serial.begin(115200);
    tft.output(&Serial);
    tft.setRotation(3);                 // landscape mode 320x240
    tft.setFramebuffer(internal_fb);    // set the internal framebuffer (enables double buffering)
    tft.setDiffBuffers(&diff1, &diff2); // set the 2 diff buffers => activate differential updates.
    tft.setDiffGap(4);                  // use a small gap for the diff buffers (useful because cells are small...)
    tft.setRefreshRate(120);            // Set the refresh rate to around 120Hz
    tft.setVSyncSpacing(1);             // set framerate = refreshrate (we must draw the fast for this to works: ok in our case).
    tft.setTouchCalibration((int *)Calibration);

    memset(fb, 0, LX * LY * sizeof(uint16_t));
    memset(mb, 0, LX * LY * sizeof(uint16_t));
    memset(gb, 0, LX * LY * sizeof(uint16_t));

    canvas.setCanvas(mb, LX, LY);
    graph.setCanvas(gb, LX, LY);
    return tft.begin(SPI_SPEED, 6000000);
}

void update_touch() {
    static char buf[256];
    if (tft.readTouch(tx, ty, tz)) {
        graph.drawFastHLine(tx - 32, ty, 32, ILI9341_T4_COLOR_CYAN);
        graph.drawFastVLine(tx, ty - 32, 32, ILI9341_T4_COLOR_CYAN);
        graph.drawFilledCircle<1, 1>(tx, ty, 5, ILI9341_T4_COLOR_GREEN, ILI9341_T4_COLOR_BLACK);

        sprintf(buf, "(%i, %i, %i)", tx, ty, tz);
        tft.overlayText(mb, buf, 2, 0, 14, ILI9341_T4_COLOR_WHITE, 1.0f, ILI9341_T4_COLOR_BLACK, 0.0f); // draw text
    }
}

int main(void) {
    bool run = init();
    elapsedMillis time_v;

    PXP_init();

    PXP_overlay_buffer(gb, 2, LX, LY);
    PXP_overlay_format(PXP_RGB565, 1, true, 0xB0);
    PXP_overlay_color_key_low(0);
    PXP_overlay_color_key_high(ILI9341_T4_COLOR_SALMON);

    PXP_input_buffer(mb, 2, LX, LY);
    PXP_input_format(PXP_RGB565);

    PXP_output_buffer(fb, 2, LX, LY);
    PXP_output_format(PXP_RGB565);

    while (run) {
        tft.overlayFPS(mb); // draw the FPS counter on the top right corner of the framebuffer
        PXP_process();
        PXP_finish();
        tft.update(fb); // push to screen (asynchronously).
    }

    return 0;
}
