# Loyd15 for arduino with DSP

HW Arduino Uno R3
See www.arduuino.cc

DSP
- Using HW buy on eBay www.mcufriend.com (IDF=0x154) 240x320 pixels.
- Use routine Adafruit it should be downloaded from https://github.com/adafruit/Adafruit-GFX-Library and https://github.com/adafruit/TFTLCD-Library.

Settings DSP:
- define LCD_CS A3    // Chip Select goes to Analog 3
- define LCD_CD A2    // Command/Data goes to Analog 2
- define LCD_WR A1    // LCD Write goes to Analog 1
- define LCD_RD A0    // LCD Read goes to Analog 0
- define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin
- define YP A1  // must be an analog pin, use "An" notation!
- define XM A2  // must be an analog pin, use "An" notation!
- define YM 7   // can be a digital pin
- define XP 6   // can be a digital pin

- Consult from your hardware. SW is debugged for Identificator IDF=0x154. Chip NXP HC245 DY8400S TXD442E
- design of screen is possible change in tftHomeScreen() function

Game can do
- generate new game + check if is possible solve
- save/load with EEPROM
- solve using smart recursion

See video on https://www.youtube.com/watch?v=T8Fxc0txz_c&feature=youtu.be
