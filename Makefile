ARDUINO_DIR  = /usr/share/arduino

TARGET       = light-painting-wand
ARDUINO_LIBS = Adafruit_NeoPixel

BOARD_TAG    = leonardo
ARDUINO_PORT = /dev/ttyACM0

include /usr/share/arduino/Arduino.mk
