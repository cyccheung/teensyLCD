/*
  Usage Notes:
  - Make sure to change EEPROMENABLE #define to 1 if testing with EEPROM
  - Resistor values can be changed by changing the R1,2,3,4 #defines
  - Pin definitions can be changed by changing their respective #defines 
  - To reorder the menu, menuLabels array and MENUITEMS enum
  - valuesMin and valuesMax store the min and max each parameter is allowed to go to
*/

#include <rotary.h>
#include "Wire.h"
#include <EEPROM.h>
#include <Adafruit_LiquidCrystal.h>

#define NUMROWS 4               // LCD height
#define NUMCOLS 20              // LCD width
#define NUMMENUITEMS 7          // Number of parameters and options
#define MENUITEMMAXLENGTH 16
#define PRINTDELAY 50           // ms to wait after printing
#define SETUPDEBUG 0            // Does not run loop if true, used to test functions in setup
#define EEPROMENABLE 0          // Disable at all times unless testing EEPROM or overall system
// EEPROM addresses
#define EEPROMSTEPSIZEADDR 0
#define EEPROMNUMSTEPSADDR 4
#define EEPROMEXPTIMEADDR 8
#define EEPROMSCANDIRADDR 12
#define EEPROMPIEZOTRAVELADDR 16
// Resistor values, make sure they are floats
#define R1 22.
#define R2 4.
#define R3 29.
#define R4 19.
#define R1R2 ((R1*R2)/(R1+R2))  // R1 // R2 parallel
#define R3R4 ((R3*R4)/(R3+R4))  // R3 // R4 parallel
// Gain values
#define G1 R3R4/R1
#define G2 R3/R1
#define G3 R3R4/R1R2
#define G4 R3/R1R2
#define STAGE2GAIN 1.0f
// Pin definitions
#define ENCODERA 5
#define ENCODERB 3
#define ENCODERBUTTON 4
#define SWD1 9
#define SWD2 10
#define SWD3 11
#define SWD4 12

Adafruit_LiquidCrystal lcd(0);  // Change number if I2C address changes
Rotary r = Rotary(ENCODERA, ENCODERB, ENCODERBUTTON);     // Encoder pin A, Encoder pin B, button pin

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

// Reorder this array and MENUITEMS enum below to change menu order
char menuLabels[][MENUITEMMAXLENGTH] = {  " STEP SIZE:",
                                          " NUM STEPS:",
                                          " TOT TRAVEL:",
                                          " EXP TIME:",
                                          " SCAN DIR:",
                                          " PIEZO TRAVEL:",
                                          " SAVE SETTINGS"
                                        };

// Reorder this enum and menuLabels above to change menu order
enum MENUITEMS {
  STEPSIZE,
  NUMSTEPS,
  TOTALTRAVEL,
  EXPTIME,
  SCANDIR,
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

void setup() {
  valuesMin[STEPSIZE] = 0.1f;     valuesMin[NUMSTEPS] = 1.0;    valuesMin[TOTALTRAVEL] = 0.0;   valuesMin[EXPTIME] = 1.0;    valuesMin[SCANDIR] = 0.0; valuesMin[PIEZOTRAVEL] = 100.0;
  valuesMax[STEPSIZE] = 1000.0f;  valuesMax[NUMSTEPS] = 1000.0; valuesMax[TOTALTRAVEL] = 100.0; valuesMax[EXPTIME] = 9999.0; valuesMax[SCANDIR] = 1.0; valuesMax[PIEZOTRAVEL] = 500.0;
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
  // Output values to peripherals
  chooseResistorCombination();
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
            lcd.setCursor(NUMCOLS - 1 - 4, currentSelection);
            lcd.print("SAVED");
            delay(PRINTDELAY);
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
              break;
            case PIEZOTRAVEL:
              state = ADJPIEZOTRAVEL;
              break;
            default:
              break;
          }
          lcd.blink();
        }
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
    if(state == SCROLLING) {
      lcd.setCursor(0, currentSelection % 4); // Move cursor to selector
      lcd.noAutoscroll();                     // Left justify
      lcd.print(" ");                         // Erase selector symbol
      delay(PRINTDELAY);
      
      // If going away from save settings, clear out last 5 cells
      if(currentSelection == SAVESETTINGS) {
        lcd.setCursor(NUMCOLS - 1 - 4, currentSelection);
        lcd.print("     ");
        delay(PRINTDELAY);
      }
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
    }
    else if(state == ADJSTEPSIZE) {
      float tempStep = 0.1;
      values[STEPSIZE] < 5.0f ? tempStep = 0.05 : tempStep = 0.1;    // So for step sizes up to a threshold, the step is smaller
      val == r.clockwise() ? values[currentSelection] += tempStep : values[currentSelection] -= tempStep;
      if(values[STEPSIZE] * values[NUMSTEPS] > valuesMax[TOTALTRAVEL]) values[currentSelection] -= tempStep;
      if(values[STEPSIZE] * values[NUMSTEPS] < valuesMin[TOTALTRAVEL]) values[currentSelection] += tempStep;
      printValue(currentSelection);
      // Update value of total travel and print it out
      values[TOTALTRAVEL] = values[STEPSIZE] * values[NUMSTEPS];
      printValue(TOTALTRAVEL);
      // Change output accordingly
      chooseResistorCombination();
    }
    else if(state == ADJNUMSTEPS) {
        val == r.clockwise() ? values[currentSelection] += 1 : values[currentSelection] -= 1;
        if(values[STEPSIZE] * values[NUMSTEPS] > valuesMax[TOTALTRAVEL]) values[currentSelection] -= 1;
        if(values[STEPSIZE] * values[NUMSTEPS] < valuesMin[TOTALTRAVEL]) values[currentSelection] += 1;
        printValue(currentSelection);
        // Update value of total travel and print it out
        values[TOTALTRAVEL] = values[STEPSIZE] * values[NUMSTEPS];
        printValue(TOTALTRAVEL);
        // Change output accordingly
        chooseResistorCombination();
    }
    else if(state == ADJTOTALTRAVEL) {
    }
    else if(state == ADJEXPTIME) {
      // Logic edits thousands place first
      int difference = 1000;
      for(int i = 0; i < expTimePosition; ++i) {
        difference /= 10;
      }
      val == r.clockwise() ? values[currentSelection] += difference : values[currentSelection] -= difference;
      values[currentSelection] = saturate(values[currentSelection], valuesMin[currentSelection], valuesMax[currentSelection]);
      printValue(currentSelection);
    }
    else if(state == ADJSCANDIR) {
      // Toggle between 0.0 and 1.0
      values[currentSelection] > 0.5f ? values[currentSelection] = 0.0f : values[currentSelection] = 1.0f;
      printValue(currentSelection);
    }
    else if(state == ADJPIEZOTRAVEL) {
      val == r.clockwise() ? values[currentSelection] += 50 : values[currentSelection] -= 50;
      values[currentSelection] = saturate((int)values[currentSelection], (int)valuesMin[currentSelection], (int)valuesMax[currentSelection]);
      printValue(currentSelection);
      valuesMax[TOTALTRAVEL] = values[PIEZOTRAVEL];
      // Change output accordingly
      chooseResistorCombination();
    }
    else if(state == ADJSAVESETTINGS) {}
    else {}    
  }

  // This block handles anything that needs to be done when nothing is pressed
  switch(state) {
    case ADJEXPTIME:
      lcd.setCursor(NUMCOLS - 1 - 5 + expTimePosition, currentSelection % 4);   // So cursor blinks over digit being edited
      break;
   case ADJSCANDIR:
     lcd.setCursor(NUMCOLS - 1 - 1, currentSelection % 4);   // So cursor blinks over +/-
     break;
    default:
      lcd.setCursor(0, currentSelection % 4);
      break;
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
      // See how many digits are needed
      lcd.setCursor(NUMCOLS - 1 - 3, printRow);
      if(values[currentSelection] < 100) {
        lcd.print(" ");
        delay(PRINTDELAY);
      }
      if(values[currentSelection] < 10) {
        lcd.print(" ");
        delay(PRINTDELAY);
      }
      lcd.print((int)values[currentSelection]);   // Cast to an int
      delay(PRINTDELAY);
      break;
    case TOTALTRAVEL:
      // See how many digits are needed
      lcd.setCursor(NUMCOLS - 1 - 7, printRow);
      if(values[currentSelection] < 100) {
        lcd.print(" ");
        delay(PRINTDELAY);
      }
      if(values[currentSelection] < 10) {
        lcd.print(" ");
        delay(PRINTDELAY);
      }
      lcd.print(values[TOTALTRAVEL]);
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
      values[currentSelection] > 0.5 ? lcd.print("+") : lcd.print("-");
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
  values[TOTALTRAVEL] = values[STEPSIZE] * values[NUMSTEPS];
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

// Function that calculates percentage of DAC being used
float calculateDACUtilization(float gain) {
  return values[TOTALTRAVEL] / values[PIEZOTRAVEL] * 10.0 / 1.195 / STAGE2GAIN / gain;
}

// Function that returns index of resistor setting that gives largest DAC utilization that is less than 100%
// Returns 1-indexed so results line up with G1-G4
int calculateBestIndex() {
  float utilizations[4] = {   calculateDACUtilization(G1),
                              calculateDACUtilization(G2),
                              calculateDACUtilization(G3),
                              calculateDACUtilization(G4),
  };
  int bestIndex = 0;
  // Find the first utilization that is less than 100%
  while(utilizations[bestIndex] >= 1.0f && bestIndex < 4) {
    bestIndex++;
  }
  if(bestIndex >= 4) return -1; // Should never return here, if so, all utilizations > 100%
  float largestUtilization = utilizations[bestIndex];
  for(int i = 0; i < 4; ++i) {
    if(utilizations[i] > utilizations[bestIndex] && utilizations[i] < 1.0f) {
      bestIndex = i;
      largestUtilization = utilizations[bestIndex];
    }
  }
  return bestIndex + 1;   // + 1 so 1 indexed
}

// Flips switch so we use the resistor combination with the largest utilization < 100%
void chooseResistorCombination() {
  int bestIndex = calculateBestIndex();
  // G1 - R3//R4 / R1
  if(bestIndex == 0) {
    digitalWrite(SWD1, LOW);
    digitalWrite(SWD2, HIGH);
  }
  // G2 - R3 / R1
  else if(bestIndex == 1) {
    digitalWrite(SWD1, LOW);
    digitalWrite(SWD2, LOW);
  }
  // G3 - R3//R4 / R1//R2
  else if(bestIndex == 2) {
    digitalWrite(SWD1, HIGH);
    digitalWrite(SWD2, HIGH);
  }
  // G4 - R3 / R1//R2
  else if(bestIndex == 3) {
    digitalWrite(SWD1, LOW);
    digitalWrite(SWD2, HIGH);
  }
}
