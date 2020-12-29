#define NUMROWS 4
#define NUMCOLS 20

#include "Wire.h"
#include <Adafruit_LiquidCrystal.h>

Adafruit_LiquidCrystal lcd(0);
char menu1[6] = "Item10";
char menu2[6] = "Item2";
char menu3[6] = "Items32";
char menu4[6] = "Item";
int menu1Ctr = 0;
int menu1Sat[] = {0, 3};
int menu2Ctr = 0;
int menu2Sat[] = {1, 4};
int menu3Ctr = 0;
int menu3Sat[] = {2, 5};
int menu4Ctr = 0;
int menu4Sat[] = {3, 6};
int encoderCount = 0;

void setup() {
  lcd.begin(NUMCOLS, NUMROWS);
  lcd.setBacklight(HIGH);
  lcd.home();
  lcd.print(menu1);
  lcd.setCursor(0, 1);
  lcd.print(menu2);
  lcd.setCursor(0, 2);
  lcd.print(menu3);
  lcd.setCursor(0, 3);
  lcd.print(menu4);
}

void loop() {
  switch(encoderCount % 4) {
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
