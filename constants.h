#define OUTER_RING_LED_COUNT 60
#define INNER_RING_LED_COUNT 12
#define OUTER_RING_LED_PIN 4
#define INNER_RING_LED_PIN 5
#define CURRENT_LIMIT_LED 1000

// ST7735 pins from the hardware test sketch: CS=D8/GPIO15, RST=D3/GPIO0, DC=D4/GPIO2.
#define TFT_CS 15
#define TFT_RST 0
#define TFT_DC 2

// Move all display content to compensate for the physical circular mask.
#define DISPLAY_CONTENT_OFFSET_X 14
#define DISPLAY_CONTENT_OFFSET_Y 0

// Display layout positions are relative to DISPLAY_CONTENT_OFFSET_X/Y.
#define DISPLAY_CENTER_X 80
#define DISPLAY_WEATHER_ICON_CENTER_Y 34
#define DISPLAY_TEMPERATURE_TOP_Y 65
#define DISPLAY_DATE_TOP_Y 100
#define DISPLAY_STATUS_TOP_Y 116

// Fallback location is Berlin and is used only if the IP location request fails.
#define WEATHER_FALLBACK_LATITUDE 52.52
#define WEATHER_FALLBACK_LONGITUDE 13.41
#define WEATHER_REFRESH_PERIOD 900000
#define WEATHER_RETRY_PERIOD 60000
#define WEATHER_HTTP_TIMEOUT_MS 5000
#define LOCATION_HTTP_TIMEOUT_MS 5000
#define GEOCODING_HTTP_TIMEOUT_MS 5000
#define PERIOD_DISPLAY_UPDATE 60000

#define WIFI_CONNECT_TIMEOUT_SECONDS 30
#define WIFI_CONFIG_PORTAL_TIMEOUT_SECONDS 180

#define PERIOD_HEARTBEAT 10000
#define PERIOD_NTP_UPDATE 60000
#define PERIOD_CLOCK_UPDATE 20
#define PERIOD_LED_UPDATE 10
#define PERIOD_NIGHTMODE_CHECK 20000

// Seconds fade curve: 1.0 is linear, lower values brighten the dim part of the fade.
#define SECONDS_FADE_GAMMA 0.65

#define EEPROM_SIZE 96      // size of EEPROM to save persistent variables
#define ADR_NM_START_H 0
#define ADR_NM_END_H 4
#define ADR_NM_START_M 8
#define ADR_NM_END_M 12
#define ADR_BRIGHTNESS_OUTER 16
#define ADR_BRIGHTNESS_INNER 17
#define ADR_CS_RED 18
#define ADR_CS_GREEN 19
#define ADR_CS_BLUE 20
#define ADR_CM_RED 21
#define ADR_CM_GREEN 22
#define ADR_CM_BLUE 23
#define ADR_CH_RED 24
#define ADR_CH_GREEN 25
#define ADR_CH_BLUE 26
#define ADR_WEATHER_CUSTOM 27
#define ADR_WEATHER_LAT 28
#define ADR_WEATHER_LON 32
#define ADR_WEATHER_LABEL 36
#define WEATHER_LOCATION_MAX_LENGTH 40
