#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# Enable the globstar shell option
shopt -s globstar

# Make sure we are inside the github workspace
cd $GITHUB_WORKSPACE

# Create directories
mkdir $HOME/Arduino
mkdir $HOME/Arduino/libraries

# Install Arduino IDE
export PATH=$PATH:$GITHUB_WORKSPACE/bin
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
arduino-cli config init
arduino-cli core update-index

# Install Arduino AVR core and ESP8266 core
arduino-cli core install arduino:avr
arduino-cli core install esp8266:esp8266

# Install the required libraries
arduino-cli lib install RTClib
arduino-cli lib install U8glib

# Patch the U8glib library
patch -p0 -d $HOME/Arduino/libraries <$GITHUB_WORKSPACE/U8glib.patch

# Compile all *.ino files for the Arduino Leonardo
for f in wifi_watering_kit_code/*.ino ; do
  arduino-cli compile -b arduino:avr:leonardo $f
done

# Compile all *.ino files for the ESP8266
for f in ESP8266_watering_mqtt_client_code/*.ino ; do
  arduino-cli compile -b arduino:esp8266:esp8266 $f
done
