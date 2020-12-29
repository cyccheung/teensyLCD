#include <rotary.h>
#include "Wire.h"
#include <Adafruit_LiquidCrystal.h>

#define NUMROWS 4               // LCD height
#define NUMCOLS 20              // LCD width
#define NUMMENUITEMS 6          // Number of parameters
#define MENUITEMMAXLENGTH 15    // Length of longest parameter name including null character

Adafruit_LiquidCrystal lcd(0);  // Change number if I2C address changes

char menuLabels[NUMMENUITEMS][MENUITEMMAXLENGTH];
menuLabels[0] = "STEP SIZE:";
menuLabels[1] = "NUM STEPS:";
menuLabels[2] = "TOTAL TRAVEL:";
menuLabels[3] = "EXP TIME/STEP:";
menuLabels[4] = "SCAN DIR:";
menuLabels[5] = "PIEZO TRAVEL:";

enum STATES {
  SCROLLING,  // Choosing which parameter to edit
  ADJUSTING   // Editing a parameter's value
};

int currentState = SCROLLING;
int currentSelection = 0;   // Stores which menu item currently pointing at

void setup() {
  lcd.begin(NUMCOLS, NUMROWS);
  lcd.setBacklight(HIGH);
  lcd.home();
  for(int i = 0; i < 4; ++i) {
    lcd.setCursor(0, i);
    lcd.print(menuLabels[i]);
  }
}

void loop() {
  volatile unsigned char val = r.process();

  // if the encoder has been turned, check the direction
  if (val) {
    if (val == r.clockwise()) {
      Serial.println("Clockwise");                    
    }
    else if (val == r.counterClockwise()) {
      Serial.println("Counter-Clockwise");
    }
  }

  // Check to see if the button has been pressed.
  // Passes in a debounce delay of 20 milliseconds
  if (r.buttonPressedReleased(20)) {
    Serial.println("Button pressed");
  }

  switch(state) {
  case 0:
    /* code */
    break;
  
  case 1:
    break;
  case 2:
    break;
  case 3:
    break;s
  default:
    break;
  }
}
