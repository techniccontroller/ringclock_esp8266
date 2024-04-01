/**
 * Ringclock - Clock based on NeoPixel with ESP8266 and NTP time update
 * 
 * created by techniccontroller 06.01.2024
 * 
 * components:
 * - ESP8266 (Wemos D1 mini)
 * - 91x NeoPixel WS2812B LED-strip 144/m
 * - 12x NeoPixel WS2812B LED-strip 30/m
 * 
 * Board settings:
 * - Board: LOLIN(WEMOS) D1 mini (clone)
 * - Flash Size: 4MB (FS:2MB OTA:~1019KB)
 * - Upload Speed: 115200
 * 
 * Libraries:
 * - Adafruit NeoPixel
 * - Adafruit GFX
 * - ArduinoOTA
 * - ESP8266WiFi
 * - WiFiManager
*/
#include <LittleFS.h>
#include <Adafruit_NeoPixel.h>          // NeoPixel library used to run the NeoPixel LEDs: https://github.com/adafruit/Adafruit_NeoPixel
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>                //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <EEPROM.h>                     //from ESP8266 Arduino Core (automatically installed when ESP8266 was installed via Boardmanager)

// own libraries
#include "constants.h"
#include "udplogger.h"
#include "ntp_client_plus.h"
#include "ledrings.h"
#include "Arduino.h"

#define DELAYVAL 100

// ports
const unsigned int HTTPPort = 80;
const unsigned int logMulticastPort = 8123;

// ip addresses for multicast logging
IPAddress logMulticastIP = IPAddress(230, 120, 10, 2);

// ip addresses for Access Point
IPAddress IPAdress_AccessPoint(192,168,10,2);
IPAddress Gateway_AccessPoint(192,168,10,0);
IPAddress Subnetmask_AccessPoint(255,255,255,0);

// SSID for Access Point
const char* AP_SSID = "ringclockAP";

// hostname
const String hostname = "ringclock";

// ----------------------------------------------------------------------------------
//                                        GLOBAL VARIABLES
// ----------------------------------------------------------------------------------

// Webserver
ESP8266WebServer server(HTTPPort);

// Wifi manager
WiFiManager wifiManager;

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel outer_ring(OUTER_RING_LED_COUNT, OUTER_RING_LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel inner_ring(INNER_RING_LED_COUNT, INNER_RING_LED_PIN, NEO_GRB + NEO_KHZ800);

// timestamp variables
long lastheartbeat = millis();      // time of last heartbeat sending
long lastStep = millis();           // time of last clock update
long lastLedStep = millis();  // time of last led update
long lastNTPUpdate = millis() - (PERIOD_NTP_UPDATE-5000);  // time of last NTP update
long lastNightmodeCheck = millis(); // time of last nightmode check

// Create necessary global objects
UDPLogger logger;
WiFiUDP NTPUDP;
NTPClientPlus ntp = NTPClientPlus(NTPUDP, "pool.ntp.org", 1, true);
LEDRings ledrings = LEDRings(&outer_ring, &inner_ring, &logger);

// colors
uint32_t colorHours = LEDRings::Color24bit(200, 200, 0);    // color of the hours
uint32_t colorMinutes = LEDRings::Color24bit(0, 200, 200);  // color of the minutes
uint32_t colorSeconds = LEDRings::Color24bit(200, 0, 200);  // color of the seconds

bool nightMode = false;                       // stores state of nightmode
bool ledOff = false;                          // stores state of LED off
// nightmode settings
uint8_t nightModeStartHour = 22;
uint8_t nightModeStartMin = 0;
uint8_t nightModeEndHour = 7;
uint8_t nightModeEndMin = 0;

// Watchdog counter to trigger restart if NTP update was not possible 30 times in a row (5min)
int watchdogCounter = 30;

// ----------------------------------------------------------------------------------
//                                       SETUP 
// ----------------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println();
  Serial.printf("\nSketchname: %s\nBuild: %s\n", (__FILE__), (__TIMESTAMP__));
  Serial.println();

  //Init EEPROM
  EEPROM.begin(EEPROM_SIZE);

  

  ledrings.setupRings();
  ledrings.setCurrentLimit(CURRENT_LIMIT_LED);
  ledrings.setOffsets(-5, 0);

  loadColorsFromEEPROM();
  loadNightmodeSettingsFromEEPROM();
  loadBrightnessSettingsFromEEPROM();

  /** Use WiFiMaanger for handling initial Wifi setup **/

  // Local intialization. Once its business is done, there is no need to keep it around


  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();

  // set custom ip for portal
  //wifiManager.setAPStaticIPConfig(IPAdress_AccessPoint, Gateway_AccessPoint, Subnetmask_AccessPoint);

  // set a custom hostname
  wifiManager.setHostname(hostname);

  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name
  // here "ringclockAP"
  // and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect(AP_SSID);

  // if you get here you have connected to the WiFi
  Serial.println("Connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // init ESP8266 File manager (LittleFS)
  setupFS();

  setupOTA(hostname); 

  server.on("/data", handleDataRequest); // process datarequests
  server.on("/cmd", handleCommand); // process commands
  server.begin();

  // create UDP Logger to send logging messages via UDP multicast
  logger = UDPLogger(WiFi.localIP(), logMulticastIP, logMulticastPort);
  logger.setName("Ringclock");
  logger.logString("Start program\n");
  logger.logString("Sketchname: "+ String(__FILE__));
  logger.logString("Build: " + String(__TIMESTAMP__));
  logger.logString("IP: " + WiFi.localIP().toString());
  logger.logString("Reset Reason: " + ESP.getResetReason());

  if(!ESP.getResetReason().equals("Software/System restart")){
    runQuickLEDTest();

    // display IP
    uint8_t address = WiFi.localIP()[3];
    uint8_t first_digit = address/100;
    uint8_t second_digit = (address/10)%10;
    uint8_t third_digit = address%10;

    // if a digit is 0 -> show 10
    if(first_digit == 0) {
      first_digit = 10;
    }
    if(second_digit == 0) {
      second_digit = 10;
    }
    if(third_digit == 0) {
      third_digit = 10;
    }

    showHour(first_digit, colorHours);
    ledrings.drawOnRingsInstant();
    delay(2000);
    showHour(second_digit, colorHours);
    ledrings.drawOnRingsInstant();
    delay(2000);
    showHour(third_digit, colorHours);
    ledrings.drawOnRingsInstant();
    delay(2000);

    // clear matrix
    ledrings.flushInnerRing();
    ledrings.flushOuterRing();
    ledrings.drawOnRingsInstant();
  }

  delay(2000);

  // setup NTP
  ntp.setupNTPClient();
  logger.logString("NTP running");
  logger.logString("Time: " +  ntp.getFormattedTime());
  logger.logString("TimeOffset (seconds): " + String(ntp.getTimeOffset()));
}

// ----------------------------------------------------------------------------------
//                                       LOOP
// ----------------------------------------------------------------------------------
void loop() {

  // handle OTA
  handleOTA();

  // handle Webserver
  server.handleClient();

  // send regularly heartbeat messages via UDP multicast
  if(millis() - lastheartbeat > PERIOD_HEARTBEAT){
    logger.logString("Heartbeat, FreeHeap: " + String(ESP.getFreeHeap()) + ", HeapFrag: " + ESP.getHeapFragmentation() + ", MaxFreeBlock: " + ESP.getMaxFreeBlockSize() + "\n");
    lastheartbeat = millis();
  }

  if((millis() - lastStep > PERIOD_CLOCK_UPDATE) && !nightMode && !ledOff){
    // update LEDs
    int hours = ntp.getHours24();
    int minutes = ntp.getMinutes();
    showTimeOnClock(hours, minutes, colorHours, colorMinutes, colorSeconds);
    lastStep = millis();
  }

  // Turn off LEDs if ledOff is true or nightmode is active
  if(ledOff || nightMode){
    ledrings.flushInnerRing();
    ledrings.flushOuterRing();
    ledrings.drawOnRingsInstant();
  }
  // periodically write colors to leds
  else if(millis() - lastLedStep > PERIOD_LED_UPDATE){
    ledrings.drawOnRingsSmooth(1.0);
    lastLedStep = millis();
  }

  // NTP time update
  if(millis() - lastNTPUpdate > PERIOD_NTP_UPDATE){
    int res = ntp.updateNTP();
    if(res == 0){
      ntp.calcDate();
      logger.logString("NTP-Update successful");
      logger.logString("Time: " +  ntp.getFormattedTime());
      logger.logString("Date: " +  ntp.getFormattedDate());
      logger.logString("Day of Week (Mon=1, Sun=7): " +  String(ntp.getDayOfWeek()));
      logger.logString("TimeOffset (seconds): " + String(ntp.getTimeOffset()));
      logger.logString("Summertime: " + String(ntp.updateSWChange()));
      lastNTPUpdate = millis();
      watchdogCounter = 30;
    }
    else if(res == -1){
      logger.logString("NTP-Update not successful. Reason: Timeout");
      lastNTPUpdate += 10000;
      watchdogCounter--;
    }
    else if(res == 1){
      logger.logString("NTP-Update not successful. Reason: Too large time difference");
      logger.logString("Time: " +  ntp.getFormattedTime());
      logger.logString("Date: " +  ntp.getFormattedDate());
      logger.logString("Day of Week (Mon=1, Sun=7): " +  ntp.getDayOfWeek());
      logger.logString("TimeOffset (seconds): " + String(ntp.getTimeOffset()));
      logger.logString("Summertime: " + String(ntp.updateSWChange()));
      lastNTPUpdate += 10000;
      watchdogCounter--;
    }
    else {
      logger.logString("NTP-Update not successful. Reason: NTP time not valid (<1970)");
      lastNTPUpdate += 10000;
      watchdogCounter--;
    }

    logger.logString("Watchdog Counter: " + String(watchdogCounter));
    if(watchdogCounter <= 0){
        logger.logString("Trigger restart due to watchdog...");
        delay(100);
        ESP.restart();
    }
    
  }

  // check if nightmode need to be activated
  if(millis() - lastNightmodeCheck > PERIOD_NIGHTMODE_CHECK){
    int hours = ntp.getHours24();
    int minutes = ntp.getMinutes();

    nightMode = false; // Initial assumption

    // Convert all times to minutes for easier comparison
    int currentTimeInMinutes = hours * 60 + minutes;
    int startInMinutes = nightModeStartHour * 60 + nightModeStartMin;
    int endInMinutes = nightModeEndHour * 60 + nightModeEndMin;

    if (startInMinutes < endInMinutes) { // Same day scenario
        if (startInMinutes < currentTimeInMinutes && currentTimeInMinutes < endInMinutes) {
            nightMode = true;
        }
    } else if (startInMinutes > endInMinutes) { // Overnight scenario
        if (currentTimeInMinutes > startInMinutes || currentTimeInMinutes < endInMinutes) {
            nightMode = true;
        }
    } 
    
    lastNightmodeCheck = millis();
  }

}

// ----------------------------------------------------------------------------------
//                                       FUNCTIONS
// ----------------------------------------------------------------------------------

/**
 * @brief Run a quick LED test
*/
void runQuickLEDTest(){
  // test quickly each LED
  for(int i=0; i<inner_ring.numPixels(); i++) {
    inner_ring.setPixelColor(i, inner_ring.Color(0, 0, 0));
    inner_ring.setPixelColor((i+1)%inner_ring.numPixels(), inner_ring.Color(0, 255, 0));
    inner_ring.show();
    delay(100);
  }

  for(int i=0; i<outer_ring.numPixels(); i++) {
    outer_ring.setPixelColor(i, outer_ring.Color(0, 0, 0));
    outer_ring.setPixelColor((i+1)%outer_ring.numPixels(), outer_ring.Color(0, 0, 255));
    outer_ring.show();
    delay(10);
  }

  // clear all LEDs
  inner_ring.clear();
  outer_ring.clear();
  inner_ring.show();
  outer_ring.show();
  delay(200);
}

/**
 * @brief Load the colors for hours, minutes and seconds from EEPROM
*/
void loadColorsFromEEPROM(){
  colorHours = getRGBColorFromEEPROM(ADR_CH_RED, ADR_CH_GREEN, ADR_CH_BLUE);
  colorMinutes = getRGBColorFromEEPROM(ADR_CM_RED, ADR_CM_GREEN, ADR_CM_BLUE);
  colorSeconds = getRGBColorFromEEPROM(ADR_CS_RED, ADR_CS_GREEN, ADR_CS_BLUE);
}

/**
 * @brief Load a color from EEPROM
 * 
 * @param adr_red address of red value in EEPROM
 * @param adr_green address of green value in EEPROM
 * @param adr_blue address of blue value in EEPROM
 * 
 * @return color as 24bit integer
*/
uint32_t getRGBColorFromEEPROM(uint8_t adr_red, uint8_t adr_green, uint8_t adr_blue){
  uint8_t red = EEPROM.read(adr_red);
  uint8_t green = EEPROM.read(adr_green);
  uint8_t blue = EEPROM.read(adr_blue);
  if(int(red) + int(green) + int(blue) < 50){
    return LEDRings::Color24bit(200, 200, 0);
  } else {
    return LEDRings::Color24bit(red, green, blue);
  }
}

/**
 * @brief Load the nightmode settings from EEPROM
*/
void loadNightmodeSettingsFromEEPROM()
{
  nightModeStartHour = EEPROM.read(ADR_NM_START_H);
  nightModeStartMin = EEPROM.read(ADR_NM_START_M);
  nightModeEndHour = EEPROM.read(ADR_NM_END_H);
  nightModeEndMin = EEPROM.read(ADR_NM_END_M);
  if (nightModeStartHour > 23)
    nightModeStartHour = 22; // set to 22 o'clock as default
  if (nightModeStartMin > 59)
    nightModeStartMin = 0;
  if (nightModeEndHour > 23)
    nightModeEndHour = 7; // set to 7 o'clock as default
  if (nightModeEndMin > 59)
    nightModeEndMin = 0;
  logger.logString("Nightmode starts at: " + String(nightModeStartHour) + ":" + String(nightModeStartMin));
  logger.logString("Nightmode ends at: " + String(nightModeEndHour) + ":" + String(nightModeEndMin));
}

/**
 * @brief Load the brightness settings from EEPROM
 * 
 * lower limit is 10 so that the LEDs are not completely off
*/
void loadBrightnessSettingsFromEEPROM()
{
  uint8_t brightnessIR = EEPROM.read(ADR_BRIGHTNESS_INNER);
  uint8_t brightnessOR = EEPROM.read(ADR_BRIGHTNESS_OUTER);
  if (brightnessIR < 10)
    brightnessIR = 10;
  if (brightnessOR < 10)
    brightnessOR = 10;
  ledrings.setBrightnessInnerRing(brightnessIR);
  ledrings.setBrightnessOuterRing(brightnessOR);
  logger.logString("BrightnessIR: " + String(brightnessIR) + ", BrightnessOR: " + String(brightnessOR));
}

/**
 * @brief Set the color for hours
 * 
 * @param red red value
 * @param green green value
 * @param blue blue value
*/
void setColorHours(uint8_t red, uint8_t green, uint8_t blue){
  colorHours = LEDRings::Color24bit(red, green, blue);
  EEPROM.write(ADR_CH_RED, red);
  EEPROM.write(ADR_CH_GREEN, green);
  EEPROM.write(ADR_CH_BLUE, blue);
  EEPROM.commit();
}

/**
 * @brief Set the color for minutes
 * 
 * @param red red value
 * @param green green value
 * @param blue blue value
*/
void setColorMinutes(uint8_t red, uint8_t green, uint8_t blue){
  colorMinutes = LEDRings::Color24bit(red, green, blue);
  EEPROM.write(ADR_CM_RED, red);
  EEPROM.write(ADR_CM_GREEN, green);
  EEPROM.write(ADR_CM_BLUE, blue);
  EEPROM.commit();
}

/**
 * @brief Set the color for seconds
 * 
 * @param red red value
 * @param green green value
 * @param blue blue value
*/
void setColorSeconds(uint8_t red, uint8_t green, uint8_t blue){
  colorSeconds = LEDRings::Color24bit(red, green, blue);
  EEPROM.write(ADR_CS_RED, red);
  EEPROM.write(ADR_CS_GREEN, green);
  EEPROM.write(ADR_CS_BLUE, blue);
  EEPROM.commit();
}

/**
 * @brief Handler for handling commands sent to "/cmd" url
 * 
 */
void handleCommand() {
  // receive command and handle accordingly
  for (uint8_t i = 0; i < server.args(); i++) {
    Serial.print(server.argName(i));
    Serial.print(F(": "));
    Serial.println(server.arg(i));
  }
  
  if (server.argName(0) == "col_hours") // the parameter which was sent to this server is the color for hours
  {
    String colorstr = server.arg(0) + "-";
    String redstr = split(colorstr, '-', 0);
    String greenstr= split(colorstr, '-', 1);
    String bluestr = split(colorstr, '-', 2);
    logger.logString(colorstr);
    logger.logString("r: " + String(redstr.toInt()));
    logger.logString("g: " + String(greenstr.toInt()));
    logger.logString("b: " + String(bluestr.toInt()));
    setColorHours(redstr.toInt(), greenstr.toInt(), bluestr.toInt());
  }
  else if (server.argName(0) == "col_minutes") // the parameter which was sent to this server is the color for minutes
  {
    String colorstr = server.arg(0) + "-";
    String redstr = split(colorstr, '-', 0);
    String greenstr= split(colorstr, '-', 1);
    String bluestr = split(colorstr, '-', 2);
    logger.logString(colorstr);
    logger.logString("r: " + String(redstr.toInt()));
    logger.logString("g: " + String(greenstr.toInt()));
    logger.logString("b: " + String(bluestr.toInt()));
    setColorMinutes(redstr.toInt(), greenstr.toInt(), bluestr.toInt());
  }
  else if (server.argName(0) == "col_seconds") // the parameter which was sent to this server is the color for seconds
  {
    String colorstr = server.arg(0) + "-";
    String redstr = split(colorstr, '-', 0);
    String greenstr= split(colorstr, '-', 1);
    String bluestr = split(colorstr, '-', 2);
    logger.logString(colorstr);
    logger.logString("r: " + String(redstr.toInt()));
    logger.logString("g: " + String(greenstr.toInt()));
    logger.logString("b: " + String(bluestr.toInt()));
    setColorSeconds(redstr.toInt(), greenstr.toInt(), bluestr.toInt());
  }
  else if(server.argName(0) == "ledoff"){
    String modestr = server.arg(0);
    logger.logString("LED off change via Webserver to: " + modestr);
    if(modestr == "1") ledOff = true;
    else ledOff = false;
  }
  else if(server.argName(0) == "setting"){
    String timestr = server.arg(0) + "-";
    logger.logString("Nightmode setting change via Webserver to: " + timestr);
    nightModeStartHour = split(timestr, '-', 0).toInt();
    nightModeStartMin = split(timestr, '-', 1).toInt();
    nightModeEndHour = split(timestr, '-', 2).toInt();
    nightModeEndMin = split(timestr, '-', 3).toInt();
    uint8_t brightnessIR = split(timestr, '-', 4).toInt();
    uint8_t brightnessOR = split(timestr, '-', 5).toInt();
    if(nightModeStartHour > 23) nightModeStartHour = 22; // set default
    if(nightModeStartMin > 59) nightModeStartMin = 0;
    if(nightModeEndHour > 23) nightModeEndHour = 7; // set default
    if(nightModeEndMin > 59) nightModeEndMin = 0;
    if(brightnessIR < 10) brightnessIR = 10;
    if(brightnessOR < 10) brightnessOR = 10;
    EEPROM.write(ADR_NM_START_H, nightModeStartHour);
    EEPROM.write(ADR_NM_START_M, nightModeStartMin);
    EEPROM.write(ADR_NM_END_H, nightModeEndHour);
    EEPROM.write(ADR_NM_END_M, nightModeEndMin);
    EEPROM.write(ADR_BRIGHTNESS_INNER, brightnessIR);
    EEPROM.write(ADR_BRIGHTNESS_OUTER, brightnessOR);
    logger.logString("Nightmode starts at: " + String(nightModeStartHour) + ":" + String(nightModeStartMin));
    logger.logString("Nightmode ends at: " + String(nightModeEndHour) + ":" + String(nightModeEndMin));
    logger.logString("BrightnessIR: " + String(brightnessIR) + ", BrightnessOR: " + String(brightnessOR));
    ledrings.setBrightnessInnerRing(brightnessIR);
    ledrings.setBrightnessOuterRing(brightnessOR);
    lastNightmodeCheck = 0;
  }
  else if (server.argName(0) == "resetwifi"){
    logger.logString("Reset Wifi via Webserver...");
    wifiManager.resetSettings();
    runQuickLEDTest();
  }
  
  server.send(204, "text/plain", "No Content"); // this page doesn't send back content --> 204
}

/**
 * @brief Splits a string at given character and return specified element
 * 
 * @param s string to split
 * @param parser separating character
 * @param index index of the element to return
 * @return String 
 */
String split(String s, char parser, int index) {
  String rs="";
  int parserCnt=0;
  int rFromIndex=0, rToIndex=-1;
  while (index >= parserCnt) {
    rFromIndex = rToIndex+1;
    rToIndex = s.indexOf(parser,rFromIndex);
    if (index == parserCnt) {
      if (rToIndex == 0 || rToIndex == -1) return "";
      return s.substring(rFromIndex,rToIndex);
    } else parserCnt++;
  }
  return rs;
}

/**
 * @brief Handler for GET requests
 * 
 */
void handleDataRequest() {
  // receive data request and handle accordingly
  for (uint8_t i = 0; i < server.args(); i++) {
    Serial.print(server.argName(i));
    Serial.print(F(": "));
    Serial.println(server.arg(i));
  }
  
  if (server.argName(0) == "key") // the parameter which was sent to this server is led color
  {
    String message = "{";
    String keystr = server.arg(0);
    if(keystr == "mode"){
      message += "\"ledoff\":\"" + String(ledOff) + "\"";
      message += ",";
      message += "\"nightMode\":\"" + String(nightMode) + "\"";
      message += ",";
      message += "\"nightModeStart\":\"" + leadingZero2Digit(nightModeStartHour) + "-" + leadingZero2Digit(nightModeStartMin) + "\"";
      message += ",";
      message += "\"nightModeEnd\":\"" + leadingZero2Digit(nightModeEndHour) + "-" + leadingZero2Digit(nightModeEndMin) + "\"";
      message += ",";
      message += "\"brightnessIR\":\"" + String(ledrings.getBrightnessInnerRing()) + "\"";
      message += ",";
      message += "\"brightnessOR\":\"" + String(ledrings.getBrightnessOuterRing()) + "\"";
    }
    message += "}";
    logger.logString(message);
    server.send(200, "application/json", message);
  }
}

/**
 * @brief Convert Integer to String with leading zero
 * 
 * @param value 
 * @return String 
 */
String leadingZero2Digit(int value){
  String msg = "";
  if(value < 10){
    msg = "0";
  }
  msg += String(value);
  return msg;
}