#include "Arduino.h"
#include "T4_PXP.h"
#include "tft_draw.hpp"
#include <ILI9341_T4.h>

//
// DEFAULT WIRING USING SPI 0 ON TEENSY 4/4.1
//
#define PIN_SCK 13        // mandatory
#define PIN_MISO 12       // mandatory
#define PIN_MOSI 11       // mandatory
#define PIN_DC 10         // mandatory, can be any pin but using pin 10 (or 36 or 37 on T4.1) provides greater performance
#define PIN_CS 9          // mandatory when using the touchscreen on the same SPI bus, can be any pin.
#define PIN_RESET 14      // optional (but recommended), can be any pin.
#define PIN_BACKLIGHT 255 // optional. Set this only if the screen LED pin is connected directly to the Teensy

#define PIN_TOUCH_CS 8  // mandatory, can be any pin
#define PIN_TOUCH_IRQ 7 // (optional) can be any digital pin with interrupt capabilities

#define SPI_SPEED 80000000

#define IDLE_TOUCH_FPS 127 // Idle ms frame time for touch

// Screen size in landscape mode
#define LX 320
#define LY 240

// the screen driver object
ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);
TFTDraw canvas;
TFTDraw graph;
ILI9341_T4::DiffBuffStatic<1 << 13> diff1;
ILI9341_T4::DiffBuffStatic<1 << 13> diff2;

uint16_t internal_fb[LX * LY]; // used by the library for buffering (in DMAMEM)
DMAMEM uint16_t fb[LX * LY];   // the final framebuffer we draw onto.
DMAMEM uint16_t mb[LX * LY];   // the main framebuffer we draw onto.
DMAMEM uint16_t gb[LX * LY];   // the graph framebuffer

// size of a cell in pixels
#define CELL_LX 1 // possible value: 1, 2, 4, 8
#define CELL_LY 1 // possible values: 1, 2, 4, 8
// size of the 'world'
#define WORLD_LX (LX / CELL_LX)
#define WORLD_LY (LY / CELL_LY)
// two world buffers
uint8_t worldA[WORLD_LX * WORLD_LY];
uint8_t worldB[WORLD_LX * WORLD_LY];
// pointers to the 'current' and the 'previous' world
uint8_t *oldworld = worldA;
uint8_t *curworld = worldB;
// swap the two worlds pointers.
void swap_worlds() {
    auto tmp = oldworld;
    oldworld = curworld;
    curworld = tmp;
}
// create a random initial world with given density in [0.0f,1.0f]
void random_world(uint8_t *world, float density) {
    memset(world, 0, WORLD_LX * WORLD_LY);
    const int thr = density * 1024;
    for (int j = 1; j < WORLD_LY - 1; j++)
        for (int i = 1; i < WORLD_LX - 1; i++)
            world[i + (j * WORLD_LX)] = (random(0, 1024) < thr) ? 1 : 0;
}
// draw a world on the framebuffer
void draw_world(uint8_t *world, uint16_t color, uint16_t bkcolor) {
    for (int j = 0; j < WORLD_LY; j++)
        for (int i = 0; i < WORLD_LX; i++)
            canvas.drawRect(i * CELL_LX, j * CELL_LY, CELL_LX, CELL_LY, world[(j * WORLD_LX) + i] ? color : bkcolor); // draw_rect
}
// number of neighbours of a cell
inline int nb_neighbours(int x, int y, uint8_t *world) {
    const int ind = x + (WORLD_LX * y);
    return world[ind - WORLD_LX - 1] + world[ind - WORLD_LX] + world[ind - WORLD_LX + 1] +
           world[ind - 1] + world[ind + 1] +
           world[ind + WORLD_LX - 1] + world[ind + WORLD_LX] + world[ind + WORLD_LX + 1];
}
const int HASH_LIST_SIZE = 128;            // max number of previous hash to store
static uint32_t hash_list[HASH_LIST_SIZE]; // array of previou hashes
int hash_nr = 0;                           // number of hashed currently stored
// check if the current hash appears multiple times among the
// previous ones. If so we have reached a periodic config so
// and recreate a anew world.
void check_world(uint32_t hash) {
    uint16_t matches = 0;
    for (uint16_t i = 0; i < hash_nr; i++) {
        if (hash_list[i] == hash)
            matches++;
    }
    if (matches >= 2) {
        hash_nr = 0;
        random_world(oldworld, random(1, 1024) / 1024.0f);
        return;
    }
    if (hash_nr == HASH_LIST_SIZE)
        hash_nr = 0;
    hash_list[hash_nr++] = hash;
}
// compute the world at the next generation.
uint32_t compute(uint8_t *cur_world, uint8_t *new_world) {
    uint32_t hash = 0;
    for (int j = 1; j < WORLD_LY - 1; j++) {
        for (int i = 1; i < WORLD_LX - 1; i++) {
            const int ind = i + (WORLD_LX * j);
            const int nbn = nb_neighbours(i, j, cur_world);
            hash += nbn * (j * 3 + i * 5);
            if (nbn == 3)
                new_world[ind] = 1;
            else if ((nbn < 2) || (nbn > 3))
                new_world[ind] = 0;
            else
                new_world[ind] = cur_world[ind];
        }
    }
    return hash;
}

void printPXP() {
    Serial.printf("CTLR: %lx \n", PXP_CTRL);
    Serial.printf("STAT: %lx \n", PXP_STAT);
    Serial.printf("OUT_CTLR: %lx \n", PXP_OUT_CTRL);
    Serial.printf("OUT_PITCH: %lu \n", PXP_OUT_PITCH);
    Serial.printf("OUT_LRC: %lx \n", PXP_OUT_LRC);
    Serial.printf("PS_CTLR: %lx \n", PXP_PS_CTRL);
    Serial.printf("PS_PITCH: %lu \n", PXP_PS_PITCH);
    Serial.printf("PS_BG: %lx \n", PXP_PS_BACKGROUND_0);
    Serial.printf("PS_SCALE: %lx \n", PXP_PS_SCALE);
    Serial.printf("PS_OFFSET: %lx \n", PXP_PS_OFFSET);
    Serial.printf("PS_ULC: %lx \n", PXP_OUT_PS_ULC);
    Serial.printf("PS_LRC: %lx \n", PXP_OUT_PS_LRC);
    Serial.printf("AS_CTLR: %lx \n", PXP_AS_CTRL);
    Serial.printf("AS_PITCH: %lu \n", PXP_AS_PITCH);
    Serial.printf("AS_ULC: %lx \n", PXP_OUT_AS_ULC);
    Serial.printf("AS_LRC: %lx \n", PXP_OUT_AS_LRC);
    Serial.printf("NEXT: %lx \n", PXP_NEXT);

    Serial.printf("\n\n");
    Serial.send_now();
}

const int Calibration[] = {399, 3769, 3812, 436};

int main(void) {
    pinMode(13, OUTPUT);
    Serial.begin(115200);
    Serial.println("Teensy On!");
    tft.output(&Serial);
    bool run = tft.begin(SPI_SPEED, 6000000);
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

    random_world(curworld, 0.7f);
    static int nbf = 0;
    int x, y, z;
    char buf[256];
    elapsedMillis time_v;

    graph.drawFastHLine(32, 32, 64, ILI9341_T4_COLOR_SALMON);
    graph.drawFastHLine(32, 31, 64, ILI9341_T4_COLOR_SALMON);
    graph.drawFastHLine(32, 30, 64, ILI9341_T4_COLOR_SALMON);

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
        PXP_process();
        uint32_t hash = compute(curworld, oldworld);                        // compute the next generation
        check_world(hash);                                                  // check if we should recreate a new world
        swap_worlds();                                                      // swap between old and new worlds
        draw_world(curworld, ILI9341_T4_COLOR_RED, ILI9341_T4_COLOR_BLACK); // draw onto the framebuffer

        tft.overlayFPS(mb); // draw the FPS counter on the top right corner of the framebuffer
        if (tft.readTouch(x, y, z)) {
            graph.drawFastHLine(x - 32, y, 64, ILI9341_T4_COLOR_CYAN);
            graph.drawFastVLine(x, y - 32, 64, ILI9341_T4_COLOR_CYAN);
            graph.drawFilledCircle<1, 1>(x, y, 5, ILI9341_T4_COLOR_GREEN, ILI9341_T4_COLOR_BLACK);

            curworld[x + (WORLD_LX * y) - 1] = !curworld[x + (WORLD_LX * y) - 1];
            curworld[x + (WORLD_LX * y) + 1] = !curworld[x + (WORLD_LX * y) + 1];
            curworld[x + (WORLD_LX * y) - WORLD_LX] = !curworld[x + (WORLD_LX * y) - WORLD_LX];
            curworld[x + (WORLD_LX * y) + WORLD_LX] = !curworld[x + (WORLD_LX * y) + WORLD_LX];

            sprintf(buf, "(%i, %i, %i)", x, y, z);
            tft.overlayText(mb, buf, 2, 0, 14, ILI9341_T4_COLOR_WHITE, 1.0f, ILI9341_T4_COLOR_BLACK, 0.0f); // draw text
        }
        PXP_finish();
        tft.update(fb); // push to screen (asynchronously).

        if (++nbf % 1000 == 500) { // output some stats on Serial every 1000 frames.
            tft.printStats();      // stats about the driver
            diff1.printStats();    // stats about the first diff buffer
            diff2.printStats();    // stats about the second diff buffer
        }
    }

    return 0;
}
