/*
   See:
   https://github.com/dsalzman/HD44780-decoder/blob/master/main.ino
   https://github.com/sp0x/LcdSniffer/blob/master/lcd_receiver.ino
   https://en.wikipedia.org/wiki/Hitachi_HD44780_LCD_controller
*/

#include <FastGPIO.h>

#define EN 2
#define RW 3
#define RS 4
#define D0 5
#define D1 6
#define D2 7
#define D3 8
#define D4 9
#define D5 10
#define D6 11
#define D7 12
#define NUM_CHARS 32

volatile char d0, d1, d2, d3, d4, d5, d6, d7, rs, rw;

void decode(void);

bool gotUpNib = false;
bool fourBitMode = true;
uint8_t upperNibble;
long changetime = 0;

char LCD[NUM_CHARS] = { 32 };
char LCD_shadow[NUM_CHARS] = { 32 };
uint8_t charIndex = 0;

void setup() {
  Serial.begin(115200);
  FastGPIO::Pin<RS>::setInput();    //Define RS
  FastGPIO::Pin<RW>::setInput();    //Define R/W
  FastGPIO::Pin<D7>::setInput();
  FastGPIO::Pin<D6>::setInput();
  FastGPIO::Pin<D5>::setInput();
  FastGPIO::Pin<D4>::setInput();
  FastGPIO::Pin<D3>::setInput();
  FastGPIO::Pin<D2>::setInput();
  FastGPIO::Pin<D1>::setInput();
  FastGPIO::Pin<D0>::setInput();
  Serial.println("Ready");

  attachInterrupt(digitalPinToInterrupt(EN), decode, FALLING);
}

void loop() {
  delay(100);
  for (int i = 0; i < NUM_CHARS; i++) {
    if (LCD[i] != LCD_shadow[i]) {
      changetime = millis();
      LCD_shadow[i] = LCD[i];
    }
  }

  if (changetime > 0 && changetime < millis() - 500) {
    Serial.println();
    for (int i = 0; i < NUM_CHARS; i++) {
      Serial.print(LCD_shadow[i]);
      if ((i + 1) % (NUM_CHARS / 2) == 0) {
        Serial.println();
      }
    }
    changetime = 0;
  }
}

inline uint8_t readBytes() {
  rs = FastGPIO::Pin<RS>::isInputHigh() ? 1 : 0;
  rw = FastGPIO::Pin<RW>::isInputHigh() ? 1 : 0;
  d7 = FastGPIO::Pin<D7>::isInputHigh() ? 1 : 0;
  d6 = FastGPIO::Pin<D6>::isInputHigh() ? 1 : 0;
  d5 = FastGPIO::Pin<D5>::isInputHigh() ? 1 : 0;
  d4 = FastGPIO::Pin<D4>::isInputHigh() ? 1 : 0;
  if (fourBitMode) {
    return (d7 << 3) + (d6 << 2) + (d5 << 1) + d4;
  } else {
    d3 = FastGPIO::Pin<D3>::isInputHigh() ? 1 : 0;
    d2 = FastGPIO::Pin<D2>::isInputHigh() ? 1 : 0;
    d1 = FastGPIO::Pin<D1>::isInputHigh() ? 1 : 0;
    d0 = FastGPIO::Pin<D0>::isInputHigh() ? 1 : 0;
    return (d7 << 7) + (d6 << 6) + (d5 << 5) + (d4 << 4) + (d3 << 3) + (d2 << 2) + (d1 << 1) + d0;
  }
}

void decode() {
  if (rw == 0) {
    // this is a write operation
    uint8_t data = readBytes();

    if (fourBitMode) {
      if (gotUpNib) {
        data = (upperNibble << 4 | data);
        gotUpNib = false;
      } else {
        upperNibble = data;
        gotUpNib = true;
        return;
      }
    }

    if (rs == 1) {
      // print character received
      LCD[charIndex] = (char) data;
      charIndex = (charIndex + 1) % 32;
    } else {
      // command received
      evalCommand(data);
    }
  }
}

const uint8_t LCD_CLEAR =  0x01;
const uint8_t LCD_CLEAR_MASK = 0xFF;

const uint8_t LCD_SETDDRAMADDR = 0x80;
const uint8_t LCD_SETDDRAMADDR_MASK = 0x80;

const uint8_t LCD_SETFUNCTION_4BIT = 0x20;
const uint8_t LCD_SETFUNCTION_8BIT = 0x30;
const uint8_t LCD_SETFUNCTION_MASK = 0xF0;

void evalCommand(uint8_t command) {
  if ((command & LCD_CLEAR_MASK) == LCD_CLEAR) {
    for (int i = 0; i < 32; i++) {
      LCD[i] = 32;
    }
    charIndex = 0;
    return;
  }
  if ((command & LCD_SETDDRAMADDR_MASK) == LCD_SETDDRAMADDR) {
    if ((command & 0x7f) < 16) {
      charIndex = (command & 0x7f);
    } else {
      charIndex = (command & 0x7f) - 48;
    }
    return;
  }
  if ((command & LCD_SETFUNCTION_MASK) == LCD_SETFUNCTION_8BIT) {
    fourBitMode = false;
    return;
  }
  if ((command & LCD_SETFUNCTION_MASK) == LCD_SETFUNCTION_4BIT) {
    fourBitMode = true;
    gotUpNib = false;
    return;
  }
}
