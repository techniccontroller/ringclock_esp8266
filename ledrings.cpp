#include "ledrings.h"

LEDRings::LEDRings(Adafruit_NeoPixel *outerRing, Adafruit_NeoPixel *innerRing, UDPLogger *logger){
    this->outerRing = outerRing;
    this->innerRing = innerRing;
    this->logger = logger;
    currentLimit = DEFAULT_CURRENT_LIMIT;
    brightnessOuterRing = 255;
    brightnessInnerRing = 255;
}

/**
 * @brief Convert RGB value to 24bit color value
 * 
 * @param r red value (0-255)
 * @param g green value (0-255)
 * @param b blue value (0-255)
 * @return uint32_t 24bit color value
 */
uint32_t LEDRings::Color24bit(uint8_t r, uint8_t g, uint8_t b){
    return ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
}

/**
 * @brief Input a value 0 to 255 to get a color value. The colors are a transition r - g - b - back to r.
 * 
 * @param WheelPos Value between 0 and 255
 * @return uint32_t return 24bit color of colorwheel
 */
uint32_t LEDRings::Wheel(uint8_t WheelPos) {
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85)
    {
        return Color24bit(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if (WheelPos < 170)
    {
        WheelPos -= 85;
        return Color24bit(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return Color24bit(WheelPos * 3, 255 - WheelPos * 3, 0);
}

/**
 * @brief Interpolates two colors24bit and returns an color of the result
 * 
 * @param color1 startcolor for interpolation
 * @param color2 endcolor for interpolatio
 * @param factor which color is wanted on the path from start to end color
 * @return uint32_t interpolated color
 */
uint32_t LEDRings::interpolateColor24bit(uint32_t color1, uint32_t color2, float factor){
    uint8_t r1 = (color1 >> 16) & 0xFF;
    uint8_t g1 = (color1 >> 8) & 0xFF;
    uint8_t b1 = color1 & 0xFF;

    uint8_t r2 = (color2 >> 16) & 0xFF;
    uint8_t g2 = (color2 >> 8) & 0xFF;
    uint8_t b2 = color2 & 0xFF;

    uint8_t r = r1 + (r2 - r1) * factor;
    uint8_t g = g1 + (g2 - g1) * factor;
    uint8_t b = b1 + (b2 - b1) * factor;

    return Color24bit(r, g, b);
}

/**
 * @brief Setup the LED rings
 * 
*/
void LEDRings::setupRings(){
    outerRing->begin();
    innerRing->begin();
    outerRing->setBrightness(brightnessOuterRing);
    innerRing->setBrightness(brightnessInnerRing);
    outerRing->show();
    innerRing->show();
}

/**
 * @brief Set the offsets of the rings
 * 
 * @param offsetOuterRing offset of the outer ring
 * @param offsetInnerRing offset of the inner ring
 */
void LEDRings::setOffsets(int offsetOuterRing, int offsetInnerRing){
    this->offsetOuterRing = offsetOuterRing;
    this->offsetInnerRing = offsetInnerRing;
}

/**
 * @brief Set the brightness of the outer ring
 * 
 * @param brightness brightness value (0-255)
 */
void LEDRings::setBrightnessOuterRing(uint8_t brightness){
    brightnessOuterRing = brightness;
    outerRing->setBrightness(brightness);
}


/**
 * @brief Set the brightness of the inner ring
 * 
 * @param brightness brightness value (0-255)
 */
void LEDRings::setBrightnessInnerRing(uint8_t brightness){
    brightnessInnerRing = brightness;
    innerRing->setBrightness(brightness);
}

/**
 * @brief Set the current limit of the LED rings
 * 
 * @param currentLimit current limit value (0-9999)
 */
void LEDRings::setCurrentLimit(uint16_t currentLimit){
    this->currentLimit = currentLimit;
}

/**
 * @brief Get the brightness of the outer ring
 * 
 * @return uint8_t brightness value (0-255)
 */
uint8_t LEDRings::getBrightnessOuterRing(){
    return brightnessOuterRing;
}

/**
 * @brief Get the brightness of the inner ring
 * 
 * @return uint8_t brightness value (0-255)
 */
uint8_t LEDRings::getBrightnessInnerRing(){
    return brightnessInnerRing;
}

/**
 * @brief Flush the outer ring
 * 
 */
void LEDRings::flushOuterRing(){
    // set all pixels to black
    for(int i=0; i<OUTER_RING_LED_COUNT; i++) {
        targetOuterring[i] = 0;
    }
}

/**
 * @brief Flush the inner ring
 * 
 */
void LEDRings::flushInnerRing(){
    // set all pixels to black
    for(int i=0; i<INNER_RING_LED_COUNT; i++) {
        targetInnerRing[i] = 0;
    }
}

/**
 * @brief Set the color of a pixel on the outer ring
 * 
 * @param pixel pixel number (0-90)
 * @param color 24bit color value
 */
void LEDRings::setPixelOuterRing(uint16_t pixel, uint32_t color){
    // check if pixel is in range
    if(pixel >= OUTER_RING_LED_COUNT){
        logger->logString("ERROR: pixel out of range");
        return;
    }
    targetOuterring[pixel] = color;
}

/**
 * @brief Set the color of a pixel on the inner ring
 * 
 * @param pixel pixel number (0-11)
 * @param color 24bit color value
 */
void LEDRings::setPixelInnerRing(uint16_t pixel, uint32_t color){
    // check if pixel is in range
    if(pixel >= INNER_RING_LED_COUNT){
        logger->logString("ERROR: pixel out of range");
        return;
    }
    targetInnerRing[pixel] = color;
}

/**
 * @brief Draw the target color on the rings instantly
 * 
 */
void LEDRings::drawOnRingsInstant(){
    drawOnRings(1.0);
}

/**
 * @brief Draw the target color on the rings with a smooth transition
 * 
 * @param factor transition factor (1.0 = instant, 0.1 = smooth)
 */
void LEDRings::drawOnRingsSmooth(float factor){
    drawOnRings(factor);
}

/**
 * @brief Draw the target color on the rings
 * 
 * @param factor transition factor (1.0 = instant, 0.1 = smooth)
 */
void LEDRings::drawOnRings(float factor){
    // calculate current limit
    uint16_t currentLimitOuterRing = currentLimit / OUTER_RING_LED_COUNT;
    uint16_t currentLimitInnerRing = currentLimit / INNER_RING_LED_COUNT;

    // set pixels on outer ring and calculate current for outer ring
    uint16_t totalCurrentOuterRing = 0;
    for(int i=0; i<OUTER_RING_LED_COUNT; i++) {
        uint32_t currentColor = currentOuterRing[i];
        uint32_t targetColor = targetOuterring[i];
        uint32_t newColor = interpolateColor24bit(currentColor, targetColor, factor);
        int correctedPixel = (OUTER_RING_LED_COUNT + i + offsetOuterRing) % OUTER_RING_LED_COUNT;
        outerRing->setPixelColor(correctedPixel, newColor);
        currentOuterRing[i] = newColor;

        totalCurrentOuterRing += calcEstimatedLEDCurrent(newColor, brightnessOuterRing);
    }

    // set pixels on inner ring and calculate current for inner ring
    uint16_t totalCurrentInnerRing = 0;
    for(int i=0; i<INNER_RING_LED_COUNT; i++) {
        uint32_t currentColor = currentInnerRing[i];
        uint32_t targetColor = targetInnerRing[i];
        uint32_t newColor = interpolateColor24bit(currentColor, targetColor, factor);
        int correctedPixel = (INNER_RING_LED_COUNT + i + offsetInnerRing) % INNER_RING_LED_COUNT;
        innerRing->setPixelColor(correctedPixel, newColor);
        currentInnerRing[i] = newColor;

        totalCurrentInnerRing += calcEstimatedLEDCurrent(newColor, brightnessInnerRing);
    }

    uint16_t totalCurrent = totalCurrentOuterRing + totalCurrentInnerRing;

    // Check if totalCurrent reaches CURRENTLIMIT -> if yes reduce brightness
    if(totalCurrent > currentLimit){
        uint8_t newBrightnessOR = brightnessOuterRing * float(currentLimit)/float(totalCurrent);
        uint8_t newBrightnessIR = brightnessInnerRing * float(currentLimit)/float(totalCurrent);
        //logger->logString("CurrentLimit reached!!!: " + String(totalCurrent));
        outerRing->setBrightness(newBrightnessOR);
        innerRing->setBrightness(newBrightnessIR);
    }
    else {
        outerRing->setBrightness(brightnessOuterRing);
        innerRing->setBrightness(brightnessInnerRing);
    }

    // show rings
    outerRing->show();
    innerRing->show();
}


/**
 * @brief Calc estimated current (mA) for one pixel with the given color and brightness
 * 
 * @param color 24bit color value of the pixel for which the current should be calculated
 * @param brightness brightness value (0-255)
 * @return the current in mA
 */
uint16_t LEDRings::calcEstimatedLEDCurrent(uint32_t color, uint8_t brightness){
  // extract rgb values
  uint8_t red = color >> 16 & 0xff;
  uint8_t green = color >> 8 & 0xff;
  uint8_t blue = color & 0xff;
  
  // Linear estimation: 20mA for full brightness per LED 
  // (calculation avoids float numbers)
  uint32_t estimatedCurrent = (20 * red) + (20 * green) + (20 * blue);
  estimatedCurrent /= 255;
  estimatedCurrent = (estimatedCurrent * brightness)/255;

  return estimatedCurrent;
}

