// //import the libraries
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "color.h"
#include <NeoPixelBrightnessBus.h>
#include <WiFiManager.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 32  // OLED display height, in pixels
//miss: pink
//vui: yellow
//sad: xanh duong
//angry: purp
//lam hoa: white
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//define the Neopixel pin, this will be different then the pin we soldered the Neopixels to, as they are marked differently
#define PIN 3  //pin on the wesmos chip, RX pin
#define INITIALBRIGHTNESS 64
#define GPIOPIN 12  //pin on the wesmos chip, 12 is D6
#define LEDS 12


//set one of the lamps to 1, the other to 2 (change when uploading sketch to different chips) (breadboard is 2)
int lampVal = 1;

//initialize the information for the Neopixels and Adafruit IO
NeoPixelBrightnessBus< NeoGrbFeature, NeoEsp8266Dma800KbpsMethod > lightStrip(LEDS, PIN);

// Adafruit_NeoPixel lightStrip = Adafruit_NeoPixel(GPIOPIN, PIN, NEO_GRB + NEO_KHZ800);  //length, pin and pixel type constructor
AdafruitIO_Feed *lamp = io.feed("lamp_aebiu");  //lamp points to the feed name

int state = 0;
int selected_color = 0;  //  Index for color vector
int i_breath;

char msg[50];  //  Custom messages for Adafruit IO

//setup the timers and status for the lamp, in millis
unsigned long initialTime = 0;
int tap = 0;

//the value that should activate the lamp (is 2 for the first lamp, and 1 for the second)
int recVal{ 0 };
int sendVal{ 0 };
String mainChar;
String currChar;
char* wifiName;
char* wifiPass = "18072022";

// Long press detection
const int long_press_time = 2000;
int lastState = LOW;  // the previous state from the input pin
int currentState;     // the current reading from the input pin
unsigned long pressedTime = 0;
unsigned long releasedTime = 0;

// Time vars
unsigned long RefMillis;
unsigned long ActMillis;
const int send_selected_color_time = 4000;
const int answer_time_out = 900000;
const int on_time = 900000;

// Disconection timeout
unsigned long currentMillis;
unsigned long previousMillis = 0;
const unsigned long conection_time_out = 300000;  // 5 minutes



void setup() {

  //Start the serial monitor for debugging and status
  Serial.begin(115200);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }

  // Activate neopixels
  lightStrip.Begin();
  lightStrip.Show(); // Initialize all pixels to 'off'

  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(500);
  display.clearDisplay();
  delay(100);
  display.setTextSize(0.8);  // Draw 1X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 1);
  display.println("Doi xiu dang ket noi");
  display.display();

  //setup the touch sensor as an input
  pinMode(GPIOPIN, INPUT);

  //  Set ID values
  if (lampVal == 1) {
    recVal = 20;
    sendVal = 10;
    mainChar = String("em biu");
    currChar = String("anh biu");
    wifiName = "Den tinh iu cua anh piu"; //set current wifiname
  } else if (lampVal == 2) {
    recVal = 10;
    sendVal = 20;
    mainChar = String("anh biu");
    currChar = String("em biu");
    wifiName = "Den tinh iu cua em piu"; //set current wifiname
  }

  wificonfig(); //initializes wifi

  //start connecting to Adafruit IO
  Serial.printf("\nConnecting to Adafruit IO with User: %s Key: %s.\n", IO_USERNAME, IO_KEY);
  AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, "", "");
  io.connect();

  //get the status of the value in Adafruit IO when a message is received
  lamp->onMessage(handleMessage);


  //connect to Adafruit IO and play the "spin" animation to show it's connecting until and connection is established
  while (io.status() < AIO_CONNECTED) {
    Serial.print(".");
    spin(6);
    delay(500);
  }

  // Clear the buffer
  display.clearDisplay();
  off();

  //when a connection to Adafruit IO is made write the status in the Serial monitor and flash the Neopixels
  Serial.println();
  Serial.println(io.statusText());
  // Animation
  spin(3);
  off();
  delay(50);
  flash(8);
  off();
  delay(100);
  flash(8);
  off();
  delay(50);

  //get the status of our value in Adafruit IO
  lamp->get();
  lamp->save(msg);
}

void loop() {

  //keeps the ESP8266 connected to Adafruit IO
  io.run();

  //set the starting timer value
  unsigned long currentTime = millis();

  // State machine
  switch (state) {
      // Wait
    case 0:
      currentState = digitalRead(GPIOPIN);
      if (lastState == LOW && currentState == HIGH) {  // Button is pressed{
        pressedTime = millis();
      } else if (currentState == HIGH) {
        releasedTime = millis();
        long pressDuration = releasedTime - pressedTime;
        if (pressDuration > long_press_time) {
          state = 1;
        }
      } else {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Giu 2 giay de goi " + mainChar + " nho");
        display.display();  // Show initial text
        delay(100);
      }
      lastState = currentState;
      break;

    case 1:  // Wait for button release
      selected_color = 0;
      light_half_intensity(selected_color);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Nho " + mainChar + " quaaa");
      display.display();
      delay(100);
      state = 2;
      RefMillis = millis();
      while (digitalRead(GPIOPIN) == HIGH) {}
      break;

    case 2:  // Color selector
      if (selected_color == 0) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Nho " + mainChar + " quaaa");
        display.display();
        delay(100);
      } else if (selected_color == 1) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Bun qua " + mainChar + " oii");
        display.display();
        delay(100);
      } else if (selected_color == 2) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Doi " + mainChar + " ruii >.<");
        display.display();
        delay(100);
      } else if (selected_color == 3) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Dang vui " + mainChar + " oii :))))");
        display.display();
        delay(100);
      } else if (selected_color == 4) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Thui lam hoa di moooo");
        display.display();
        delay(100);
      } else if (selected_color == 5) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Doi y het muon goi rui");
        display.display();  // Show initial text
        delay(100);
      }
      if (digitalRead(GPIOPIN) == HIGH) {
        selected_color++;
        // reset state if scroll passed
        if (selected_color > 5)
          selected_color = 0;
        while (digitalRead(GPIOPIN) == HIGH) {}
        light_half_intensity(selected_color);
        // Reset timer each time it is touched
        RefMillis = millis();
      }

      // If a color is selected more than a time, it is interpreted as the one selected
      ActMillis = millis();
      if (ActMillis - RefMillis > send_selected_color_time) {
        if (selected_color == 5)  //  Cancel msg
          state = 8;
        else
          state = 3;
      }
      break;

    case 3:  // Publish msg
      sprintf(msg, "L%d: color send", lampVal);
      lamp->save(msg);
      lamp->save(selected_color + sendVal);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Dang goi " + mainChar);
      display.println("Doi xiu i..");
      display.display();  // Show initial text
      delay(100);
      Serial.print(selected_color + sendVal);
      state = 4;
      flash(selected_color);
      light_half_intensity(selected_color);
      delay(100);
      flash(selected_color);
      light_half_intensity(selected_color);
      break;

    case 4:  // Set timer
      RefMillis = millis();
      state = 5;
      i_breath = 0;
      break;

    case 5:  // Wait for answer
      for (i_breath = 0; i_breath <= 314; i_breath++) {
        breath(selected_color, i_breath);
        ActMillis = millis();
        if (ActMillis - RefMillis > answer_time_out) {
          off();
          sprintf(msg, "L%d: Answer time out", lampVal);
          lamp->save(msg);
          lamp->save(0);
          state = 8;
          break;
        }
      }
      break;

    case 6:  // Answer received
      Serial.println("Answer received");
      light_full_intensity(selected_color);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println(mainChar + " tra loi neee..");
      display.println("<3 <3 <3..");
      display.display();  // Show initial text
      delay(100);
      RefMillis = millis();
      sprintf(msg, "L%d: connected", lampVal);
      lamp->save(msg);
      lamp->save(0);
      state = 7;
      break;

    case 7:  // Turned on
      ActMillis = millis();
      //  Send pulse
      if (digitalRead(GPIOPIN) == HIGH) {
        lamp->save(420 + sendVal);
        pulse(selected_color);
      }
      if (ActMillis - RefMillis > on_time) {
        off();
        lamp->save(0);
        state = 8;
      }
      break;

    case 8:  // Reset before state 0
      off();
      state = 0;
      break;

    case 9:  // Msg received
      if (selected_color == 0) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println(mainChar + " nho " + currChar + " quaaa");
        display.display();
        delay(100);
      } else if (selected_color == 1) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println(mainChar + " bun qua baby oii");
        display.display();
        delay(100);
      } else if (selected_color == 2) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println(mainChar + " doi ruii >.<");
        display.display();
        delay(100);
      } else if (selected_color == 3) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println(mainChar + " vui quo hehehe :))))");
        display.display();
        delay(100);
      } else if (selected_color == 4) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Thui lam hoa di moooo");
        display.display();
        delay(100);
      }
      sprintf(msg, "L%d: msg received", lampVal);
      lamp->save(msg);
      RefMillis = millis();
      state = 10;
      break;

    case 10:  // Send answer wait
      for (i_breath = 236; i_breath <= 549; i_breath++) {
        breath(selected_color, i_breath);
        if (digitalRead(GPIOPIN) == HIGH) {
          state = 11;
          break;
        }
        ActMillis = millis();
        if (ActMillis - RefMillis > answer_time_out) {
          off();
          sprintf(msg, "L%d: answer time out", lampVal);
          lamp->save(msg);
          state = 8;
          break;
        }
      }
      break;

    case 11:  // Send answer
      light_full_intensity(selected_color);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Ket noi thanh cong :*");
      display.display();  // Show initial text
      delay(100);
      RefMillis = millis();
      sprintf(msg, "L%d: answer sent", lampVal);
      lamp->save(msg);
      lamp->save(1);
      state = 7;
      break;

    default:
      state = 0;
      break;
  }
}

// void displayLCDText(Strings[] strArr) {
//   display.clearDisplay();
//   display.setCursor(0, 7);
//   for (int i = 0; i < sizeof strArr; i++){
//     display.println(strArr[i]);
//   }
//   delay(50);
// }
void testscrolltext(void) {
  display.clearDisplay();

  display.setTextSize(1);  // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.println(F("nho em biu <3"));
  display.display();  // Show initial text
  delay(100);

  // Scroll in various directions, pausing in-between:
  display.startscrollright(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrollleft(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrolldiagright(0x00, 0x07);
  delay(2000);
  display.startscrolldiagleft(0x00, 0x07);
  delay(2000);
  display.stopscroll();
  delay(1000);
}

//code that tells the ESP8266 what to do when it recieves new data from the Adafruit IO feed
void handleMessage(AdafruitIO_Data *data) {
  //convert the recieved data to an INT
  int reading = data->toInt();
  if (reading == 66) {  //reset lamp 1 with code 401
    sprintf(msg, "L%d: rebooting", lampVal);
    lamp->save(msg);
    delay(2000);
    ESP.restart();
  } else if (reading == 100) {
    sprintf(msg, "L%d: ping", lampVal);
    lamp->save(msg);
    lamp->save(0);
  } else if (reading == 420 + recVal) {
    sprintf(msg, "L%d: pulse received", lampVal);
    lamp->save(msg);
    lamp->save(0);
    pulse(selected_color);
  } else if (reading != 0 && reading / 10 != lampVal) {
    // Is it a color msg?
    if (state == 0 && reading != 1) {
      state = 9;
      selected_color = reading - recVal;
    }
    // Is it an answer?
    if (state == 5 && reading == 1)
      state = 6;
  }
}
// 50% intensity
void light_half_intensity(int ind) {
  lightStrip.SetBrightness(max_intensity / 2);
  for (int i = 0; i < LEDS; i++) {
    lightStrip.SetPixelColor(i, colors[ind]);
  }
  lightStrip.Show();
}

// 100% intensity
void light_full_intensity(int ind) {
  lightStrip.SetBrightness(max_intensity);
  for (int i = 0; i < LEDS; i++) {
    lightStrip.SetPixelColor(i, colors[ind]);
  }
  lightStrip.Show();
}

void pulse(int ind) {
  int i;
  int i_step = 5;
  for (i = max_intensity; i > 80; i -= i_step) {
    lightStrip.SetBrightness(i);
    for (int i = 0; i < LEDS; i++) {
      lightStrip.SetPixelColor(i, colors[ind]);
      lightStrip.Show();
      delay(1);
    }
  }
  delay(20);
  for (i = 80; i < max_intensity; i += i_step) {
    lightStrip.SetBrightness(i);
    for (int i = 0; i < LEDS; i++) {
      lightStrip.SetPixelColor(i, colors[ind]);
      lightStrip.Show();
      delay(1);
    }
  }
}

// Inspired by Jason Yandell
void breath(int ind, int i) {
  float MaximumBrightness = max_intensity / 2;
  float SpeedFactor = 0.02;
  float intensity;
  if (state == 5)
    intensity = MaximumBrightness / 2.0 * (1 + cos(SpeedFactor * i));
  else
    intensity = MaximumBrightness / 2.0 * (1 + sin(SpeedFactor * i));
  lightStrip.SetBrightness(intensity);
  for (int ledNumber = 0; ledNumber < LEDS; ledNumber++) {
    lightStrip.SetPixelColor(ledNumber, colors[ind]);
    lightStrip.Show();
    delay(1);
  }
}
//turn all of the Neopixels off
void off() {
  lightStrip.SetBrightness(max_intensity);
  for (int i = 0; i < LEDS; i++) {
    lightStrip.SetPixelColor(i, black);
  }
  lightStrip.Show();
}

//code to flash the Neopixels when a stable connection to Adafruit IO is made
void flash(int ind) {
  lightStrip.SetBrightness(max_intensity);
  for (int i = 0; i < LEDS; i++) {
    lightStrip.SetPixelColor(i, colors[ind]);
  }
  lightStrip.Show();

  delay(200);
}



//The code that creates the gradual color change animation in the Neopixels (thank you to Adafruit for this!!)
void spin(int ind) {
  lightStrip.SetBrightness(max_intensity);
  for (int i = 0; i < LEDS; i++) {
    lightStrip.SetPixelColor(i, colors[ind]);
    lightStrip.Show();
    delay(30);
  }
  for (int i = 0; i < LEDS; i++) {
    lightStrip.SetPixelColor(i, black);
    lightStrip.Show();
    delay(30);
  }
}


// Waiting connection led setup
void wait_connection() {
  lightStrip.SetBrightness(max_intensity/2);
  for (int i = 0; i < 3; i++) {
    lightStrip.SetPixelColor(i, yellow);
  }
  lightStrip.Show();
  delay(50);
  for (int i = 3; i < 6; i++) {
    lightStrip.SetPixelColor(i, pink);
  }
  lightStrip.Show();
  delay(50);
  for (int i = 6; i < 9; i++) {
    lightStrip.SetPixelColor(i, cyan);
  }
  lightStrip.Show();
  delay(50);
  for (int i = 9; i < 12; i++) {
    lightStrip.SetPixelColor(i, white);
  }
  lightStrip.Show();
  delay(50);
}

void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  wait_connection();
}

void wificonfig() {
  WiFi.mode(WIFI_STA);
  WiFiManager wifiManager;

  std::vector<const char *> menu = { "wifi", "info" };
  wifiManager.setMenu(menu);
  // set dark theme
  wifiManager.setClass("invert");

  bool res;
  wifiManager.setAPCallback(configModeCallback);
  res = wifiManager.autoConnect(wifiName, wifiPass);  // password protected ap
  display.setCursor(0, 0);
  display.println("Ket noi");
  display.display();
  if (!res) {
    spin(0);
    delay(50);
    off();
  } else {
    //if you get here you have connected to the WiFi
    spin(3);
    delay(50);
    off();
  }
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
