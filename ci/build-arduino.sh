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

# Install Arduino AVR core
arduino-cli core install arduino:avr

# Install the required libraries
arduino-cli lib install RTClib
arduino-cli lib install U8glib

# Patch the U8glib library
patch -p0 -d $HOME/Arduino/libraries <$GITHUB_WORKSPACE/U8glib.patch

# Compile all *.ino files for the Arduino Leonardo
for f in **/*.ino ; do
  arduino-cli compile -b arduino:avr:leonardo $f
done
