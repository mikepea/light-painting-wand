#include <Adafruit_NeoPixel.h>

#define PIN 6
#define NUM_PIXELS 60

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
uint8_t displayBuffer[bufferSize]; // RGB byte triples

uint8_t bufPos = 0;

uint32_t lastDisplayMillis = 0;
uint32_t updateIntervalMillis = 50;
uint32_t serialWaitMillis = 1000;

const int statusLed = 13;
const int timingLed = 14;
const int serialLed = 15;
const int stripSendLed = 16;

uint32_t loopCounter = 0;

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
  delay(500);
  digitalWrite(statusLed, HIGH);
  delay(500);

  // initialise buffer
  for ( uint16_t i=0; i<NUM_PIXELS; i++ ) {
    displayBuffer[i * 3] = 0;
    displayBuffer[i * 3 + 1] = 128;
    displayBuffer[i * 3 + 2] = 64;
  }

  Serial.begin(SERIAL_BAUD);

  while (!Serial) {
    digitalWrite(serialLed, HIGH);
    delay(100);
    digitalWrite(serialLed, LOW);
    delay(100);
  }

  for ( uint8_t i=0; i<5; i++) {
    Serial.println(i);
    delay(200);
  }

  Serial.print("Size of displayBuffer: ");
  Serial.println(bufferSize);

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  fullStripDisplay(); // show initial pattern

  delay(500);
  digitalWrite(serialLed, LOW);
  delay(500);
  digitalWrite(timingLed, LOW);
  delay(500);

}


bool readyToAskForData = true;
bool readyToReceiveData = false;
uint32_t timeLastAskedForData = 0;

void loop() {

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

void requestDataFromSerial() {
  if (readyToAskForData || ( millis() - timeLastAskedForData > serialWaitMillis ) ) {
    if ( Serial.available() <= 0 ) {
      Serial.println("READY\n\r");
      bufPos = 0;
      readyToAskForData = false;
      readyToReceiveData = true;
      timeLastAskedForData = millis();
    }
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

bool readIntoBuffer = false;
uint16_t byteCounter = 0;

void processIncomingByte(uint8_t b) {
  if ( b == 'Y' ) {
    readIntoBuffer = true;
    bufPos = 0;
    byteCounter = 0;
    Serial.println("==== got header!");

  } else if ( b == 'Z' ) {
    // end of transmission
    // fill rest with zeros
    for ( uint16_t i=bufPos; i < bufferSize; i++ ) {
      displayBuffer[i] = 0;
    }
    readIntoBuffer = false;
    Serial.println("==== got footer!");

  } else if ( readIntoBuffer && bufPos < bufferSize ) {
    uint8_t val = convertAsciiHexToBin(b);
    if ( val != 255 ) {

      if ( byteCounter % 2 == 0 ) { 
        // high octet
        displayBuffer[bufPos] = val << 4;
        byteCounter++;
      } else {
        // low octet 
        displayBuffer[bufPos] += val;
        bufPos++;
        byteCounter++;
      }

    }

  } else if ( bufPos >= NUM_PIXELS*3 ) {
    // urg, too much data
    Serial.println("==== too much serial data sent!");
    readIntoBuffer = false;
  }

  //Serial.print("byteCounter: ");
  //Serial.println(byteCounter);

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

bool updateDisplayIfTimingCorrect(uint32_t wait ) {
  if ( millis() - lastDisplayMillis > wait ) {
    Serial.println("displaying buffer");
    fullStripDisplay();
    lastDisplayMillis = millis();
    return true;
  } else {
    return false;
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
  digitalWrite(stripSendLed, HIGH);
  uint32_t c;
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    c = strip.Color( displayBuffer[(i*3)], displayBuffer[(i*3)+1], displayBuffer[(i*3)+2] );
    strip.setPixelColor(i, c);
#ifdef DEBUG_BUFFER_SEND_VALUES
    Serial.print(i);
    Serial.print(':');
    Serial.print(displayBuffer[(i*3)], HEX);
    Serial.print(',');
    Serial.print(displayBuffer[(i*3)+1], HEX);
    Serial.print(',');
    Serial.print(displayBuffer[(i*3)+2], HEX);
    Serial.println();
#endif
  }
  strip.show();
  readyToAskForData = true;
  digitalWrite(stripSendLed, LOW);
}
