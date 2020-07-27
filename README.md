# code cleanup
The original code from Elecrow needed some cleanup (usused code, logo display, display driver, duplicate code blocks).
Since I also aim to use the water level sensor together with the Elecrow Watering Kit, I've used the codebase by rfrancis97.
In order to compile this code, the U8glib (which has been unmaintained for several years) must be patched using the patch file U8glib.patch.
As an alternative for patching the U8glib, the U8GLIB_SSD1306_130X64 can be replaced with the original U8GLIB_SSD1306_128X64, which shows the two rows of pixels at the right edge of the screen.

# elecrow-watering-kit-to-ESP8266
I wanted to modify the code for the Elecrow Watering kit to read moisture and water level data and send it over MQTT to Node-RED or Home Assistant via wifi using an ESP8266 NodeMCU.
It is necessary to have an Elecrow Leonardo Watering Kit relay board that has the correct Arduino uploader to modify the existing code. I had to specifically request a replacement, but you can install the uploader using another Arduino.
Leonardo RX (Serial1) on 8-pin header should connect to ESP8266 RX pin D5(GPIO14). I used a voltage divider to reduce 5v Leonardo Serial1 output to 3.3v ESP8266 input.
I used an ultrasonic sensor type HC-SR04 and Leonardo pins D7 and A4 to measure reserve tank water level; D7=pinMode(1, OUTPUT). A4=pinMode(A4, INPUT). I used Grove - 4 Pin Female Jumper Cables to connect to the Elecrow board.
Modifications accomplish ability to do the following:
  1. Read (4) moisture sensors and pump status (on/off) via mqtt (read once per minute, but is programmable).
  2. Use ultrasonic sensor HC-SR04 to measure reserve water tank level and disable the pump when low, so pump does not run when dry.
