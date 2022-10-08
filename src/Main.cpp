#include "Arduino.h"
#include "Example.h"

int main(void) {
    pinMode(13, OUTPUT);
    Serial.begin(115200);
    Serial.println("Teensy On!");
    delay(1000);
    while (true) {
        delay(500);
        static bool on = true;
        digitalWriteFast(13, (on = !on));
        Serial.printf("Hello %d", example());
    }
    return 0;
}
