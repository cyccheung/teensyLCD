#include <rotary.h>
#include "Wire.h"
#include <EEPROM.h>
#include <Adafruit_LiquidCrystal.h>

#define NUMROWS 4               // LCD height
#define NUMCOLS 20              // LCD width
#define NUMMENUITEMS 7          // Number of parameters and options
#define MENUITEMMAXLENGTH 16
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

// Reorder this array and MENUITEMS enum below to change menu order
char menuLabels[][MENUITEMMAXLENGTH] = {  " STEP SIZE:",
                                          " NUM STEPS:",
                                          " TOTAL TRAVEL:",
                                          " SCAN DIR:",
                                          " EXP TIME:",
                                          " PIEZO TRAVEL:",
                                          " SAVE SETTINGS"
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

// Reorder this enum and menuLabels above to change menu order
enum MENUITEMS {
  STEPSIZE,
  NUMSTEPS,
  TOTALTRAVEL,
  SCANDIR,
  EXPTIME,
  PIEZOTRAVEL,
  SAVESETTINGS
};

// Stores minimum value each parameter can be
float valuesMin[NUMMENUITEMS-1];
// Stores maximum value each parameter can be
float valuesMax[NUMMENUITEMS-1];

// Stores the values of each parameter
// Initializes them as minimum values
float values[] = {valuesMin[0], valuesMin[1], valuesMin[2], valuesMin[3], valuesMin[4], valuesMin[5]};

int state = SCROLLING;                // Start off in scrolling mode
int currentSelection = 0;             // Stores which menu item currently pointing at
int expTimePosition = 0;              // Stores offset from start of exposure time value cells

void loadSettings();
void printPage(int currentSelection);
void computeParameters();

void setup() {
  valuesMin[STEPSIZE] = 0.0f; valuesMin[NUMSTEPS] = 0.0; valuesMin[TOTALTRAVEL] = 0.0; valuesMin[EXPTIME] = 1.0;    valuesMin[SCANDIR] = 0.0; valuesMin[PIEZOTRAVEL] = 100.0;
  valuesMax[STEPSIZE] = 10.0; valuesMax[NUMSTEPS] = 0.0; valuesMax[TOTALTRAVEL] = 0.0; valuesMax[EXPTIME] = 9999.0; valuesMax[SCANDIR] = 1.0; valuesMax[PIEZOTRAVEL] = 500.0;
  values[STEPSIZE] = valuesMin[STEPSIZE]; 
  values[NUMSTEPS] = valuesMin[NUMSTEPS]; 
  values[TOTALTRAVEL] = valuesMin[TOTALTRAVEL]; 
  values[EXPTIME] = valuesMin[EXPTIME]; 
  values[SCANDIR] = valuesMin[SCANDIR]; 
  values[PIEZOTRAVEL] = valuesMin[PIEZOTRAVEL]; 
  
  lcd.begin(NUMCOLS, NUMROWS);
  lcd.setBacklight(HIGH);
  lcd.home();
  lcd.noAutoscroll();                 // Left justify
  lcd.print(selector);                // Print out selector symbol
  delay(PRINTDELAY);

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
  delay(PRINTDELAY);
  // TODO: Compute resolution, min, and max values for parameters
  computeParameters();
  // TODO: Output values to peripherals

  Serial.begin(9600);
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
    Serial.println("Button pressed");
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
          switch(currentSelection) {
            case STEPSIZE:
              state = ADJSTEPSIZE;
              break;
            case NUMSTEPS:
              state = ADJNUMSTEPS;
              break;
            case TOTALTRAVEL:
              state = ADJTOTALTRAVEL;
              break;
            case EXPTIME:
              state = ADJEXPTIME;
              break;
            case SCANDIR:
              state = ADJSCANDIR;
              Serial.println("Going to ADJSCANDIR");
//              lcd.setCursor(13, 0);
//              lcd.print("Chosen");
              break;
            case PIEZOTRAVEL:
              state = ADJPIEZOTRAVEL;
              break;
            default:
              break;
          }
          // state = currentSelection;
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
        Serial.println("Going to Scrolling");
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
//    Serial.print("Turned in state: ");
//    Serial.println(state == ADJSCANDIR);
    if(state == SCROLLING) {

    }
    else if(state == ADJSCANDIR) {
      Serial.println("Turned in adjscandir");
    }
    
    switch(state) {
      case SCROLLING:
        Serial.println("Turned in SCROLLING");
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
        currentSelection = saturate(currentSelection, 0, NUMMENUITEMS - 1);

        lcd.setCursor(0, currentSelection % 4); // Move cursor to 0th column and chosen row
        lcd.noAutoscroll();                     // Left justify
        lcd.print(selector);                    // Print out selector symbol
        delay(PRINTDELAY);
        break;
      case ADJSTEPSIZE:
        Serial.println("Turned in ADJSTEPSIZE");
        val == r.clockwise() ? values[currentSelection] += 0.1 : values[currentSelection] -= 0.1;
        values[currentSelection] = saturate(values[currentSelection], 0.0f, 10.0f); // TODO: Replace limits with calculated limits
        printValue(currentSelection);
        // TODO: Update value of total travel and print it out
        break;
      case ADJNUMSTEPS:
        Serial.println("Turned in ADJNUMSTEPS");
        val == r.clockwise() ? values[currentSelection] += 1 : values[currentSelection] -= 1;
        values[currentSelection] = saturate((int)values[currentSelection], 0, 10); // TODO: Replace limits with calculated limits
        printValue(currentSelection);
        // TODO: Update value of total travel and print it out
        break;
      case ADJTOTALTRAVEL:
        Serial.println("Turned in ADJTOTALTRAVEL");
        // Should not be adjusting this value
        break;
      case ADJEXPTIME:
        Serial.println("Turned in ADJEXPTIME");
        // Logic edits thousands place first
        int difference = 1000;
        for(int i = 0; i < expTimePosition; ++i) {
          difference /= 10;
        }
        val == r.clockwise() ? values[currentSelection] += difference : values[currentSelection] -= difference;
        values[currentSelection] = saturate(values[currentSelection], 1.0f, 9999.0f);
        printValue(currentSelection);
        // lcd.setCursor(NUMCOLS - 1 - 5 + expTimePosition, currentSelection);   // So cursor blinks over digit being edited
        break;
      case ADJSCANDIR:
        Serial.println("Turned in ADJSCANDIR");
        // Toggle between 0.0 and 1.0
        if(values[currentSelection] > 0.5f) {
          values[currentSelection] = 0.0f;
        }
        else {
          values[currentSelection] = 1.0f;
        }
        Serial.println(values[currentSelection]);
        printValue(currentSelection);
        break;
      case ADJPIEZOTRAVEL:
        Serial.println("Turned in ADJPIEZOTRAVEL");   
        val == r.clockwise() ? values[currentSelection] += 50 : values[currentSelection] -= 50;
        values[currentSelection] = saturate((int)values[currentSelection], 100, 500);
        printValue(currentSelection);
        break;
      case ADJSAVESETTINGS:
        Serial.println("Turned in ADJSAVESETTINGS");   
        break;
      default:
        Serial.println("Turned in default");
        break;
    }
    for(int i = 0; i < 6; ++i) {
      Serial.print(values[i]);
      Serial.print(" ");
    }
    Serial.println("");
    
//    Serial.println("Done switch");
  }

  // This block handles anything that needs to be done when nothing is pressed
  switch(state) {
    case ADJEXPTIME:
      lcd.setCursor(NUMCOLS - 1 - 5 + expTimePosition, currentSelection % 4);   // So cursor blinks over digit being edited
      break;
//    case ADJSCANDIR:
//      lcd.setCursor(NUMCOLS - 1 - 1, currentSelection % 4);   // So cursor blinks over +/-
//      lcd.print("FF");
//      break;
    default:
      lcd.setCursor(0, currentSelection % 4);
      break;
  }

//  Serial.println(state);
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
      lcd.setCursor(NUMCOLS - 1 - 3, printRow);
      if(values[currentSelection] < 10) {
        lcd.print(" ");
        delay(PRINTDELAY);
      }
      lcd.print((int)values[currentSelection]);   // Cast to an int
      delay(PRINTDELAY);
      break;
    case TOTALTRAVEL:
      // TODO: See how many digits are needed
      lcd.setCursor(NUMCOLS - 1 - 5, printRow);
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
      lcd.setCursor(NUMCOLS - 1 - 1, printRow);
      delay(PRINTDELAY);
//      values[currentSelection] = 1.0f;
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
      lcd.print((int)values[currentSelection]);
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
  if(val < minimum) {
    return minimum;
  }
  else if(val > maximum) {
    return maximum;
  }
  return val;
}
int saturate(int val, int minimum, int maximum) {
  if(val < minimum) {
    return minimum;
  }
  else if(val > maximum) {
    return maximum;
  }
  return val;
}
