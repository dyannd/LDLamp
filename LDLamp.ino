//import the libraries
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include <NeoPixelBrightnessBus.h>

//define the Neopixel pin, this will be different then the pin we soldered the Neopixels to, as they are marked differently
#define PIN 3  //pin on the wesmos chip, RX pin
#define INITIALBRIGHTNESS 64
#define GPIOPIN 12  //pin on the wesmos chip, 12 is D6
#define LEDS 12
//initialize the information for the Neopixels and Adafruit IO
NeoPixelBrightnessBus< NeoGrbFeature, NeoEsp8266Dma800KbpsMethod > lightStrip(LEDS, PIN);

// Adafruit_NeoPixel lightStrip = Adafruit_NeoPixel(GPIOPIN, PIN, NEO_GRB + NEO_KHZ800);  //length, pin and pixel type constructor
AdafruitIO_Feed *lamp = io.feed("lamp_aebiu");  //lamp points to the feed name

int state = 0;
int selected_color = 0;  //  Index for color vector
int i_breath;

char msg[50];  //  Custom messages for Adafruit IO

const int max_intensity = 255;  //  Max intensity
//  Color definitions
RgbColor red(max_intensity, 0, 0);
RgbColor green(0, max_intensity, 0);
RgbColor blue(0, 0, max_intensity);
RgbColor purple(200, 0, max_intensity);
RgbColor cyan(0, max_intensity, max_intensity);
RgbColor yellow(max_intensity, 200, 0);
RgbColor white(max_intensity, max_intensity, max_intensity);
RgbColor pink(255, 20, 30);
RgbColor orange(max_intensity, 50, 0);
RgbColor black(0, 0, 0);

RgbColor colors[] = {
  red,
  orange,
  yellow,
  green,
  cyan,
  blue,
  purple,
  pink,
  white,
  black
};

//how long we want the lamps to stay on when activated in millis
const unsigned long interval = 60000;

//setup the timers and status for the lamp, in millis
unsigned long initialTime = 0;
int tap = 0;

//set one of the lamps to 1, the other to 2 (change when uploading sketch to different chips)
int lampVal = 2;

//the value that should activate the lamp (is 2 for the first lamp, and 1 for the second)
int recVal{ 0 };
int sendVal{ 0 };

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
const unsigned long conection_time_out = 300000;  // 5 minutos

void setup() {

  //Start the serial monitor for debugging and status
  Serial.begin(9600);

  //  Set ID values
  if (lampVal == 1) {
    recVal = 20;
    sendVal = 10;
  } else if (lampVal == 2) {
    recVal = 10;
    sendVal = 20;
  }


  //Activate the Neopixels
  lightStrip.Begin();
  lightStrip.Show();  // Initialize all pixels

  //setup the touch sensor as a interrupt and input
  pinMode(GPIOPIN, INPUT);
  // attachInterrupt(digitalPinToInterrupt(GPIOPIN), touch, CHANGE);  //executes the touch function when event changes on GPIOpin

  //start connecting to Adafruit IO
  Serial.print("Connecting to Adafruit IO");
  io.connect();

  //get the status of the value in Adafruit IO
  lamp->onMessage(handleMessage);

  //connect to Adafruit IO and play the "spin" animation to show it's connecting until and connection is established
  while (io.status() < AIO_CONNECTED) {
    Serial.print(".");
    spin(6);
    delay(500);
  }
  off();

  //when a connection to Adafruit IO is made write the status in the Serial monitor and flash the Neopixels white
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
      if (lastState == LOW && currentState == HIGH)  // Button is pressed
      {
        pressedTime = millis();
      } else if (currentState == HIGH) {
        releasedTime = millis();
        long pressDuration = releasedTime - pressedTime;
        if (pressDuration > long_press_time) {
          state = 1;
        }
      }
      lastState = currentState;
      break;

    case 1:  // Wait for button release
      selected_color = 0;
      light_half_intensity(selected_color);
      state = 2;
      RefMillis = millis();
      while (digitalRead(GPIOPIN) == HIGH) {}
      break;

    case 2:  // Color selector
      if (digitalRead(GPIOPIN) == HIGH) {
        selected_color++;
        if (selected_color > 9)
          selected_color = 0;
        while (digitalRead(GPIOPIN) == HIGH) {
          delay(50);
        }
        light_half_intensity(selected_color);
        // Reset timer each time it is touched
        RefMillis = millis();
      }
      // If a color is selected more than a time, it is interpreted as the one selected
      ActMillis = millis();
      if (ActMillis - RefMillis > send_selected_color_time) {
        if (selected_color == 9)  //  Cancel msg
          state = 8;
        else
          state = 3;
      }
      break;

    case 3:  // Publish msg
      sprintf(msg, "L%d: color send", lampVal);
      lamp->save(msg);
      lamp->save(selected_color + sendVal);
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

  //tap = 1 means that we have established that the other lamp was tapped
  // if (tap == 1) {

  //   //check to see if the timer if over our predefined interval. If it is, turn off the Neopixels and reset the tap status
  //   if (currentTime - initialTime >= interval) {
  //     off();
  //     tap = 0;
  //   } else {
  //     //if the timer isn't over the predefined interval, continue playing the rainbow animation at a slow speed
  //     rainbow(200);
  //   }
  // }
}


//the interrupt program that runs when the touch sensor is activated
// IRAM_ATTR void touch() {

//   //while the touch sensor is activated, save the lampVal (either 1 or 2) to the Adafruit IO feed and turn the Neopixels to purple
//   while (digitalRead(GPIOPIN) == 1) {

//     lamp->save(lampVal);
//     Serial.print("This is lamp number ");
//     Serial.print(lampVal);
//     Serial.println();
//     for (int i = 0; i < LEDS; i++) {
//       lightStrip.SetPixelColor(i, 102, 0, 204);
//     }

//     lightStrip.Show();

//     //once the touch sensor isn't activated, send a 0 back to the Adafruit IO feed.
//   }
//   if (digitalRead(GPIOPIN) == 0) {

//     lamp->save(0);

//     for (int i = 0; i < LEDS; i++) {

//       lightStrip.SetPixelColor(i, 0, 0, 0);
//     }

//     lightStrip.Show();
//   }
// }


//code that tells the ESP8266 what to do when it recieves new data from the Adafruit IO feed
void handleMessage(AdafruitIO_Data *data) {

  Serial.print("received <- ");

  //convert the recieved data to an INT
  int reading = data->toInt();

  //if the received value is equal to the recVal, and the lamp status is currently off, change the status to on and renew the timer
  if (reading == recVal && tap == 0) {

    Serial.println("TAP");
    initialTime = millis();
    Serial.println(initialTime);
    tap = 1;

    //if we recieve a value but the lamp is already on, nothing happens
  } else {
    Serial.println("LOW");
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


// //The code that creates the gradual color change animation in the Neopixels (thank you to Adafruit for this!!)
// void rainbow(uint8_t wait) {
//   uint16_t i, j;

//   for (j = 0; j < 256; j++) {
//     for (i = 0; i < LEDS; i++) {
//       lightStrip.SetPixelColor(i, Wheel((i + j) & 255));
//     }
//     lightStrip.Show();
//     delay(wait);
//   }
// }


// //complicated geometry or something to figure out the color values
// uint32_t Wheel(byte WheelPos) {
//   WheelPos = 255 - WheelPos;
//   if (WheelPos < 85) {
//     return lightStrip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
//   }
//   if (WheelPos < 170) {
//     WheelPos -= 85;
//     return lightStrip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
//   }
//   WheelPos -= 170;
//   return lightStrip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
// }


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
