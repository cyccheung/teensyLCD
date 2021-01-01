#include <rotary.h>
#include "Wire.h"
#include <EEPROM.h>
#include <Adafruit_LiquidCrystal.h>

#define NUMROWS 4               // LCD height
#define NUMCOLS 20              // LCD width
#define NUMMENUITEMS 7          // Number of parameters and options
#define MENUITEMMAXLENGTH 16    // Length of longest parameter name including null character
#define PRINTDELAY 50           // ms to wait after printing
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
  // Editing a parameter's value
  ADJSTEPSIZE,
  ADJNUMSTEPS,
  ADJTOTALTRAVEL,
  ADJEXPTIME,
  ADJSCANDIR,
  ADJPIEZOTRAVEL,
  ADJSAVESETTINGS,  // State not used, for completeness only
  // Scrolling in menu
  SCROLLING
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

int state = SCROLLING;                // Start off in scrolling mode
int currentSelection = STEPSIZE;      // Stores which menu item currently pointing at
int expTimePosition = 0;              // Stores offset from start of exposure time value cells

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
  // Print out selector on first row
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
  // Passes in a debounce delay of 10 milliseconds
  // This block of code deals with state machine logic only, nothing should be printed
  // and cursor should not be moved here, only state transitions, global variable updates, and cursor blink
  if (r.buttonPressedReleased(10)) {
    switch(state) {
      case SCROLLING:
        // If selector is pointing at save settings, call save function
        if(currentSelection == SAVESETTINGS) {
          if(EEPROMENABLE) {
            saveSettings();
          }
        }
        // If selector pointing at parameters
        else {
          // If user is scrolling, send him to adjusting state for chosen parameter
          state = currentSelection;
        }
        lcd.blink();
        break;
      case ADJSTEPSIZE:
        // User is done adjusting step size, just go back to scrolling
        state = SCROLLING;
        lcd.noBlink();
        break;
      case ADJNUMSTEPS:
        // User is done adjusting num steps, just go back to scrolling
        state = SCROLLING;
        lcd.noBlink();
        break;
      case ADJTOTALTRAVEL:
        // User is done adjusting total travel, just go back to scrolling
        state = SCROLLING;
        lcd.noBlink();
        break;
      case ADJEXPTIME:
        // User is done editing current digit
        // If user still has digits to edit, increment expTimePosition and keep him in ADJEXPTIME state
        if(expTimePosition < 3) {
          state = ADJEXPTIME;
          expTimePosition++;
        }
        // If user is done editing the ones place digit, send him back to scrolling
        else {
          state = SCROLLING;
          lcd.noBlink();
          expTimePosition = 0;
        }
        break;
      case ADJSCANDIR:
        // User is done adjusting scan direction, just go back to scrolling
        state = SCROLLING;
        lcd.noBlink();
        break;
      case ADJPIEZOTRAVEL:
        // User is done adjusting piezo travel, just go back to scrolling
        state = SCROLLING;
        lcd.noBlink();
        break;
      case ADJSAVESETTINGS:
        // Should never be in this state, state exists for completeness only
        state = SCROLLING;
        lcd.noBlink();
        break;
      default:
        break;
    }
  }

  // If encoder has been turned
  if(val) {
    switch(state) {
      case SCROLLING:
        lcd.setCursor(0, currentSelection % 4); // Move cursor to selector
        lcd.noAutoscroll();                     // Left justify
        lcd.print(" ");                         // Erase selector symbol
        delay(PRINTDELAY);

        // If going from page 1 to page 2
        if(val == r.clockwise() && currentSelection == NUMROWS - 1) {
          currentSelection++;
          printPage(currentSelection);
        }
        // If going from page 2 to page 1
        else if(val == r.counterClockwise() && currentSelection == NUMROWS) {
          currentSelection--;
          printPage(currentSelection);
        }
        // If within one page
        else {
          val == r.clockwise() ? currentSelection++ : currentSelection--;
        }
        // Saturate currentSelection
        currentSelection = saturate(currentSelection, 0, NUMMENUITEMS - 1)

        lcd.setCursor(0, currentSelection % 4); // Move cursor to 0th column and chosen row
        lcd.noAutoscroll();                     // Left justify
        lcd.print(selector);                    // Print out selector symbol
        delay(PRINTDELAY);
        break;
      case ADJSTEPSIZE:
        val == r.clockwise() ? values[currentSelection] += 0.1 : values[currentSelection] -= 0.1;
        values[currentSelection] = saturate(values[currentSelection], 0.0f, 10.0f); // TODO: Replace limits with calculated limits
        printValue(currentSelection);
        // TODO: Update value of total travel and print it out
        break;
      case ADJNUMSTEPS:
        val == r.clockwise() ? values[currentSelection] += 1 : values[currentSelection] -= 1;
        values[currentSelection] = saturate(values[currentSelection], 0.0f, 10.0f); // TODO: Replace limits with calculated limits
        printValue(currentSelection);
        // TODO: Update value of total travel and print it out
        break;
      case ADJTOTALTRAVEL:
        // Should not be adjusting this value
        break;
      case ADJEXPTIME:
        // Logic edits thousands place first
        int difference = 1000;
        for(int i = 0; i < expTimePosition; ++i) {
          difference /= 10;
        }
        val == r.clockwise() ? values[currentSelection] += difference : values[currentSelection] -= difference;
        values[currentSelection] = saturate(values[currentSelection], 0.0f, 9999.0f);
        printValue(currentSelection);
        lcd.setCursor(NUMCOLS - 1 - 5 + expTimePosition);   // So cursor blinks over digit being edited
        break;
      case ADJSCANDIR:
      // Toggle between 0.0 and 1.0
        values[currentSelection] < 0.5 ? values[currentSelection] = 1.0 : values[currentSelection] = 0.0;
        break;
      case ADJPIEZOTRAVEL:
        val == r.clockwise() ? values[currentSelection] += 50.0 : values[currentSelection] -= 50.0;
        values[currentSelection] = saturate(values[currentSelection], 100.0f, 500.0f);
        printValue(currentSelection);
        break;
      case ADJSAVESETTINGS:
        break;
      default:
        break;
    }
  }
}
/*
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
  if(currentSelection == EXPTIME) {
    lcd.setCursor(NUMCOLS - 1 - parameterValueLengths[currentSelection] + expTimePosition, currentSelection);
  }
  else {
    // Move cursor back to last column on row so blinking is in the right place
    lcd.setCursor(NUMCOLS - 1, currentSelection); 
  }
}
*/
// Function to print out menu labels
// Takes in position of menu selector
void printMenuLabels(int currentSelection) {
  // Page 2
  if(currentSelection >= 4) {
    for(int i = 4; i < 7; ++i) {
      lcd.setCursor(0, i - 4);
      lcd.print(menuLabels[i]);
      delay(PRINTDELAY);
    }
  }
  // Page 1
  else {
    for(int i = 0; i < 4; ++i) {
      lcd.setCursor(0, i);
      lcd.print(menuLabels[i]);
      delay(PRINTDELAY);
    }
  }
}

// Function to print out parameter value
// Takes in position of menu selector
// Does not move cursor to where it should be next, let another function handle
void printValue(int currentSelection) {
  int printRow = currentSelection % 4;
  switch(currentSelection) {
    case STEPSIZE:
      values[currentSelection] >= 10 ? lcd.setCursor(NUMCOLS - 1 - 6, printRow) : lcd.setCursor(NUMCOLS - 1 - 5, printRow);
      lcd.print(values[currentSelection]);
      delay(PRINTDELAY);
      lcd.print("um");
      delay(PRINTDELAY);
      break;
    case NUMSTEPS:
      // TODO: See how many digits are needed
      lcd.setCursor(NUMCOLS - 1 - 6, printRow);
      lcd.print((int)values[currentSelection]);   // Cast to an int
      delay(PRINTDELAY);
      break;
    case TOTALTRAVEL:
      // TODO: See how many digits are needed
      lcd.setCursor(NUMCOLS - 1 - 6, printRow);
      lcd.print(values[currentSelection]);
      delay(PRINTDELAY);
      lcd.print("um");
      delay(PRINTDELAY);
      break;
    case EXPTIME:
      lcd.setCursor(NUMCOLS - 1 - 5, printRow);
      if(values[currentSelection] < 1000) lcd.print("0"); // Print leading zeros
      if(values[currentSelection] < 100) lcd.print("0");
      if(values[currentSelection] < 10) lcd.print("0"); 
      lcd.print((int)values[currentSelection]);
      delay(PRINTDELAY);
      lcd.print("ms");
      delay(PRINTDELAY);
      break;
    case SCANDIR:
      lcd.setCursor(NUMCOLS - 1, printRow);
      if(values[currentSelection] > 0.5) {
        lcd.print("+");
      }
      else {
        lcd.print("-");
      }
      delay(PRINTDELAY);
      break;
    case PIEZOTRAVEL:
      lcd.setCursor(NUMCOLS - 1 - 4, printRow);
      lcd.print(values[currentSelection]);
      delay(PRINTDELAY);
      lcd.print("um");
      delay(PRINTDELAY);
      break;
    case SAVESETTINGS:
      // No value to print
      break;
    default:
      break;
  }
}

// Function to print out full menu page of labels and values
// Takes in position of menu selector
void printPage(int currentSelection) {
  lcd.clear();
  printMenuLabels(currentSelection);
  if(currentSelection < 4) {
    for(int i = 0; i < 4; ++i) {
      printValue(i);
    }
  }
  else {
    for(int i = 4; i < 6; ++i) {
      printValue(i);
    }
  }
  // updateValueOnScreen(currentSelection);
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

// Function that saturates val between min and max
float saturate(float val, float minimum, float maximum) {
  val < minimum ? return minimum :
  val > maximum ? return maximum :
  return val;
}
int saturate(int val, int minimum, int maximum) {
  val < minimum ? return minimum :
  val > maximum ? return maximum :
  return val;
}
