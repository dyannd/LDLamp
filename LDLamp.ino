//import the libraries
#include <Adafruit_NeoPixel.h>
#include "config.h"

//define the Neopixel pin, this will be different then the pin we soldered the Neopixels to, as they are marked differently
#define PIN 5


//initialize the information for the Neopixels and Adafruit IO
Adafruit_NeoPixel strip = Adafruit_NeoPixel(12, PIN, NEO_GRB + NEO_KHZ800);
AdafruitIO_Feed *lamp = io.feed("lamp_aebiu");

//how long we want the lamps to stay on when activated (600,000 ms = 10 minutes)
const long interval = 600000;

//setup the timers and status for the lamp
unsigned long previousMillis = 0;
int tap = 0;

//set one of the lamps to 1, the other to 2
int lampVal = 1;

//the value that should activate the lamp. Don't mess with this, the code will figure out what this should be in the setup
int recVal = 0;



void setup() {

  //Start the serial monitor for debugging and status
  Serial.begin(9600);

  //figure out what recieved value should turn on the lamp (lampVal of other lamp)
  if (lampVal == 1) recVal = 2;
  if (lampVal == 2) recVal = 1;
  
  //Activate the Neopixels
  strip.begin();
  strip.setBrightness(255);
  strip.show(); // Initialize all pixels to 'off'

  //setup the touch sensor as a interrupt and input
  pinMode(12, INPUT);
  attachInterrupt(digitalPinToInterrupt(12), touch, CHANGE);
  
  //start connecting to Adafruit IO
  Serial.print("Connecting to Adafruit IO");
  io.connect();

  //get the status of the value in Adafruit IO
  lamp->onMessage(handleMessage);

  //connect to Adafruit IO and play the "spin" Neopixel animation to show it's connecting until and connection is established
  while(io.status() < AIO_CONNECTED) {
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
  
  
  previousMillis = millis();
}

void loop() {

  //keeps the ESP8266 connected to Adafruit IO
  io.run();

  //set the starting timer value
  unsigned long currentMillis = millis();

  //tap = 1 means that we have established that the other lamp was tapped
  if (tap == 1) {

    //check to see if the timer if over 10 minutes. If it is, turn off the Neopixels and reset the tap status
    if (currentMillis - previousMillis >= interval) {
      off();
      tap = 0;
    
    } else {

      //if the timer isn't over 10 minutes, continue playing the rainbow animation at a slow speed
      rainbow(200);
      
    }
    
  }
  
}


//the interrupt program that runs when the touch sensor is activated
ICACHE_RAM_ATTR void touch() {

  //while the touch sensor is activated, save the lampVal (either 1 or 2) to the Adafruit IO feed and turn the Neopixels to purple
  while (digitalRead(12) == 1) {

    lamp->save(lampVal);
      Serial.print("This is lamp number ");
      Serial.print(lampVal);
      Serial.println();
      for(int i=0; i<strip.numPixels(); i++) {
        
      strip.setPixelColor(i, 102, 0, 204);
      
    }
    
    strip.show();

   //once the touch sensor isn't activated, send a 0 back to the Adafruit IO feed. 
  } if (digitalRead(12) == 0) {
    
      lamp->save(0);
      
      for(int i=0; i<strip.numPixels(); i++) {
        
      strip.setPixelColor(i, 0, 0, 0);
      
    }
    
    strip.show();
    
  }
  

  
}


//code that tells the ESP8266 what to do when it recieves new data from the Adafruit IO feed
void handleMessage(AdafruitIO_Data *data) {

  Serial.print("received <- ");

  //convert the recieved data to an INT
  int reading = data->toInt();

  //if the recieved value is equal to the recVal, and the lamp status is currently off, change the status to on and recent the timer
  if(reading == recVal && tap == 0) {
    
    Serial.println("TAP");
    previousMillis = millis();
    tap = 1;

  //if we recieve a value but the lamp is already on nothing happens
  } else {
    Serial.println("LOW");
  }

}

//simple code to turn all of the Neopixels off
void off() {
  
for(int i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, 0, 0, 0);
    }
    strip.show();
  
}


//The code that creates the gradual color change animation in the Neopixels (thank you to Adafruit for this!!)
void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}


//complicated geometry or something to figure out the color values (I don't know how this stuff works, thank goodness for adafruit)
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}


//code to flash the Neopixels when a stable connection to Adafruit IO is made
void flash() {

  for(int i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, 255, 255, 255);
    }
  strip.show();
  
  delay(200);

  for(int i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, 0, 0, 0);
    }
  strip.show();
  
  delay(200);

  for(int i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, 255, 255, 255);
    }
  strip.show();
  
  delay(200);

  for(int i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, 0, 0, 0);
    }
  strip.show();
  
  delay(200);
}


//the code to create the blue spinning animation when connecting to Adafruit IO
void spin() {

  for(int i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, 0, 0, 255);
      strip.show();
      delay(20);
    }
    for(int i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, 0, 0, 0);
      strip.show();
      delay(20);
    }
  
}
