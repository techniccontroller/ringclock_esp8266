#ifndef ledrings_h
#define ledrings_h

#include <Adafruit_NeoPixel.h>          // NeoPixel library used to run the NeoPixel LEDs:
#include "udplogger.h"
#include "constants.h"

#define DEFAULT_CURRENT_LIMIT 9999

class LEDRings{
    public:
        LEDRings(Adafruit_NeoPixel *outerRing, Adafruit_NeoPixel *innerRing, UDPLogger *logger);
        static uint32_t Color24bit(uint8_t r, uint8_t g, uint8_t b);
        static uint32_t Wheel(uint8_t WheelPos);
        static uint32_t interpolateColor24bit(uint32_t color1, uint32_t color2, float factor);

        void setupRings();
        void setOffsets(int offsetOuterRing, int offsetInnerRing);

        void setBrightnessOuterRing(uint8_t brightness);
        void setBrightnessInnerRing(uint8_t brightness);
        void setCurrentLimit(uint16_t currentLimit);

        uint8_t getBrightnessOuterRing();
        uint8_t getBrightnessInnerRing();

        void flushOuterRing();
        void flushInnerRing();

        void setPixelOuterRing(uint16_t pixel, uint32_t color);
        void setPixelInnerRing(uint16_t pixel, uint32_t color);

        void drawOnRingsInstant();
        void drawOnRingsSmooth(float factor);
        

    private:
        Adafruit_NeoPixel *outerRing;
        Adafruit_NeoPixel *innerRing;
        UDPLogger *logger;

        uint16_t currentLimit;
        uint8_t brightnessOuterRing;
        uint8_t brightnessInnerRing;

        int offsetOuterRing;
        int offsetInnerRing;

        uint32_t targetOuterring[OUTER_RING_LED_COUNT] = {0};
        uint32_t currentOuterRing[OUTER_RING_LED_COUNT] = {0};
        uint32_t targetInnerRing[INNER_RING_LED_COUNT] = {0};
        uint32_t currentInnerRing[INNER_RING_LED_COUNT] = {0};

        void drawOnRings(float factor);
        uint16_t calcEstimatedLEDCurrent(uint32_t color, uint8_t brightness);
        
};


#endif