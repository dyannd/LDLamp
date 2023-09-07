


//get this info from the "My Key" section of the Adafruit IO website
#define IO_USERNAME  "tzuy4nhh"
#define IO_KEY       "aio_hJPr30cQSCVndn6yWXsbyZuKzu7W"

//Input your WiFi username and password here so the ESP8266 can connect to it
#define WIFI_SSID "Verizon_D4WVJH"
#define WIFI_PASS "gym-both7-warp"
// #define WIFI_SSID "tzuyanhh"
// #define WIFI_PASS "dyan0808"


// ***************************IGNORE ALL BELOW THIS**********************************




#include "AdafruitIO_WiFi.h"

#if defined(USE_AIRLIFT) || defined(ADAFRUIT_METRO_M4_AIRLIFT_LITE) ||         \
    defined(ADAFRUIT_PYPORTAL)
// Configure the pins used for the ESP32 connection
#if !defined(SPIWIFI_SS) // if the wifi definition isnt in the board variant
// Don't change the names of these #define's! they match the variant ones
#define SPIWIFI SPI
#define SPIWIFI_SS 10 // Chip select pin
#define NINA_ACK 9    // a.k.a BUSY or READY pin
#define NINA_RESETN 6 // Reset pin
#define NINA_GPIO0 -1 // Not connected
#endif
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS, SPIWIFI_SS,
                   NINA_ACK, NINA_RESETN, NINA_GPIO0, &SPIWIFI);
#else
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);
#endif
