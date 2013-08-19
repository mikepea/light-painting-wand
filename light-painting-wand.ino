#include <Adafruit_NeoPixel.h>

#define PIN 6
#define NUM_PIXELS 120

#define SERIAL_BAUD 115200

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream
//   NEO_GRB     Pixels are wired for GRB bitstream
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);

const uint16_t bufferSize = NUM_PIXELS * 3;

uint8_t brightness = 10;

uint32_t lastDisplayMillis = 0;
uint32_t updateIntervalMillis = 100;
uint32_t serialTimeOutMillis = 1000;

const int statusLed = 13;
const int timingLed = 14;
const int serialLed = 15;
const int stripSendLed = 16;

uint32_t loopCounter = 0;

bool readyToAskForData = true;
bool readyToReceiveData = false;
uint32_t timeLastAskedForData = 0;

bool readIntoBuffer = false;
uint16_t byteCounter = 0;
uint16_t colorPos = 0;
uint32_t colorBuf = 0;

uint8_t r;
uint8_t g;
uint8_t b;

void requestDataFromSerial() {
  if (readyToAskForData || ( millis() - timeLastAskedForData > serialTimeOutMillis ) ) {
    if ( Serial.available() <= 0 ) {
      Serial.println("READY\n\r");
      colorPos = 0;
      readyToAskForData = false;
      readyToReceiveData = true;
      timeLastAskedForData = millis();
    }
  }
}

uint8_t convertAsciiHexToBin(uint8_t hexDigit) {
  if ( hexDigit >= 97 && hexDigit <= 102 ) {
    // a-f
    return (hexDigit - 87);
  } else if ( hexDigit >= 65 && hexDigit <= 70 ) {
    // A-F
    return (hexDigit - 55);
  } else if ( hexDigit >= 48 && hexDigit <= 57 ) {
    // 0-9
    return (hexDigit - 48);
  } else {
    // fail
    return 255;
  }

}

void processIncomingByte(uint8_t byte) {
  if ( byte == 'Y' ) {
    readIntoBuffer = true;
    colorPos = 0;
    byteCounter = 0;
    Serial.println("==== got header!");

  } else if ( byte == 'Z' ) {
    // end of transmission
    // fill rest with zeros
    for ( uint16_t i=colorPos; i < strip.numPixels(); i++ ) {
      strip.setPixelColor(i, strip.Color(0,0,0));
    }
    readIntoBuffer = false;
    Serial.println("==== got footer!");

  } else if ( readIntoBuffer && ( colorPos < strip.numPixels() ) ) {
    uint8_t val = convertAsciiHexToBin(byte);
    if ( val != 255 ) {

      // build up a 32-bit strip color, via 6 hex digits
      switch ( byteCounter ) {
        case 0: // high octet red
          r = val << 4;
          byteCounter++;
          break;
        case 1: // low octet red
          r += val;
          byteCounter++;
          break;
        case 2: // high octet green
          g = val << 4;
          byteCounter++;
          break;
        case 3: // low octet green
          g += val;
          byteCounter++;
          break;
        case 4: // high octet blue
          b = val << 4;
          byteCounter++;
          break;
        case 5: // low octet blue
          b += val;
          // end of color, so set it and clear buffer.
          strip.setPixelColor(colorPos, r, g, b);
          r = 0; g = 0; b = 0; // set to black/off for rewriting
          byteCounter = 0;
          colorPos++;
          break;
          
      }

    }

  } else if ( colorPos >= strip.numPixels() ) {
    // urg, too much data
    Serial.println("==== too much serial data sent!");
    readIntoBuffer = false;
  }

}


bool getDataFromSerial() {
  uint8_t bytesRead = 0;
  while ( Serial.available() > 0 ) {
    uint8_t c = Serial.read();
    processIncomingByte(c);
    bytesRead++;
    if ( bytesRead > 50 ) {
      Serial.println("overflow!");
      break;
    }
  }
}

void fullStripSetSingleColour(uint32_t c) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
  strip.show();
  Serial.println();
}


void fullStripDisplay() {
  strip.setBrightness(brightness);
  strip.show();
}

bool updateDisplayIfTimingCorrect(uint32_t wait ) {
  if ( millis() - lastDisplayMillis > wait ) {
    Serial.println("displaying buffer");
    fullStripDisplay();
    readyToAskForData = true;
    lastDisplayMillis = millis();
    return true;
  } else {
    return false;
  }
}


void setupLedOutputs() {
  pinMode(statusLed, OUTPUT);
  pinMode(serialLed, OUTPUT);
  pinMode(timingLed, OUTPUT);
  pinMode(stripSendLed, OUTPUT);
}

void displayLedBootSeq() {
  digitalWrite(serialLed, HIGH);
  digitalWrite(timingLed, HIGH);
  digitalWrite(stripSendLed, HIGH);
  digitalWrite(statusLed, HIGH);
  delay(500);
  digitalWrite(statusLed, LOW);
  delay(500);
  digitalWrite(statusLed, HIGH);
  delay(500);
}

void displayLedBootTrailSeq() {
  delay(500);
  digitalWrite(serialLed, LOW);
  delay(500);
  digitalWrite(timingLed, LOW);
  delay(500);
}

void initialiseDisplayBuffer() {
  for ( uint16_t i=0; i<strip.numPixels(); i++ ) {
    strip.setPixelColor(i, 0, 0, 16);
  }
}

void setupAndWaitForSerial() {
  Serial.begin(SERIAL_BAUD);
  while (!Serial) {
    digitalWrite(serialLed, HIGH);
    delay(100);
    digitalWrite(serialLed, LOW);
    delay(100);
  }
}

void serialBootSequenceDisplay() {
  for ( uint8_t i=0; i<5; i++) {
    Serial.println(i);
    delay(200);
  }
  Serial.print("Size of displayBuffer: ");
  Serial.println(bufferSize);
  Serial.print("Size of LED Strip: ");
  Serial.println(strip.numPixels());
}

void initialiseLedStrip() {
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

//-----------------------------------------------------------------------------

void setup() {
  setupLedOutputs();
  displayLedBootSeq();
  initialiseLedStrip();
  initialiseDisplayBuffer();
  fullStripDisplay();
  setupAndWaitForSerial();
  serialBootSequenceDisplay();
  displayLedBootTrailSeq();
}

void loop() {

  // toggle status LED state, so we know how fast we're looping
  digitalWrite(statusLed, loopCounter % 2);

  requestDataFromSerial();

  if ( readyToReceiveData ) {
    getDataFromSerial();
  }

  updateDisplayIfTimingCorrect(updateIntervalMillis);

  if ( loopCounter == 5000 ) {
    digitalWrite(timingLed, HIGH);
    //fullStripSetSingleColour(strip.Color(128,128,0));
  } else if ( loopCounter == 0 ) {
    digitalWrite(timingLed, LOW);
    //fullStripSetSingleColour(strip.Color(0,128,128));
  } 

  loopCounter++;
  loopCounter %= 10000;

}

