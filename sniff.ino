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
#define RS 7
#define RW 8
#define NUM_CHARS 32

volatile char d4, d5, d6, d7, rs, rw;


void decode(void);

bool gotUpNib = false;
uint8_t upperNibble;
uint8_t ;

long changetime = 0;

char LCD[NUM_CHARS] = { 32 };
char LCD_shadow[NUM_CHARS] = { 32 };
uint8_t charIndex = 0;

void setup() {
  Serial.begin(115200);
  pinMode(RS, INPUT);
  pinMode(RW, INPUT);
  pinMode(D7, INPUT);
  pinMode(D6, INPUT);
  pinMode(D5, INPUT);
  pinMode(D4, INPUT);
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
  uint8_t data = PIND;
  rs = data & digitalPinToBitMask(RS) ? 1 : 0;
  d4 = data & digitalPinToBitMask(D4) ? 1 : 0;
  d5 = data & digitalPinToBitMask(D5) ? 1 : 0;
  d6 = data & digitalPinToBitMask(D6) ? 1 : 0;
  d7 = data & digitalPinToBitMask(D7) ? 1 : 0;

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

const uint8_t LCD_SETFUNCTION = 0x40;
const uint8_t LCD_SETFUNCTION_MASK = 0xC0;

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
  if ((command & LCD_SETFUNCTION_MASK) == LCD_SETFUNCTION) {
    gotUpNib = false;
    Serial.println("init");
    return;
  }
}
