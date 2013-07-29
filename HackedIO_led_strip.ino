#include <Adafruit_NeoPixel.h>

#define PIN 6
#define NUM_PIXELS 120


// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream
//   NEO_GRB     Pixels are wired for GRB bitstream
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);

typedef struct {
  uint8_t x[NUM_PIXELS * 3];
} stripLine;

uint8_t displayBuffer[NUM_PIXELS * 3]; // RGB byte pairs
uint8_t serialBuffer[NUM_PIXELS * 3]; // RGB byte pairs
uint8_t incomingByte;
uint8_t lastByte;
uint8_t lastButOneByte;
bool populateBuffer = 1;
uint8_t headerCounter = 0;
uint16_t bufPos = 0;

uint32_t lastDisplayMillis = 0;
uint32_t updateIntervalMillis = 1000;

const int statusLed = 13;
const int timingLed = 14;
const int serialLed = 15;
const int stripSendLed = 16;


void setup() {
  pinMode(statusLed, OUTPUT);
  pinMode(serialLed, OUTPUT);
  pinMode(timingLed, OUTPUT);
  pinMode(stripSendLed, OUTPUT);

  digitalWrite(serialLed, HIGH);
  digitalWrite(timingLed, HIGH);
  digitalWrite(stripSendLed, HIGH);

  digitalWrite(statusLed, HIGH);
  delay(500);
  digitalWrite(statusLed, LOW);
  delay(100);
  digitalWrite(statusLed, HIGH);
  delay(500);

  // initialise buffer
  for ( uint16_t i=0; i<NUM_PIXELS; i++ ) {
    displayBuffer[i * 3] = 255;
    displayBuffer[i * 3 + 1] = 0;
    displayBuffer[i * 3 + 2] = 0;
    serialBuffer[i * 3] = 0;
    serialBuffer[i * 3 + 1] = 0;
    serialBuffer[i * 3 + 2] = 0;
  }

  Serial.begin(115200);

  while (!Serial) {
    digitalWrite(serialLed, HIGH);
    delay(100);
    digitalWrite(serialLed, LOW);
    delay(100);
  }

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  digitalWrite(statusLed, HIGH);
  delay(500);
  digitalWrite(statusLed, LOW);
  delay(500);
  digitalWrite(statusLed, HIGH);
  delay(500);
  digitalWrite(statusLed, LOW);

  digitalWrite(serialLed, LOW);
  digitalWrite(timingLed, LOW);

}

bool updateSerialBuffer = false;
uint32_t loopCounter = 0;

void loop() {

  if ( updateSerialBuffer ) {
    if ( getDataFromSerial() ) {
      for ( uint16_t i=0; i<(NUM_PIXELS*3); i++) {
        displayBuffer[i] = serialBuffer[i];
      }
      updateSerialBuffer = false;
    } 
  }

  if ( updateDisplayIfTimingCorrect(displayBuffer, updateIntervalMillis) ) {
    updateSerialBuffer = true;
  }

  loopCounter++;
  loopCounter %= 10000;

  if ( loopCounter == 5000 ) {
    digitalWrite(timingLed, HIGH);
    fullStripSetSingleColour(strip.Color(128,128,0));
  } else if ( loopCounter == 0 ) {
    digitalWrite(timingLed, LOW);
    strip.show(); // blank
  } 

  digitalWrite(statusLed, HIGH);

}

void requestDataFromSerial() {
  if (Serial) {
    Serial.println("READY\n\r");
  }
}

void ackDataFromSerial() {
  if (Serial) {
    Serial.println("PACKET_RECEIVED\n\r");
  }
}

void abortDataFromSerial() {
  if (Serial) {
    Serial.println("PACKET_FAILED\n\r");
  }
}

bool getDataFromSerial() {
  uint16_t bufPos = 0;
  while ( Serial.available() > 0 ) {
    digitalWrite(serialLed, HIGH);
    lastButOneByte = lastByte;
    lastByte = incomingByte;
    incomingByte = Serial.read();
    if ( bufPos < ( NUM_PIXELS * 3) ) {
      serialBuffer[bufPos] = incomingByte;
      bufPos++;
      digitalWrite(serialLed, LOW);
    } else {
      return false;  // should not get here - if we transmit NUM_PIXELS*3 bytes correctly that is.
    }
  }
  if ( bufPos < (NUM_PIXELS * 3) ) {
     // bad packet - too small.
     return false;
  }
}

bool updateDisplayIfTimingCorrect(uint8_t *buffer, uint32_t wait ) {
  if ( lastDisplayMillis - millis() > wait ) {
    //fullStripSet(buffer);
    return true;
  } else {
    return false;
  }
}

void fullStripSetSingleColour(uint32_t c) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    digitalWrite(stripSendLed, HIGH);
    delay(50);
    strip.setPixelColor(i, c);
    digitalWrite(stripSendLed, LOW);
    delay(50);
  }
  strip.show();
}

void fullStripSet(uint8_t *c) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c[(i*3)], c[(i*3)+1], c[(i*3)+2]);
  }
  strip.show();
}
