#include <rotary.h>
#include "Wire.h"
#include <Adafruit_LiquidCrystal.h>

#define NUMROWS 4               // LCD height
#define NUMCOLS 20              // LCD width
#define NUMMENUITEMS 6          // Number of parameters
#define MENUITEMMAXLENGTH 16    // Length of longest parameter name including null character
#define SETUPDEBUG 0            // Does not run loop if true

Adafruit_LiquidCrystal lcd(0);  // Change number if I2C address changes
Rotary r = Rotary(5, 3, 4);

char menuLabels[][MENUITEMMAXLENGTH] = {  " STEP SIZE:",
                                          " NUM STEPS:",
                                          " TOTAL TRAVEL:",
                                          " EXP TIME/STEP:",
                                          " SCAN DIR:",
                                          " PIEZO TRAVEL:"
                                        };

// Stores the number of characters the values of each parameter takes, including units
int parameterValueLengths[] = {5, 3, 6, 5, 1, 5};
// Stores the char arrays of units that each parameter takes
// char parameterUnits[NUMMENUITEMS][3];
char parameterUnits[][3] = {  "um",
                              "",
                              "um",
                              "ms",
                              "",
                              "um"
                            };
// Stores the values of each parameter
float values[] = {0.0, 0.0, 0.0, 0.0, 1.0, 0.0};
// Stores minimum value each parameter can be
float valuesMin[] = {0.0, 0.0, 0.0, 0.0, 0.0, 100.0};
// Stores maximum value each parameter can be
float valuesMax[] = {10.0, 0.0, 0.0, 0.0, 0.0, 500.0};

// Stores the amount that one encoder detent adjusts parameter by
float resolutions[] = { 0.1,  // Step size 
                        1.0,  // Num steps 
                        1.0,  // Total travel
                        1.0,  // Exposure time per step
                        1.0,  // Scanning direction, do not use this value
                        50.0  // Piezo travel 
                        };

char selector = '>';  // Symbol to use for selecting parameter in main menu

enum STATES {
  SCROLLING,  // Choosing which parameter to edit
  ADJUSTING   // Editing a parameter's value
};

enum MENUITEMS {
  STEPSIZE,
  NUMSTEPS,
  TOTALTRAVEL,
  EXPTIME,
  SCANDIR,
  PIEZOTRAVEL
};

int state = SCROLLING;
int currentSelection = STEPSIZE;     // Stores which menu item currently pointing at

void setup() {
  lcd.begin(NUMCOLS, NUMROWS);
  lcd.setBacklight(HIGH);
  lcd.home();
  // Print out parameter names
  for(int i = 0; i < 4; ++i) {
    lcd.setCursor(0, i);
    lcd.print(menuLabels[i]);
  }

  // TODO: Load up saved values for each parameter
  // TODO: Print out values for each parameter
  // TODO: Compute resolution, min, and max values for parameters
  // TODO: Output values to peripherals
}

void loop() {
  while(SETUPDEBUG) {
    delay(10000);
  }
  volatile unsigned char val = r.process();

  // Check to see if the button has been pressed
  // Passes in a debounce delay of 20 milliseconds
  if (r.buttonPressedReleased(20)) {
    if(state == SCROLLING) {
      state = ADJUSTING;
    }
    else if(state == ADJUSTING) {
      state = SCROLLING;
    }
  }

  // If encoder has been turned
  if(val) {
    switch(state) {
      case SCROLLING:
        lcd.setCursor(0, currentSelection); // Move cursor to 0th column and chosen row
        lcd.noAutoscroll();                 // Left justify
        lcd.print(" ");                     // Erase selector symbol
        delay(50);

        // Change which row selector symbol is on and saturate it between 0 and NUMMENUITEMS - 1
        if(val == r.clockwise()) {
          currentSelection >= NUMMENUITEMS - 1 ? currentSelection = NUMMENUITEMS - 1 : currentSelection++;
        }
        else if(val == r.counterClockwise()) {
          currentSelection <= 0 ? currentSelection = 0 : currentSelection--;
        }

        lcd.setCursor(0, currentSelection); // Move cursor to 0th column and chosen row
        lcd.noAutoscroll();                 // Left justify
        lcd.print(selector);                // Print out selector symbol
        delay(50);
        break;
      case ADJUSTING:
        switch(currentSelection) {
          // case STEPSIZE:
          //   break;
          // case NUMSTEPS:
          //   break;
          // case TOTALTRAVEL:
          //   break;
          // case EXPTIME:
          //   break;
          case SCANDIR:
            if(values[currentSelection] < 0.5) {
              values[currentSelection] = 1.0;
            }
            else {
              values[currentSelection] = 0.0;
            }
            lcd.setCursor(NUMCOLS - 1 - parameterValueLengths[currentSelection], currentSelection); // Move cursor to last column and chosen row
            values[currentSelection] < 0.5 ? lcd.print("-") : lcd.print("+"); // 0.0 = -, 1.0 = +
            break;
          // case PIEZOTRAVEL:
          //   break;
          // Assume that all parameters other than SCANDIR has resolution and saturation values
          default:
            if(val == r.clockwise()) {
              values[currentSelection] >= valuesMax[currentSelection] ? 
                values[currentSelection] = valuesMax[currentSelection] :    // Saturate at max
                values[currentSelection] += resolutions[currentSelection];  // Increment by resolution
            }
            else if(val == r.counterClockwise()) {
              values[currentSelection] <= valuesMin[currentSelection] ? 
                values[currentSelection] = valuesMin[currentSelection] :    // Saturate at min
                values[currentSelection] -= resolutions[currentSelection];  // Decrement by resolution
            }
            lcd.setCursor(NUMCOLS - 1 - parameterValueLengths[currentSelection], currentSelection); // Move cursor to correct column from the right and chosen row
            lcd.print(values[currentSelection]);          // Print out value
            lcd.print(parameterUnits[currentSelection]);  // Print out units
            break;
        }
        break;
      default:
        break;
    }
  }

  

}
