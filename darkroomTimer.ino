#include <Time.h>
#include <TimeLib.h>
#include <LiquidCrystal.h>
#include <TimeLib.h> // FIX 2018-08-12 This fixes «‘now’ was not declared in this scope» error
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

const int resetButtonPin = 6;
const int startStopButtonPin = 7;
const int downButtonPin = 8;
const int upButtonPin = 9;
const int buttonPin = 2;
const int relayPin = 4;

int buttonState = 0;

int setupHours = 0;     // How many hours will count down when started
int setupMinutes = 0;   // How many minutes will count down when started
int setupSeconds = 0;   // How many seconds will count down when started
time_t setupTime = 0;

int currentHours = 0;
int currentMinutes = 0;
int currentSeconds = 0;
time_t currentTime = 0;

time_t startTime = 0;
time_t elapsedTime = 0;

int resetButtonState = LOW;
long resetButtonLongPressCounter = 0;
int startStopButtonState = LOW;
int upButtonState = LOW;
int downButtonState = LOW;
int resetButtonPrevState = LOW;
int startStopButtonPrevState = LOW;
int relayState = LOW;
int upButtonPrevState = LOW;
int downButtonPrevState = LOW;
bool resetButtonPressed = false;
bool resetButtonLongPressed = false;
bool startStopButtonPressed = false;
bool upButtonPressed = false;
bool downButtonPressed = false;
bool relayActive = false;

const int MODE_IDLE = 0;
const int MODE_SETUP = 1;
const int MODE_RUNNING = 2;
const int MODE_RINGING = 3;

const int MODE_ON = 0;
const int MODE_OFF = 1;
int relayMode = 0;

int currentMode = MODE_IDLE;    // 0=idle 1=setup 2=running 3=ringing
                                // Power up --> idle
                                // Reset --> idle
                                //  Start/Stop --> start or stop counter
                                //  Up / Down --> NOP
                                // Reset (long press) --> enter setup
                                //   Start/Stop --> data select
                                //   Up --> increase current data value
                                //   Down --> decrease current data value
                                //   Reset --> exit setup (idle)

int dataSelection = 0;  // Currently selected data for edit (setup mode, changes with Start/Stop)
                        // 0=hours (00-99) 1=minutes (00-59) 2=seconds (00-59)

void setup() {
  // put your setup code here, to run once:
  lcd.init();
  lcd.init();
  pinMode(resetButtonPin, INPUT);
  pinMode(startStopButtonPin, INPUT);
  pinMode(upButtonPin, INPUT);
  pinMode(downButtonPin, INPUT);
  pinMode(buttonPin, INPUT);
  pinMode(relayPin, OUTPUT);  
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  startStopButtonPressed = false;
  upButtonPressed = false;
  downButtonPressed = false;
  relayActive = false;
  buttonState = digitalRead(buttonPin);

  /*
   * Relay on/off button management
   */
  if (buttonState == HIGH) {    
    digitalWrite(relayPin, LOW);    
  } else {    
    digitalWrite(relayPin, HIGH);
  }
  
  /*
   * Reset button management
   */
  resetButtonPressed = false;
  resetButtonLongPressed = false;
  resetButtonState = digitalRead(resetButtonPin);
  if(resetButtonState != resetButtonPrevState)
  {
    resetButtonPressed = resetButtonState == HIGH;
    resetButtonPrevState = resetButtonState;
  }
  else  // Long press management...
  {
    if(resetButtonState == HIGH)
    {
      resetButtonLongPressCounter++;
      if(resetButtonLongPressCounter == 20)
      {
        resetButtonPressed = false;
        resetButtonLongPressed = true;
        resetButtonLongPressCounter = 0;
      }
    }
    else
    {
      resetButtonLongPressCounter = 0;
      resetButtonPressed = false;
      resetButtonLongPressed = false;
    }
  }

  /*
   * Start/Stop button management
   */
  startStopButtonPressed = false;
  startStopButtonState = digitalRead(startStopButtonPin);
  if(startStopButtonState != startStopButtonPrevState)
  {
    startStopButtonPressed = startStopButtonState == HIGH;
    startStopButtonPrevState = startStopButtonState;
  }

  /*
   * Down button management
   */
  downButtonPressed = false;
  downButtonState = digitalRead(downButtonPin);
  if(downButtonState != downButtonPrevState)
  {
    downButtonPressed = downButtonState == HIGH;
    downButtonPrevState = downButtonState;
  }

  /*
   * Up button management
   */
  upButtonPressed = false;
  upButtonState = digitalRead(upButtonPin);
  if(upButtonState != upButtonPrevState)
  {
    upButtonPressed = upButtonState == HIGH;
    upButtonPrevState = upButtonState;
  }

  /*
   * Mode management
   */
  switch(currentMode)
  {
    case MODE_IDLE:
      if(resetButtonPressed)
      {
        Reset();
      }
      if(resetButtonLongPressed)
      {
        currentMode = MODE_SETUP;
      }
      if(startStopButtonPressed)
      {
        currentMode = currentMode == MODE_IDLE ? MODE_RUNNING : MODE_IDLE;
        if(currentMode == MODE_RUNNING)
        {
          // STARTING TIMER!
          startTime = now();
        }
      }
      break;

    case MODE_SETUP:
      if(resetButtonPressed)
      {
        // Exit setup mode
        setupTime = setupSeconds + (60 * setupMinutes) + (3600 * setupHours);
        currentHours = setupHours;
        currentMinutes = setupMinutes;
        currentSeconds = setupSeconds;
        dataSelection = 0;
        currentMode = MODE_IDLE;
      }
      if(startStopButtonPressed)
      {
        // Select next data to adjust
        dataSelection++;
        if(dataSelection == 3)
        {
          dataSelection = 0;
        }
      }
      if(downButtonPressed)
      {
        switch(dataSelection)
        {
          case 0: // seconds
            setupSeconds--;
            if(setupSeconds == -1)
            {
              setupSeconds = 59;
            }
            break;
          
          case 1: // minutes
            setupMinutes--;
            if(setupMinutes == -1)
            {
              setupMinutes = 59;
            }
            break;
          case 2: // hours
            setupHours--;
            if(setupHours == -1)
            {
              setupHours = 99;
            }
            break;
        }
      }
      if(upButtonPressed)
      {
        switch(dataSelection)
        {
          case 0: // seconds
            setupSeconds++;
            if(setupSeconds == 60)
            {
              setupSeconds = 0;
            }
            break;
          
          case 1: // minutes
            setupMinutes++;
            if(setupMinutes == 60)
            {
              setupMinutes = 0;
            }
            break;
          case 2: // hours
            setupHours++;
            if(setupHours == 100)
            {
              setupHours = 0;
            }
            break;
        }
      }
      break;
    
    case MODE_RUNNING:
      if(startStopButtonPressed)
      {
        currentMode = MODE_IDLE;
      }
      if(resetButtonPressed)
      {
        Reset();
        currentMode = MODE_IDLE;
      }
      break;
  }

  /*
   * Time management
   */
  switch(currentMode)
  {
    case MODE_IDLE:
    case MODE_SETUP:
      // NOP
      break;
    case MODE_RUNNING:
      currentTime = setupTime - (now() - startTime);
      digitalWrite(relayPin, LOW);
      if(currentTime <= 0)
      {
        digitalWrite(relayPin, HIGH);
        currentMode = MODE_IDLE;
      }
      break;
  }

  /*
   * LCD management
   */
  //lcd.clear();
  lcd.backlight();
  lcd.setCursor(0, 0);
  switch(currentMode)
  {
    case MODE_IDLE:
      lcd.print("Timer ready     ");
      lcd.setCursor(0, 1);
      lcd.print(currentHours);
      lcd.print(" ");
      lcd.print(currentMinutes);
      lcd.print(" ");
      lcd.print(currentSeconds);
      lcd.print("    ");
      break;
    case MODE_SETUP:
      lcd.print("Setup mode: ");
      switch(dataSelection)
      {
        case 0:
          lcd.print("SECS");
          break;
        case 1:
          lcd.print("MINS");
          break;
        case 2:
          lcd.print("HRS ");
          break;
      }
      lcd.setCursor(0, 1);
      lcd.print(setupHours);
      lcd.print(" ");
      lcd.print(setupMinutes);
      lcd.print(" ");
      lcd.print(setupSeconds);
      lcd.print("    ");
      break;
    case MODE_RUNNING:
      lcd.print("Counting down...");
      lcd.setCursor(0, 1);
      if(hour(currentTime) < 10) lcd.print("0");
      lcd.print(hour(currentTime));
      lcd.print(":");
      if(minute(currentTime) < 10) lcd.print("0");
      lcd.print(minute(currentTime));
      lcd.print(":");
      if(second(currentTime) < 10) lcd.print("0");
      lcd.print(second(currentTime));
      break;
  }
  delay(10);
}

void Reset()
{
  currentMode = MODE_IDLE;
  currentHours = setupHours;
  currentMinutes = setupMinutes;
  currentSeconds = setupSeconds;
}
