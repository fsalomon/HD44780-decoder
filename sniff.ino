/*
   See:
   https://github.com/dsalzman/HD44780-decoder/blob/master/main.ino
   https://github.com/sp0x/LcdSniffer/blob/master/lcd_receiver.ino
   https://en.wikipedia.org/wiki/Hitachi_HD44780_LCD_controller
*/

#include <FastGPIO.h>

#define EN 2
#define D7 3
#define D6 4
#define D5 5
#define D4 6
#define RW 7
#define RS 8
#define NUM_CHARS 32

volatile char d4, d5, d6, d7, rs, rw;

void decode(void);

bool gotUpNib = false;
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
  Serial.println("Ready");

  attachInterrupt(digitalPinToInterrupt(EN), decode, RISING);
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
  return (d7 << 3) + (d6 << 2) + (d5 << 1) + d4;
}

void decode() {
  uint8_t nibble = readBytes();

  if (rs == 1) {
    // print character received
    if (gotUpNib == false) {
      upperNibble = nibble;
      gotUpNib = true;
      return;
    } else {
      gotUpNib = false;
      LCD[charIndex] = (char) (upperNibble << 4 | nibble);
      //Serial.println( LCD[charIndex]);
      charIndex = (charIndex + 1) % 32;
      return;
    }
  } else {
    // command received
    if (gotUpNib == false) {
      upperNibble = nibble;
      gotUpNib = true;
      return;
    } else {
      gotUpNib = false;
      evalCommand(upperNibble << 4 | nibble);
      return;
    }
  }
}

const uint8_t LCD_CLEAR =  0x01;
const uint8_t LCD_CLEAR_MASK = 0xFF;

const uint8_t LCD_SETDDRAMADDR = 0x80;
const uint8_t LCD_SETDDRAMADDR_MASK = 0x80;

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
}
