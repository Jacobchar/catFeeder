
// Include Libraries
#include "Arduino.h"
#include "Button.h"
#include "RTClib.h"
#include <SPI.h>
#include <TFT.h>

// Pin Definitions
#define PUSHBUTTON_1_PIN 2
#define PUSHBUTTON_2_PIN 3
#define PUSHBUTTON_3_PIN 4
#define PUSHBUTTON_4_PIN 5
#define STEPPER_MOTOR_STEP_PIN 6
#define STEPPER_MOTOR_DIR_PIN 7
#define STEPPER_MOTOR_ENABLE_PIN 8

// Motor steps per rotation
const int STEPS_PER_REV = 200;
const int STEPS_PER_QUARTER_REV = STEPS_PER_REV / 4;

// Text variables
#define TITLE_TEXT_SIZE 2
#define TIME_TEXT_SIZE 5
#define TIME_TEXT_HEIGHT 45
#define HOUR_TEXT_OFFSET 8
#define MINUTE_TEXT_OFFSET 98
#define ROTATION_PER_SERVING_TEXT_SIZE 1
#define ROTATION_TEXT_OFFSET 110
#define ROTATION_TEXT_HEIGHT 119

// LCD
#define cs 10
#define dc 9
#define rst 0

// Global variables and defines
// Object initialization
Button addRotation(PUSHBUTTON_1_PIN);
Button decrementRotation(PUSHBUTTON_2_PIN);
Button feedNow(PUSHBUTTON_3_PIN);
Button sleepScreen(PUSHBUTTON_4_PIN);
RTC_PCF8523 rtcPCF;

// Create an instance of the library
TFT TFTscreen = TFT(cs, dc, rst);

// Feeding times NOTE: For the homebrew alarms we only care about the hour/
// minute times
DateTime morningFeedingTime{2000U, 1U, 1U, 6U, 0U, 0U};
DateTime eveningFeedingTime{2000U, 1U, 1U, 17U, 6U, 0U};
DateTime midnight{2000U, 1U, 1U, 0U, 0U, 0U};
static bool catsFedMorning_ = false;
static bool catsFedEvening_ = false;

// Rotations Global
static uint8_t numRotations_ = 8U;
static const uint8_t MAX_ROTATIONS = 40U;

void setupTFT() {
  // Initialize the library
  TFTscreen.begin();

  // Clear the screen with a black background
  TFTscreen.background(0, 0, 0);

  // Set stroke color to white
  TFTscreen.stroke(255, 255, 255);

  // Print the static screen elements
  TFTscreen.setTextSize(TITLE_TEXT_SIZE);
  TFTscreen.text("Next Meal In: ", 0, 0);
  TFTscreen.setTextSize(ROTATION_PER_SERVING_TEXT_SIZE);
  TFTscreen.text("1/4 Rot. Per Serv: ", 0, ROTATION_TEXT_HEIGHT);

  // Print an initial rotation number of 0
  TFTscreen.text("08", ROTATION_TEXT_OFFSET, ROTATION_TEXT_HEIGHT);

  // Print the initial time to the screen
  DateTime now = rtcPCF.now();
  char timeString[9];
  snprintf(timeString, 9, "%0.2d:%0.2d", now.hour(), now.minute());
  TFTscreen.setTextSize(TIME_TEXT_SIZE);
  TFTscreen.text(timeString, HOUR_TEXT_OFFSET, TIME_TEXT_HEIGHT);
}

// Setup the essentials for your circuit to work. It runs first every time your
// circuit is powered with electricity.
void setup() {
  // Setup Serial which is useful for debugging
  // Use the Serial Monitor to view printed messages
  Serial.begin(9600);
  while (!Serial)
    ; // wait for serial port to connect. Needed for native USB
  Serial.println("Serial Connection Established!");

  // Initialize the push buttons
  addRotation.init();
  decrementRotation.init();
  feedNow.init();
  sleepScreen.init();

  if (!rtcPCF.begin()) {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }
  if (!rtcPCF.initialized()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtcPCF.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtcPCF.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  } else {
    DateTime now = rtcPCF.now();
    Serial.print("The Current Time is: ");
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print('/');
    Serial.print(now.year(), DEC);
    Serial.print("  ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
  }

  setupTFT();

  // Setup the stepper pins as outputs
  pinMode(STEPPER_MOTOR_STEP_PIN, OUTPUT);
  pinMode(STEPPER_MOTOR_DIR_PIN, OUTPUT);
  pinMode(STEPPER_MOTOR_ENABLE_PIN, OUTPUT);

  // Disable the A4988 chip
  digitalWrite(STEPPER_MOTOR_ENABLE_PIN, HIGH);
}

void rotateStepperMotor(uint8_t numRotations_) {
  // Set motor direction clockwise
  digitalWrite(STEPPER_MOTOR_DIR_PIN, HIGH);

  // Enable the A4988 chip
  digitalWrite(STEPPER_MOTOR_ENABLE_PIN, LOW);

  // Spin motor one rotation
  for (int x = 0; x < (STEPS_PER_QUARTER_REV * numRotations_); x++) {
    digitalWrite(STEPPER_MOTOR_STEP_PIN, HIGH);
    delayMicroseconds(2000);
    digitalWrite(STEPPER_MOTOR_STEP_PIN, LOW);
    delayMicroseconds(2000);
  }

  // Disable the A4988 chip
  digitalWrite(STEPPER_MOTOR_ENABLE_PIN, HIGH);
}

void updateTimeOnScreen(DateTime prevTime, DateTime now) {
  // Character array for time
  char timeString[4];

  // Compare the previous time to the current time to determine if the time
  // needs to be updated
  if (prevTime.hour() != now.hour()) {
    // Update hour
    TFTscreen.setTextSize(TIME_TEXT_SIZE);
    // First clear old hour
    snprintf(timeString, 4, "%0.2d:", prevTime.hour());
    TFTscreen.stroke(0, 0, 0);
    TFTscreen.text(timeString, HOUR_TEXT_OFFSET, TIME_TEXT_HEIGHT);

    // Then print new hour
    snprintf(timeString, 4, "%0.2d:", now.hour());
    TFTscreen.stroke(255, 255, 255);
    TFTscreen.text(timeString, HOUR_TEXT_OFFSET, TIME_TEXT_HEIGHT);
  }
  if (prevTime.minute() != now.minute()) {
    // Update minute
    TFTscreen.setTextSize(TIME_TEXT_SIZE);
    // First clear old minute
    snprintf(timeString, 4, "%0.2d:", prevTime.minute());
    TFTscreen.stroke(0, 0, 0);
    TFTscreen.text(timeString, MINUTE_TEXT_OFFSET, TIME_TEXT_HEIGHT);

    // Then print new minute
    snprintf(timeString, 4, "%0.2d:", now.minute());
    TFTscreen.stroke(255, 255, 255);
    TFTscreen.text(timeString, MINUTE_TEXT_OFFSET, TIME_TEXT_HEIGHT);
  }
}

void updateRotations(uint8_t rotations) {
  char numRotationString[4];

  // Erase the current number of rotations
  TFTscreen.setTextSize(ROTATION_PER_SERVING_TEXT_SIZE);
  snprintf(numRotationString, 4, "%0.2d", numRotations_);
  TFTscreen.stroke(0, 0, 0);
  TFTscreen.text(numRotationString, ROTATION_TEXT_OFFSET, ROTATION_TEXT_HEIGHT);

  numRotations_ = rotations;

  // Update the number of rotations
  snprintf(numRotationString, 4, "%0.2d", numRotations_);
  TFTscreen.stroke(255, 255, 255);
  TFTscreen.text(numRotationString, ROTATION_TEXT_OFFSET, ROTATION_TEXT_HEIGHT);
}

bool checkAlarm(DateTime now, DateTime alarmTime) {
  return ((now.hour() == alarmTime.hour()) &&
          (now.minute() == alarmTime.minute()));
}

bool clearAlarm(DateTime now, DateTime alarmTime) {
  return ((now.hour() == alarmTime.hour()) &&
          (now.minute() > alarmTime.minute()));
}

// After setup, it runs over and over again, in an eternal loop.
void loop() {
  // Get an initial reading and use this variable to store the previous time
  static DateTime prevTime = rtcPCF.now();

  // Read the current time
  DateTime now = rtcPCF.now();

  // Three states of the count down timer
  DateTime countDownTime{};
  // Midnight to 6 am
  if (!catsFedMorning_ && !catsFedEvening_) {
    countDownTime = {2000U,
                     1U,
                     1U,
                     (morningFeedingTime.hour() - now.hour()),
                     (morningFeedingTime.minute() - now.minute()),
                     0U};
    // 6 am to 5:30 pm
  } else if (catsFedMorning_ && !catsFedEvening_) {
    countDownTime = {2000U,
                     1U,
                     1U,
                     (eveningFeedingTime.hour() - now.hour()),
                     (eveningFeedingTime.minute() - now.minute()),
                     0U};
    // 5:30 pm to midnight
  } else if (catsFedEvening_) {
    countDownTime = {2000U,
                     1U,
                     1U,
                     (24U - now.hour() + morningFeedingTime.hour()),
                     (59U - now.minute() + morningFeedingTime.minute()),
                     0U};
  }

  updateTimeOnScreen(prevTime, countDownTime);

  // Increment the amount of rotations
  if (addRotation.onPress()) {
    updateRotations(numRotations_ == MAX_ROTATIONS ? MAX_ROTATIONS
                                                   : (numRotations_ + 1));
  }

  // Decrement the number of rotations
  if (decrementRotation.onPress()) {
    updateRotations(numRotations_ == 1U ? 1U : (numRotations_ - 1));
  }

  // Rotate the servo when button pressed
  if (feedNow.onPress()) {
    // Feed the cats immediately
    rotateStepperMotor(numRotations_);
  }

  // Wakeup/Sleep the TFT screen
  if (sleepScreen.onPress()) {
    // This button is currently on th esame line as the feedNow button
  }

  // Alarm check - morning
  if (!catsFedMorning_ && checkAlarm(now, morningFeedingTime)) {
    // Feed the cats
    rotateStepperMotor(numRotations_);
    catsFedMorning_ = true;
  }

  // Alarm check - evening
  if (!catsFedEvening_ && checkAlarm(now, eveningFeedingTime)) {
    // Feed the cats
    rotateStepperMotor(numRotations_);
    catsFedEvening_ = true;
  }

  // Having a delay seems to help the system not get bogged down. Since the time
  // is much less than one second though we should not skip the alarm
  delay(100);

  // Only clear the fed boolean once a minute has elapsed
  if (catsFedMorning_ && checkAlarm(now, midnight)) {
    catsFedMorning_ = false;
  }

  if (catsFedEvening_ && checkAlarm(now, midnight)) {
    catsFedEvening_ = false;
  }

  // Update old time
  prevTime = countDownTime;
}
