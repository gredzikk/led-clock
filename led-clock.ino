#include <bitBangedSPI.h>
#include <Wire.h>
#include <DS3231.h>
#include "SevSegShift.h"

#define SHIFT_PIN_DS   10
#define SHIFT_PIN_STCP 8
#define SHIFT_PIN_SHCP 9

#define BUTTON_PIN A2

SevSegShift sevseg(SHIFT_PIN_DS, SHIFT_PIN_SHCP, SHIFT_PIN_STCP); //Instantiate a seven segment controller object

DS3231 zegar;

bool h12Flag = false;
bool pmFlag = false;
bool monthFlag = false;

int adjustedSeconds[] = {64, 
                          1,  2,  3,  4,  5,  6,  7,  8,
                          9,  10, 11, 12, 13, 14, 15, 16,
                          18, 19, 20, 21, 22, 23, 24, 26,
                          27, 28, 29, 30, 31, 32, 34, 35,
                          36, 37, 38, 39, 40, 42, 43, 44,
                          45, 46, 47, 48, 49, 50, 51, 52,
                          53, 54, 55, 56, 57, 58, 59, 60,
                          61, 62, 63 };
                                
bitBangedSPI bbSPI(5, bitBangedSPI::NO_PIN, 6);

const uint8_t LATCH = 7;

const byte numberOfChips = 8;
const byte maxLEDs = numberOfChips * 8;

typedef struct LedInfo
{
  int chip;
  byte mask;
} LedInfo;

bool ready = false;
short readyMillis = 200;

bool timeIsLoaded;

int keyPressed;
int screenId = 0;
int prevScreen = 0;

int date;

int monthLength[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

const int btnNONE = 0;
const int btnUP = 1;
const int btnDOWN = 2;
const int btnLEFT = 3;
const int btnRIGHT = 4;

unsigned long previousMillisButton = 0;

int readAnalogButton() {
  int val = analogRead(BUTTON_PIN);

  if (val < 100) return btnLEFT;
  if (val < 550) return btnRIGHT;
  if (val < 725) return btnUP;
  if (val < 850) return btnDOWN;

  return btnNONE;  // when all others fail, return this.
}

int timeToDisplay;

int yearToSet, monthToSet, DoWToSet, dayToSet, hourToSet, minuteToSet, secondToSet;

void lcdDisplayPage(unsigned int screenId) {
  switch (screenId) {
    case 0:
      {
        sevseg.setNumber(timeToDisplay);
        break;
      }
    case 1:
      {
        sevseg.setNumber(date);
        break;
      }
    case 2:
      {
        sevseg.setNumber(2024);
        break;
      }
    case 3:
      {
        sevseg.blank();
        break;
      }
    case 4:
      {
        if (!timeIsLoaded) {
          yearToSet = zegar.getYear();
          monthToSet = zegar.getMonth(monthFlag);
          dayToSet = zegar.getDate();
          DoWToSet = zegar.getDoW();
          hourToSet = zegar.getHour(h12Flag, pmFlag);
          minuteToSet = zegar.getMinute();
          secondToSet = zegar.getSecond();

          timeIsLoaded = true;
        }

        sevseg.setChars("SET");
        if (keyPressed == btnUP)
          saveParams();
        break;
      }
    case 5:
      {
        sevseg.setNumber(minuteToSet);
        break;
      }
    case 6:
      {
        sevseg.setNumber(hourToSet);
        break;
      }
    case 7:
      {
        sevseg.setNumber(dayToSet);
        break;
      }
    case 8:
      {
        sevseg.setNumber(monthToSet);
        break;
      }
    case 9:
      {
        sevseg.setNumber(yearToSet);
        break;
      }
  }
}

void saveParams() {
  zegar.setYear(yearToSet);
  zegar.setMonth(monthToSet);
  zegar.setDate(dayToSet);
  zegar.setDoW(DoWToSet);
  zegar.setHour(hourToSet);
  zegar.setMinute(minuteToSet);

  clearLED(0);
  setLED(adjustedSeconds[0]);
  setLED(adjustedSeconds[5]);
  setLED(adjustedSeconds[10]);
  setLED(adjustedSeconds[15]);
  setLED(adjustedSeconds[20]);
  setLED(adjustedSeconds[25]);
  setLED(adjustedSeconds[30]);
  setLED(adjustedSeconds[35]);
  setLED(adjustedSeconds[40]);
  setLED(adjustedSeconds[45]);
  setLED(adjustedSeconds[50]);
  setLED(adjustedSeconds[55]);
  refreshLEDs();
  delay(100);
  clearLED(0);

  timeIsLoaded = false;
  
}

byte LEDdata[numberOfChips] = {0}; // initial pattern

void refreshLEDs()
{
  //clearLED(0);
  digitalWrite(LATCH, LOW);
  for (int i = numberOfChips - 1; i >= 0; i--)
    bbSPI.transfer(LEDdata[i]);
  digitalWrite(LATCH, HIGH);
} // end of refreshLEDs

// turn an LED number into the position in the array, and a bit mask
boolean getChipAndBit(unsigned int led, LedInfo &ledInfo)
{
  led--;
  // divide by 8 to work out which chip
  ledInfo.chip = led / 8; // which chip
  // remainder is bit number
  ledInfo.mask = 1 << (led % 8);
} // end of getChipAndBit

// clear LED n (or all if zero)
void clearLED(const long n)
{
  // zero means all
  if (n == 0)
  {
    for (int i = 0; i < numberOfChips; i++)
      LEDdata[i] = 0;
    return;
  } // end of if zero
  
  LedInfo ledInfo;
  if (getChipAndBit(n, ledInfo))
    return; // bad number

  LEDdata[ledInfo.chip] &= ~ledInfo.mask;
} // end of clearLED

// set LED n (or all if zero)
void setLED(const long n)
{
  // zero means all
  if (n == 0)
  {
    for (int i = 0; i < numberOfChips; i++)
      LEDdata[i] = 0xFF;
    return;
  } // end of if zero

  LedInfo ledInfo;
  if (getChipAndBit(n, ledInfo))
    return; // bad number

  LEDdata[ledInfo.chip] |= ledInfo.mask;
} // end of setLED

void unready() {
  ready = false;
}

void nextPage() {
  ++screenId;
}

void prevPage() {
  --screenId;
}

void increment (int screenId) {
  switch (screenId) {
    case 5:
    {
      minuteToSet++;
      if (minuteToSet > 59)
        minuteToSet = 0;
      break;
    }
    case 6:
    {
      hourToSet++;
      if (hourToSet > 23)
        hourToSet = 0;
      break;
    }
    case 7:
    {
      dayToSet++;
      if (dayToSet > monthLength[monthToSet-1])
        dayToSet = 1;
      break;
    }
    case 8:
    {
      monthToSet++;
      if (monthToSet > 12)
        monthToSet = 1;
      break;
    }
    case 9:
    {
      yearToSet++;
      if (yearToSet > 2069)
        yearToSet = 2024;
      break;
    }
    deafult:
    {
      break;
    }
  }
}

void decrement (int screenId) {
  switch (screenId) {
    case 5:
    {
      minuteToSet--;
      if (minuteToSet < 0)
        minuteToSet = 59;
      break;
    }
    case 6:
    {
      hourToSet--;
      if (hourToSet < 0)
        hourToSet = 23;
      break;
    }
    case 7:
    {
      dayToSet--;
      if (dayToSet < 1)
        dayToSet = monthLength[monthToSet-1];
      break;
    }
    case 8:
    {
      monthToSet--;
      if (monthToSet < 1)
        monthToSet = 12;
      break;
    }
    case 9:
    {
      yearToSet--;
      if (yearToSet < 2024)
        yearToSet = 2069;
      break;
    }
    default:
    {
      break;
    }
  }
}

void setup()
{
  Serial.begin(9600);
  Serial.println("enter setup");
  bbSPI.begin();
  pinMode(LATCH, OUTPUT);

  Serial.println("Starting ...");
  Wire.begin();

  byte numDigits = 4;
  byte digitPins[] = {2, 3, 4, 1}; // of ShiftRegister(s) | 8+x (2nd Register)
  byte segmentPins[] = {7, 5, 8+6, 8+4, 8+3, 6, 8+7, 8+5}; // of Shiftregister(s) | 8+x (2nd Register)
  bool resistorsOnSegments = false; // 'false' means resistors are on digit pins
  byte hardwareConfig = P_TRANSISTORS; // See README.md for options
  bool updateWithDelays = false; // Default 'false' is Recommended
  bool leadingZeros = true; // Use 'true' if you'd like to keep the leading zeros
  bool disableDecPoint = false; // Use 'true' if your decimal point doesn't exist or isn't connected

  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments,
  updateWithDelays, leadingZeros, disableDecPoint);
  sevseg.setBrightness(1);

  yearToSet = zegar.getYear();
  monthToSet = zegar.getMonth(monthFlag);
  dayToSet = zegar.getDate();
  DoWToSet = zegar.getDoW();
  hourToSet = zegar.getHour(h12Flag, pmFlag);
  minuteToSet = zegar.getMinute();
  secondToSet = zegar.getSecond();
} // end of setup

int second, prevSecond;
int minute, prevMinute;

void loop()
{
  unsigned long currentMillis = millis();

  keyPressed = readAnalogButton();
  screenId = constrain(screenId, 0, 9);

  if (!ready && keyPressed && currentMillis - previousMillisButton >= readyMillis) {
    ready = true;
    previousMillisButton = currentMillis;
  }

  second = zegar.getSecond();
  minute = zegar.getMinute();

  switch (keyPressed) {
    case btnRIGHT:
    {
      if (ready)
        nextPage();
      unready();
      break;
    }
    case btnLEFT:
    {
      if (ready)
        prevPage();
      unready();
      break;
    }
    case btnDOWN:
    {
      if (ready)
        decrement(screenId);
      unready();
      break;
    }
    case btnUP:
    {
      if (ready)
        increment(screenId);
      unready();
      break;
    }

  }
  
  lcdDisplayPage(screenId);
  if (screenId != prevScreen) {
    prevScreen = screenId;
  }

  if (second != prevSecond) {
    if (second == 0)
      clearLED(0);
    clearLED(adjustedSeconds[prevSecond]);
    for (int i = 0; i < second; ++i)
      if (i % 5 == 0)
        setLED(adjustedSeconds[i]);
    refreshLEDs();
    setLED(adjustedSeconds[second]);
    refreshLEDs();
    prevSecond = second;
  }

  if (minute != prevMinute)
  {
    timeToDisplay = zegar.getHour(h12Flag, pmFlag) * 100 + minute;
    date = zegar.getDate() * 100 + zegar.getMonth(monthFlag);
    sevseg.setNumber(timeToDisplay);
    prevMinute = minute;
  }  

  sevseg.refreshDisplay();
} 
