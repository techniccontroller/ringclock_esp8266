/**
 * @file ringclockfunctions.ino
 * 
 * @brief This file contains all functions used to control clock related functions
*/

#include "constants.h"
#define MILLIS_PER_MINUTE 60000


/**
 * @brief Show the hour on the clock
 * 
 * @param hour hour to show
 * @param color color to use
 */
void showHour(uint8_t hour, uint32_t color) {
    // clear matrix
    ledrings.flushInnerRing();

    // convert 24h to 12h
    if(hour > 12) {
        hour -= 12;
    }

    ledrings.setPixelInnerRing(hour-1, color);
}

void showMinutes(uint8_t minutes, uint32_t colorMinutes, uint32_t colorSeconds) {

    static uint8_t lastMinutes = 0;
    static uint32_t timeOfLastMinuteChange = 0;
    static uint32_t black = LEDRings::Color24bit(0, 0, 0);

    // check if minutes changed
    if(minutes != lastMinutes) {
        timeOfLastMinuteChange = millis();
        lastMinutes = minutes;
    }

    // calculate time since last minute change
    uint32_t timeSinceLastMinuteChange = millis() - timeOfLastMinuteChange;

    // calculate seconds progress
    float progressSeconds = (float)timeSinceLastMinuteChange / (float)MILLIS_PER_MINUTE;
    int activePixelSeconds = (int)(progressSeconds * OUTER_RING_LED_COUNT);
    float pixelProgressSeconds = progressSeconds * OUTER_RING_LED_COUNT - activePixelSeconds;

    // calculate minutes progress
    float minutesContinuous = (float)minutes + progressSeconds;
    float progressMinutes = (float)minutesContinuous / 60.0;
    int activePixelMinutes = (int)(progressMinutes * OUTER_RING_LED_COUNT);
    float pixelProgressMinutes = progressMinutes * OUTER_RING_LED_COUNT - activePixelMinutes;

    // clear led ring
    ledrings.flushOuterRing();

    for(int i = 0; i < OUTER_RING_LED_COUNT; i++) {

        if(i == activePixelSeconds - 1){
            uint32_t color = LEDRings::interpolateColor24bit(black, colorSeconds, (1 - pixelProgressSeconds));
            ledrings.setPixelOuterRing(i, color);
        }
        else if(i == activePixelSeconds){
            uint32_t color = LEDRings::interpolateColor24bit(black, colorSeconds, pixelProgressSeconds);
            ledrings.setPixelOuterRing(i, color);
        }
        else {
            if(i < activePixelMinutes) {
                ledrings.setPixelOuterRing(i, colorMinutes);
            }
            else if(i == activePixelMinutes) {
                uint32_t color = LEDRings::interpolateColor24bit(black, colorMinutes, pixelProgressMinutes);
                ledrings.setPixelOuterRing(i, color);
            }
            else {
                ledrings.setPixelOuterRing(i, black);
            }
        }
    }
}

void showTimeOnClock(uint8_t hour, uint8_t minutes, uint32_t colorHours, uint32_t colorMinutes, uint32_t colorSeconds) {
    showHour(hour, colorHours);
    showMinutes(minutes, colorMinutes, colorSeconds);
}