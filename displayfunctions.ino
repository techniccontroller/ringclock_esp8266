/**
 * @file displayfunctions.ino
 *
 * @brief TFT display, weather and location related functions.
 */

void setupDisplay(){
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.setTextWrap(false);
  tft.fillScreen(ST77XX_BLACK);
  drawCenteredText("Ringclock", 34, 2, ST77XX_CYAN);
  drawCenteredText("connecting...", 58, 1, ST77XX_WHITE);
}

bool updateLocationFromAPI(){
  if(WiFi.status() != WL_CONNECTED){
    location.valid = false;
    return false;
  }

  WiFiClient client;
  HTTPClient http;
  bool result = false;

  logger.logString("[HTTP] Requesting location from IP-API");

  if(http.begin(client, "http://ip-api.com/json/?fields=status,message,country,countryCode,region,regionName,city,zip,lat,lon,timezone,offset,query")){
    client.setTimeout(LOCATION_HTTP_TIMEOUT_MS);
    http.setTimeout(LOCATION_HTTP_TIMEOUT_MS);
    int httpCode = http.GET();

    if(httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY){
      String payload = http.getString();
      String status = extractJsonString(payload, "status");

      if(status == "success"){
        float latitude = 0.0;
        float longitude = 0.0;
        int offsetSeconds = 0;

        if(extractJsonFloat(payload, "lat", latitude) && extractJsonFloat(payload, "lon", longitude)){
          location.latitude = latitude;
          location.longitude = longitude;
          location.city = extractJsonString(payload, "city");
          location.timezone = extractJsonString(payload, "timezone");

          if(extractJsonInt(payload, "offset", offsetSeconds)){
            location.offsetMinutes = offsetSeconds / 60;
          }

          location.valid = true;
          result = true;
          logger.logString("[HTTP] Received location: " + location.city + " (" + String(location.latitude, 4) + ", " + String(location.longitude, 4) + ")");
          logger.logString("[HTTP] Received timezone: " + location.timezone + ", offset min: " + String(location.offsetMinutes));
        }
      }
      else {
        logger.logString("[HTTP] IP-API returned status: " + status + ", message: " + extractJsonString(payload, "message"));
      }
    }
    else {
      logger.logString("[HTTP] Location request failed: HTTP " + String(httpCode));
    }

    http.end();
  }
  else {
    logger.logString("[HTTP] Unable to connect to IP-API");
  }

  if(!result){
    location.valid = false;
    location.latitude = WEATHER_FALLBACK_LATITUDE;
    location.longitude = WEATHER_FALLBACK_LONGITUDE;
    logger.logString("[HTTP] Using fallback weather location: " + String(location.latitude, 4) + ", " + String(location.longitude, 4));
  }

  return result;
}

void showIPAddressOnDisplay(const IPAddress &ip){
  displayBlankedForNightMode = false;
  tft.fillScreen(ST77XX_BLACK);
  drawCenteredText("IP address", 30, 2, ST77XX_CYAN);
  drawCenteredText(ip.toString(), 64, 1, ST77XX_WHITE);
  drawCenteredText("LEDs show last byte", 90, 1, ST77XX_YELLOW);
  lastDisplayUpdate = millis();
}

void scheduleNextWeatherAttempt(bool success){
  if(success){
    lastWeatherUpdate = millis();
  }
  else {
    lastWeatherUpdate = millis() - WEATHER_REFRESH_PERIOD + WEATHER_RETRY_PERIOD;
  }
}

void markWeatherAttemptFailed(const String &message){
  logger.logString(message);
  if(!weather.valid){
    weather.updatedAt = "--:--";
  }
  scheduleNextWeatherAttempt(false);
}

void updateDisplay(){
  displayBlankedForNightMode = false;
  tft.fillScreen(ST77XX_BLACK);

  int16_t x = DISPLAY_CONTENT_OFFSET_X;
  int16_t y = DISPLAY_CONTENT_OFFSET_Y;

  if(weather.valid){
    drawWeatherIcon(x + DISPLAY_CENTER_X, y + DISPLAY_WEATHER_ICON_CENTER_Y, weather.weatherCode);
    drawCenteredTemperature(y + DISPLAY_TEMPERATURE_TOP_Y, weather.temperature);
  }
  else {
    //drawCenteredText("weather --", y + DISPLAY_WEATHER_ICON_CENTER_Y, 2, ST77XX_YELLOW);
  }

  drawCenteredText(ntp.getFormattedDate(), y + DISPLAY_DATE_TOP_Y, 1, ST77XX_WHITE);

  /*tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN);
  if(location.valid && location.city.length() > 0){
    tft.setCursor(x + 12, y + DISPLAY_STATUS_TOP_Y);
    tft.print(location.city);
    tft.print(" ");
    tft.print(weather.updatedAt);
  }
  else {
    tft.setCursor(x + 48, y + DISPLAY_STATUS_TOP_Y);
    tft.print("upd ");
    tft.print(weather.updatedAt);
  }*/

  lastDisplayUpdate = millis();
}

void blankDisplayForNightMode(){
  if(!displayBlankedForNightMode){
    tft.fillScreen(ST77XX_BLACK);
    displayBlankedForNightMode = true;
  }

  lastDisplayUpdate = millis();
}

void updateWeather(){
  if(WiFi.status() != WL_CONNECTED){
    WiFi.reconnect();
    markWeatherAttemptFailed("Weather update skipped: WiFi disconnected");
    return;
  }

  if(!location.valid){
    updateLocationFromAPI();
  }

  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(WEATHER_HTTP_TIMEOUT_MS);
  HTTPClient http;
  String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(location.latitude, 4) +
               "&longitude=" + String(location.longitude, 4) +
               "&current=temperature_2m,weather_code&timezone=auto";

  if(!http.begin(client, url)){
    markWeatherAttemptFailed("Weather update failed: HTTP begin failed");
    return;
  }

  http.setTimeout(WEATHER_HTTP_TIMEOUT_MS);
  int httpCode = http.GET();
  bool success = false;
  if(httpCode == HTTP_CODE_OK){
    String payload = http.getString();
    float temperature = 0.0;
    int weatherCode = -1;

    bool temperatureFound = extractJsonFloat(payload, "temperature_2m", temperature);
    bool weatherCodeFound = extractJsonInt(payload, "weather_code", weatherCode);

    if(!temperatureFound){
      temperatureFound = extractJsonFloat(payload, "temperature", temperature);
    }
    if(!weatherCodeFound){
      weatherCodeFound = extractJsonInt(payload, "weathercode", weatherCode);
    }

    if(temperatureFound && weatherCodeFound){
      weather.temperature = temperature;
      weather.weatherCode = weatherCode;
      weather.valid = true;
      weather.updatedAt = ntp.getFormattedTime().substring(0, 5);
      success = true;
      logger.logString("Weather update successful: " + String(weather.temperature, 1) + "C, code " + String(weather.weatherCode));
    }
    else {
      logger.logString("Weather update failed: JSON values missing");
    }
  }
  else {
    logger.logString("Weather update failed: HTTP " + String(httpCode));
  }

  http.end();
  scheduleNextWeatherAttempt(success);
  if(nightMode){
    blankDisplayForNightMode();
  }
  else {
    updateDisplay();
  }
}

bool extractJsonFloat(const String &payload, const String &key, float &value){
  String token = "\"" + key + "\":";
  int searchFrom = 0;

  while(searchFrom < payload.length()){
    int start = payload.indexOf(token, searchFrom);
    if(start < 0){
      return false;
    }

    start += token.length();
    while(start < payload.length() && (payload[start] == ' ' || payload[start] == '\n' || payload[start] == '\r' || payload[start] == '\t')){
      start++;
    }

    if(start < payload.length() && (isDigit(payload[start]) || payload[start] == '-' || payload[start] == '.')){
      int end = start;
      while(end < payload.length() && (isDigit(payload[end]) || payload[end] == '-' || payload[end] == '.')){
        end++;
      }

      value = payload.substring(start, end).toFloat();
      return true;
    }

    searchFrom = start + 1;
  }

  return false;
}

bool extractJsonInt(const String &payload, const String &key, int &value){
  float parsedValue = 0.0;
  if(!extractJsonFloat(payload, key, parsedValue)){
    return false;
  }

  value = int(parsedValue);
  return true;
}

String extractJsonString(const String &payload, const String &key){
  String token = "\"" + key + "\":\"";
  int start = payload.indexOf(token);
  if(start < 0){
    return "";
  }

  start += token.length();
  int end = payload.indexOf("\"", start);
  if(end < 0){
    return "";
  }

  return payload.substring(start, end);
}

void drawCenteredText(const String &text, int16_t y, uint8_t size, uint16_t color){
  int16_t x1, y1;
  uint16_t w, h;
  tft.setTextSize(size);
  tft.setTextColor(color);
  tft.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  tft.setCursor(DISPLAY_CONTENT_OFFSET_X + ((160 - w) / 2), y);
  tft.print(text);
}

void drawCenteredTemperature(int16_t y, float temperature){
  String tempText = String(temperature, 0) + "C";
  int16_t x1, y1;
  uint16_t w, h;

  tft.setTextSize(3);
  tft.setTextColor(ST77XX_WHITE);
  tft.getTextBounds(tempText, 0, y, &x1, &y1, &w, &h);

  int16_t x = DISPLAY_CONTENT_OFFSET_X + ((160 - w) / 2);
  tft.setCursor(x, y);
  tft.print(String(temperature, 0));
  tft.drawCircle(x + w - 19, y + 3, 2, ST77XX_WHITE);
  tft.setCursor(x + w - 12, y);
  tft.print("C");
}

void drawTemperature(int16_t x, int16_t y, float temperature){
  String tempText = String(temperature, 0);
  int16_t x1, y1;
  uint16_t w, h;

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(3);
  tft.getTextBounds(tempText, x, y, &x1, &y1, &w, &h);
  tft.setCursor(x, y);
  tft.print(tempText);
  tft.drawCircle(x + w + 7, y + 3, 2, ST77XX_WHITE);
  tft.setCursor(x + w + 14, y);
  tft.print("C");
}

void drawWeatherIcon(int16_t x, int16_t y, int weatherCode){
  if(weatherCode == 0){
    drawSunIcon(x, y);
  }
  else if(weatherCode == 1 || weatherCode == 2 || weatherCode == 3 || weatherCode == 45 || weatherCode == 48){
    drawCloudIcon(x, y, weatherCode == 1 || weatherCode == 2);
  }
  else if((weatherCode >= 51 && weatherCode <= 67) || (weatherCode >= 80 && weatherCode <= 82)){
    drawRainIcon(x, y);
  }
  else if(weatherCode >= 71 && weatherCode <= 77){
    drawSnowIcon(x, y);
  }
  else if(weatherCode >= 95){
    drawStormIcon(x, y);
  }
  else {
    drawCloudIcon(x, y, false);
  }
}

void drawSunIcon(int16_t x, int16_t y){
  tft.fillCircle(x, y, 14, ST77XX_YELLOW);
  for(int i = 0; i < 8; i++){
    float angle = i * PI / 4.0;
    int16_t x1 = x + cos(angle) * 20;
    int16_t y1 = y + sin(angle) * 20;
    int16_t x2 = x + cos(angle) * 28;
    int16_t y2 = y + sin(angle) * 28;
    tft.drawLine(x1, y1, x2, y2, ST77XX_YELLOW);
  }
}

void drawCloudIcon(int16_t x, int16_t y, bool partlySunny){
  if(partlySunny){
    tft.fillCircle(x - 17, y - 10, 11, ST77XX_YELLOW);
  }

  uint16_t cloudColor = ST77XX_WHITE;
  tft.fillCircle(x - 12, y, 12, cloudColor);
  tft.fillCircle(x + 2, y - 7, 16, cloudColor);
  tft.fillCircle(x + 18, y, 12, cloudColor);
  tft.fillRoundRect(x - 24, y, 56, 16, 5, cloudColor);
}

void drawRainIcon(int16_t x, int16_t y){
  drawCloudIcon(x, y - 8, false);
  for(int i = -18; i <= 18; i += 12){
    tft.drawLine(x + i, y + 16, x + i - 4, y + 25, ST77XX_CYAN);
  }
}

void drawSnowIcon(int16_t x, int16_t y){
  drawCloudIcon(x, y - 8, false);
  for(int i = -18; i <= 18; i += 18){
    tft.drawLine(x + i - 4, y + 19, x + i + 4, y + 27, ST77XX_CYAN);
    tft.drawLine(x + i + 4, y + 19, x + i - 4, y + 27, ST77XX_CYAN);
  }
}

void drawStormIcon(int16_t x, int16_t y){
  drawCloudIcon(x, y - 8, false);
  tft.fillTriangle(x - 4, y + 12, x + 8, y + 12, x, y + 27, ST77XX_YELLOW);
  tft.fillTriangle(x, y + 24, x + 12, y + 24, x - 4, y + 42, ST77XX_YELLOW);
}
