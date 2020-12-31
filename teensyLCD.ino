#include <rotary.h>
#include "Wire.h"
#include <EEPROM.h>
#include <Adafruit_LiquidCrystal.h>

#define NUMROWS 4               // LCD height
#define NUMCOLS 20              // LCD width
#define NUMMENUITEMS 7          // Number of parameters and options
#define MENUITEMMAXLENGTH 16    // Length of longest parameter name including null character
#define SETUPDEBUG 0            // Does not run loop if true
#define EEPROMENABLE 0          // Disable at all times unless testing EEPROM or overall system
// EEPROM addresses
#define EEPROMSTEPSIZEADDR 0
#define EEPROMNUMSTEPSADDR 4
#define EEPROMEXPTIMEADDR 8
#define EEPROMSCANDIRADDR 12
#define EEPROMPIEZOTRAVELADDR 16

Adafruit_LiquidCrystal lcd(0);  // Change number if I2C address changes
Rotary r = Rotary(5, 3, 4);     // Encoder pin A, Encoder pin B, button pin

char menuLabels[][MENUITEMMAXLENGTH] = {  " STEP SIZE:",
                                          " NUM STEPS:",
                                          " TOTAL TRAVEL:",
                                          " EXP TIME:",
                                          " SCAN DIR:",
                                          " PIEZO TRAVEL:",
                                          " SAVE SETTINGS"
                                        };

// Stores the number of characters the values of each parameter takes, including units
int parameterValueLengths[] = {5, 3, 6, 6, 1, 5};
// Stores the char arrays of units that each parameter takes
// char parameterUnits[NUMMENUITEMS][3];
char parameterUnits[][3] = {  "um",
                              "",
                              "um",
                              "ms",
                              "",
                              "um"
                            };
// Stores minimum value each parameter can be
float valuesMin[] = {0.0, 0.0, 0.0, 1.0, 0.0, 100.0};
// Stores maximum value each parameter can be
float valuesMax[] = {10.0, 0.0, 0.0, 9999.0, 0.0, 500.0};
// Stores the values of each parameter
// Initializes them as minimum values
float values[] = {valuesMin[0], valuesMin[1], valuesMin[2], valuesMin[3], valuesMin[4], valuesMin[5]};

// Stores the amount that one encoder detent adjusts parameter by
float resolutions[] = { 0.1,  // Step size 
                        1.0,  // Num steps 
                        1.0,  // Total travel
                        1.0,  // Exposure time
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
  PIEZOTRAVEL,
  SAVESETTINGS
};

int state = SCROLLING;
int currentSelection = STEPSIZE;      // Stores which menu item currently pointing at
int expTimePosition = 0;              // Stores offset from start of exposure time value cells
int eepromAddr = 0;

void loadSettings();
void printPage(int currentSelection);
void computeParameters();

void setup() {
  lcd.begin(NUMCOLS, NUMROWS);
  lcd.setBacklight(HIGH);
  lcd.home();
  lcd.noAutoscroll();                 // Left justify
  lcd.print(selector);                // Print out selector symbol
  delay(50);

  // Load up saved values for each parameter
  if(EEPROMENABLE) {
    loadSettings();
  }
  // Print out values for each parameter
  printPage(currentSelection);
  // Print out selector
  lcd.home();
  lcd.noAutoscroll();                 // Left justify
  lcd.print(selector);                // Print out selector symbol
  delay(50);
  // TODO: Compute resolution, min, and max values for parameters
  computeParameters();
  // TODO: Output values to peripherals
}

void loop() {
  while(SETUPDEBUG) {
    delay(10000);
  }
  volatile unsigned char val = r.process();

  // Check to see if the button has been pressed
  // Passes in a debounce delay of 20 milliseconds
  if (r.buttonPressedReleased(10)) {
    if(state == SCROLLING) {
      state = ADJUSTING;
      // Move cursor to right most spot and blink it
      lcd.setCursor(NUMCOLS - 1, currentSelection);
      lcd.blink();
    }
    // Special case if user is changing exposure time
    // Move the cursor to the left by one spot
    else if(state == ADJUSTING && currentSelection == EXPTIME && expTimePosition < 3) {
      expTimePosition++;
      lcd.setCursor(NUMCOLS - parameterValueLengths[currentSelection] + expTimePosition, currentSelection);
    }
    // User is done adjusting value
    else if(state == ADJUSTING) {
      expTimePosition = 0;  // Redundant most of the time but makes the code simpler so...
      state = SCROLLING;
      lcd.setCursor(0, currentSelection);
      lcd.noBlink();
    }
  }

  // If encoder has been turned
  if(val) {
    switch(state) {
      case SCROLLING:
        lcd.setCursor(0, currentSelection % 4); // Move cursor to 0th column and chosen row
        lcd.noAutoscroll();                 // Left justify
        lcd.print(" ");                     // Erase selector symbol
        delay(50);

        // Change which row selector symbol is on and saturate it between 0 and NUMMENUITEMS - 1
        if(val == r.clockwise() && currentSelection == 3) {
          currentSelection >= NUMMENUITEMS - 1 ? currentSelection = NUMMENUITEMS - 1 : currentSelection++;
          // Print page 2
          printPage(currentSelection);
        }
        else if(val == r.clockwise()) {
          currentSelection >= NUMMENUITEMS - 1 ? currentSelection = NUMMENUITEMS - 1 : currentSelection++;
        }
        else if(val == r.counterClockwise() && currentSelection == 4) {
          currentSelection <= 0 ? currentSelection = 0 : currentSelection--;
          // Print page 1
          printPage(currentSelection);
        }
        else if(val == r.counterClockwise()) {
          currentSelection <= 0 ? currentSelection = 0 : currentSelection--;
        }

        lcd.setCursor(0, currentSelection % 4); // Move cursor to 0th column and chosen row
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
          case EXPTIME:
            // Lets user adjust exposure time like a luggage lock
            int difference = 1000;
            for(int i = 0; i < expTimePosition; ++i) {
              difference /= 10;
            }
            if(val == r.clockwise()) {
              values[currentSelection] += difference;                       // Increment by difference
              if(values[currentSelection] >= valuesMax[currentSelection]) {
                values[currentSelection] = valuesMax[currentSelection];     // Saturate at max
              } 
            }
            else if(val == r.counterClockwise()) {
              values[currentSelection] -= difference;                       // Decrement by difference
              if(values[currentSelection] <= valuesMin[currentSelection]) {
                values[currentSelection] = valuesMin[currentSelection];     // Saturate at min
              }
            }
            updateValueOnScreen(currentSelection);
            break;
          case SCANDIR:
            if(values[currentSelection] < 0.5) {
              values[currentSelection] = 1.0;
            }
            else {
              values[currentSelection] = 0.0;
            }
            lcd.setCursor(NUMCOLS - 1 - parameterValueLengths[currentSelection], currentSelection); // Move cursor to last column and chosen row
            values[currentSelection] < 0.5 ? lcd.print("-") : lcd.print("+"); // 0.0 = -, 1.0 = +
            delay(50);
            break;
          // case PIEZOTRAVEL:
          //   break;
          case SAVESETTINGS:
            if(EEPROMENABLE) {
              saveSettings();
            }
            break;
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
            updateValueOnScreen(currentSelection);
            // lcd.setCursor(NUMCOLS - 1 - parameterValueLengths[currentSelection], currentSelection); // Move cursor to correct column from the right and chosen row
            // lcd.print(values[currentSelection]);          // Print out value
            // delay(50);
            // lcd.print(parameterUnits[currentSelection]);  // Print out units
            // delay(50);
            // lcd.setCursor(NUMCOLS - 1, currentSelection); // Move cursor back to last column on row so blinking is in the right place
            break;
        }
        break;
      default:
        break;
    }
  }
}

// Function to update value of parameter on screen
void updateValueOnScreen(int currentSelection) {
  if(currentSelection == SAVESETTINGS) { return; }
  // Move cursor to correct column from the right and chosen row
  if(currentSelection == STEPSIZE && values[currentSelection] > 9.99) {
    // If STEPSIZE is 10.00, it needs one more cell
    lcd.setCursor(NUMCOLS - 2 - parameterValueLengths[currentSelection], currentSelection);
  }
  // Print out leading zeros for EXPTIME
  else if(currentSelection == EXPTIME) {
    lcd.setCursor(NUMCOLS - 1 - parameterValueLengths[currentSelection], currentSelection);
    if(values[currentSelection] < 1000) {
      lcd.print("0");
    }
    if(values[currentSelection] < 100) {
      lcd.print("0");
    }
    if(values[currentSelection] < 10) {
      lcd.print("0");
    }
  }
  else {
    lcd.setCursor(NUMCOLS - 1 - parameterValueLengths[currentSelection], currentSelection);
  }
  if(currentSelection == EXPTIME || currentSelection == NUMSTEPS || currentSelection == PIEZOTRAVEL) {
    lcd.print((int)values[currentSelection]);          // Print out int casted value
  }
  else {
    lcd.print(values[currentSelection]);          // Print out value
  }
  delay(50);
  lcd.print(parameterUnits[currentSelection]);  // Print out units
  delay(50);
  if(currentSelection != EXPTIME) {
    // Move cursor back to last column on row so blinking is in the right place
    lcd.setCursor(NUMCOLS - 1, currentSelection); 
  }
}

// Function to print out menu labels
// Takes in position of menu selector
void printMenuLabels(int currentSelection) {
  // Page 2
  if(currentSelection >= 4) {
    for(int i = 4; i < 7; ++i) {
      lcd.setCursor(0, i - 4);
      lcd.print(menuLabels[i]);
    }
  }
  // Page 1
  else {
    for(int i = 0; i < 4; ++i) {
      lcd.setCursor(0, i);
      lcd.print(menuLabels[i]);
    }
  }
}

// Function to print out parameter values
// Takes in position of menu selector
void printValues(int currentSelection) {
  // Page 2
  if(currentSelection >= 4) {
    for(int i = 4; i < 6; ++i) {
      lcd.setCursor(0, i - 4);
      lcd.print(values[i]);
    }
  }
  // Page 1
  else {
    for(int i = 0; i < 4; ++i) {
      lcd.setCursor(0, i);
      lcd.print(values[i]);
    }
  }
}

// Function to print out full menu page of labels and values
// Takes in position of menu selector
void printPage(int currentSelection) {
  lcd.clear();
  printMenuLabels(currentSelection);
  updateValueOnScreen(currentSelection);
//  printValues(currentSelection);
}

// Compute resolution, min, and max values for following parameters:
// Total travel
void computeParameters() {

}

// Save settings to EEPROM
// Teensy does wear levelling by itself so addresses we pass in can be fixed
// Unnecessarily uses more storage for some of the values but Teensy has enough storage 
// so we can use the values[] array for convenience and ease of understanding later
void saveSettings() {
  EEPROM.put(EEPROMSTEPSIZEADDR, values[STEPSIZE]);
  EEPROM.put(EEPROMNUMSTEPSADDR, values[NUMSTEPS]);   
  EEPROM.put(EEPROMEXPTIMEADDR, values[EXPTIME]);
  EEPROM.put(EEPROMSCANDIRADDR, values[SCANDIR]);
  EEPROM.put(EEPROMPIEZOTRAVELADDR, values[PIEZOTRAVEL]);
}

// Load settings from EEPROM
void loadSettings() {
  EEPROM.get(EEPROMSTEPSIZEADDR, values[STEPSIZE]);
  EEPROM.get(EEPROMNUMSTEPSADDR, values[NUMSTEPS]);   
  EEPROM.get(EEPROMEXPTIMEADDR, values[EXPTIME]);
  EEPROM.get(EEPROMSCANDIRADDR, values[SCANDIR]);
  EEPROM.get(EEPROMPIEZOTRAVELADDR, values[PIEZOTRAVEL]);
}
