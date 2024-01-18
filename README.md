# Long distance lamp using ESP8266

## Goal:  
  Create a pair of lamp to establish a meaningful way to communicate between friends or couples. With just some taps, the lamp lightens up with different colors representing moods of the sender. Once the receiver replies, both lamps lighten up brightly, indicating a strong connection is made.

## Demo:
![](lamps.gif) 

## Part list:
- 1 x 12-LEDs ring
- Wesmos D1 mini (clone)
- TP223B touch sensor
- SSD1306 OLED display
- Wires

## Instructions
1. Initially, you can connect the lamp to WiFi using a phone/laptop and using the name and password defined by `wifiName` and `wifiPassword` in `./LDLamp.ino`.
2. Once connected, the lamp flashes Yellow twice.
3. Start choosing your moodlight by pressing the sensor for 2 seconds. Each sequential tap will change the moodlight, whose message is described by the OLED screen.
4. Release the sensor to confirm the moodlight. Signal is sent to the other user.
5. Once the other user replied by tapping the lamp, a connection is made and both lamps lighten up at 100%! This last for 15 minutes.
   
