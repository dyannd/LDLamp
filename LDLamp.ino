//import the libraries
#include <Adafruit_NeoPixel.h>
#include "config.h"

//define the Neopixel pin, this will be different then the pin we soldered the Neopixels to, as they are marked differently
#define PIN 5
#define INITIALBRIGHTNESS 64
#define GPIOPIN 12  //pin on the wesmos chip, 12 is D6

//initialize the information for the Neopixels and Adafruit IO
Adafruit_NeoPixel lightStrip = Adafruit_NeoPixel(GPIOPIN, PIN, NEO_GRB + NEO_KHZ800);  //length, pin and pixel type constructor
AdafruitIO_Feed *lamp = io.feed("lamp_aebiu");                                         //lamp points to the feed name

//how long we want the lamps to stay on when activated in millis
const unsigned long interval = 60000;

//setup the timers and status for the lamp, in millis
unsigned long initialTime = 0;
int tap = 0;

//set one of the lamps to 1, the other to 2 (change when uploading sketch to different chips)
int lampVal = 2;

//the value that should activate the lamp (is 2 for the first lamp, and 1 for the second)
int recVal = 0;



void setup() {

  //Start the serial monitor for debugging and status
  Serial.begin(9600);

  //figure out what recieved value should turn on the lamp (lampVal of other lamp)
  if (lampVal == 1) recVal = 2;
  if (lampVal == 2) recVal = 1;

  //Activate the Neopixels
  lightStrip.begin();
  lightStrip.setBrightness(INITIALBRIGHTNESS);
  lightStrip.show();  // Initialize all pixels

  //setup the touch sensor as a interrupt and input
  pinMode(GPIOPIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(GPIOPIN), touch, CHANGE);  //executes the touch function when event changes on GPIOpin

  //start connecting to Adafruit IO
  Serial.print("Connecting to Adafruit IO");
  io.connect();

  //get the status of the value in Adafruit IO
  lamp->onMessage(handleMessage);

  //connect to Adafruit IO and play the "spin" animation to show it's connecting until and connection is established
  while (io.status() < AIO_CONNECTED) {
    Serial.print(".");
    spin();
    delay(500);
  }

  //when a connection to Adafruit IO is made write the status in the Serial monitor and flash the Neopixels white
  Serial.println();
  Serial.println(io.statusText());
  flash();

  //get the status of our value in Adafruit IO
  lamp->get();

  //init the initial time
  initialTime = millis();
}

void loop() {

  //keeps the ESP8266 connected to Adafruit IO
  io.run();

  //set the starting timer value
  unsigned long currentTime = millis();

  //tap = 1 means that we have established that the other lamp was tapped
  if (tap == 1) {

    //check to see if the timer if over our predefined interval. If it is, turn off the Neopixels and reset the tap status
    if (currentTime - initialTime >= interval) {
      off();
      tap = 0;
    } else {
      //if the timer isn't over the predefined interval, continue playing the rainbow animation at a slow speed
      rainbow(200);
    }
  }
}


//the interrupt program that runs when the touch sensor is activated
IRAM_ATTR void touch() {

  //while the touch sensor is activated, save the lampVal (either 1 or 2) to the Adafruit IO feed and turn the Neopixels to purple
  while (digitalRead(GPIOPIN) == 1) {

    lamp->save(lampVal);
    Serial.print("This is lamp number ");
    Serial.print(lampVal);
    Serial.println();
    for (int i = 0; i < lightStrip.numPixels(); i++) {
      lightStrip.setPixelColor(i, 102, 0, 204);
    }

    lightStrip.show();

    //once the touch sensor isn't activated, send a 0 back to the Adafruit IO feed.
  }
  if (digitalRead(GPIOPIN) == 0) {

    lamp->save(0);

    for (int i = 0; i < lightStrip.numPixels(); i++) {

      lightStrip.setPixelColor(i, 0, 0, 0);
    }

    lightStrip.show();
  }
}


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

//turn all of the Neopixels off
void off() {

  for (int i = 0; i < lightStrip.numPixels(); i++) {
    lightStrip.setPixelColor(i, 0, 0, 0);
  }
  lightStrip.show();
}


//The code that creates the gradual color change animation in the Neopixels (thank you to Adafruit for this!!)
void rainbow(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256; j++) {
    for (i = 0; i < lightStrip.numPixels(); i++) {
      lightStrip.setPixelColor(i, Wheel((i + j) & 255));
    }
    lightStrip.show();
    delay(wait);
  }
}


//complicated geometry or something to figure out the color values
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return lightStrip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return lightStrip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return lightStrip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}


//code to flash the Neopixels when a stable connection to Adafruit IO is made
void flash() {

  for (int i = 0; i < lightStrip.numPixels(); i++) {
    lightStrip.setBrightness(20);
    lightStrip.setPixelColor(i, 255, 255, 255);
  }

  lightStrip.show();

  delay(200);

  for (int i = 0; i < lightStrip.numPixels(); i++) {
    lightStrip.setPixelColor(i, 0, 0, 0);
  }
  lightStrip.show();

  delay(200);

  for (int i = 0; i < lightStrip.numPixels(); i++) {
    lightStrip.setBrightness(20);
    lightStrip.setPixelColor(i, 255, 255, 255);
  }
  lightStrip.show();

  delay(200);

  for (int i = 0; i < lightStrip.numPixels(); i++) {
    lightStrip.setPixelColor(i, 0, 0, 0);
  }
  lightStrip.show();

  delay(200);
}



//Spinning animation when connecting to Adafruit IO
void spin() {

  for (int i = 0; i < lightStrip.numPixels(); i++) {
    lightStrip.setBrightness(64);
    lightStrip.setPixelColor(i, 255, 255, 255);
    lightStrip.show();
    delay(100);
  }
  for (int i = 0; i < lightStrip.numPixels(); i++) {
    lightStrip.setBrightness(64);
    lightStrip.setPixelColor(i, 0, 0, 0);
    lightStrip.show();
    delay(100);
  }
}
