
// Include Libraries
#include "Arduino.h"
#include "Button.h"
#include "Wire.h"
#include "RTClib.h"
#include "Servo.h"
#include <TFT.h>
#include <SPI.h>

// Pin Definitions
#define PUSHBUTTON_1_PIN_2 2
#define PUSHBUTTON_2_PIN_2 3
#define PUSHBUTTON_3_PIN_2 4
#define SERVO360MICRO_PIN_SIG 5

// Text variables
#define TITLE_TEXT_SIZE 2
#define TIME_TEXT_SIZE 5
#define TIME_TEXT_HEIGHT 45
#define HOUR_TEXT_OFFSET 8
#define MINUTE_TEXT_OFFSET 98
#define ROTATION_PER_SERVING_TEXT_SIZE 1
#define ROTATION_TEXT_OFFSET 85
#define ROTATION_TEXT_HEIGHT 119

// Rotation variables
#define MAX_ROTATIONS 40

// LCD
#define cs 10
#define dc 9
#define rst 8

// Global variables and defines

// Object initialization
Button pushButton_1(PUSHBUTTON_1_PIN_2);
Button pushButton_2(PUSHBUTTON_2_PIN_2);
Button pushButton_3(PUSHBUTTON_3_PIN_2);
RTC_PCF8523 rtcPCF;
Servo servo360Micro;

// Create an instance of the library
TFT TFTscreen = TFT(cs, dc, rst);

// Feeding times
DateTime morningFeedingTime{2000, 1, 1, 6, 0, 0};
DateTime eveningFeedingTime{2000, 1, 1, 16, 30, 0};
static bool catsFedMorning = false;
static bool catsFedEvening = false;

// namespace feedingTime
// {
// enum Type :
// {
//   MORNING,
//   EVENING
// };
// };

// Rotations Global
static uint8_t numRotations = 0;

void setupTFT()
{
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
  TFTscreen.text("Rot.Per Serv: ", 0, ROTATION_TEXT_HEIGHT);

  // Print an initial rotation number of 0
  TFTscreen.text("00", ROTATION_TEXT_OFFSET, ROTATION_TEXT_HEIGHT);

  // Print the initial time to the screen
  DateTime now = rtcPCF.now();
  char timeString[9];
  snprintf(timeString, 9, "%0.2d:%0.2d", now.hour(), now.minute());
  TFTscreen.setTextSize(TIME_TEXT_SIZE);
  TFTscreen.text(timeString, HOUR_TEXT_OFFSET, TIME_TEXT_HEIGHT);
}

// Setup the essentials for your circuit to work. It runs first every time your circuit is powered with electricity.
void setup()
{
  // Setup Serial which is useful for debugging
  // Use the Serial Monitor to view printed messages
  Serial.begin(9600);
  while (!Serial)
    ; // wait for serial port to connect. Needed for native USB
  Serial.println("Serial Connection Established!");

  pushButton_1.init();
  pushButton_2.init();
  pushButton_3.init();
  if (!rtcPCF.begin())
  {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }
  if (!rtcPCF.initialized())
  {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtcPCF.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtcPCF.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  else
  {
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
}

void rotateServo(uint8_t numRotations)
{
  servo360Micro.attach(SERVO360MICRO_PIN_SIG); // Attach the servo to correct pin to control it.
  servo360Micro.write(180);                    // Turns servo CW in full speed. change the value in the brackets (180) to change the speed. As these numbers move closer to 90, the servo will move slower in that direction.
  delay(825 * numRotations);                   // Waits for the time specified; On rotation takes X seconds
  servo360Micro.write(90);                     // Sending 90 stops the servo
  delay(300);                                  // Delay so the servo has time to stop before we detach it
  servo360Micro.detach();                      // Release the servo to conserve power. When detached the servo will NOT hold it's position under stress.
}

void updateTimeOnScreen(DateTime prevTime, DateTime now)
{
  // Character array for time
  char timeString[4];

  // Compare the previous time to the current time to determine if the time needs to be updated
  if (prevTime.hour() != now.hour())
  {
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
  if (prevTime.minute() != now.minute())
  {
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

void updateRotations(uint8_t rotations)
{
  char numRotationString[4];

  // Erase the current number of rotations
  TFTscreen.setTextSize(ROTATION_PER_SERVING_TEXT_SIZE);
  snprintf(numRotationString, 4, "%0.2d", numRotations);
  TFTscreen.stroke(0, 0, 0);
  TFTscreen.text(numRotationString, ROTATION_TEXT_OFFSET, ROTATION_TEXT_HEIGHT);

  numRotations = rotations;

  // Update the number of rotations
  snprintf(numRotationString, 4, "%0.2d", numRotations);
  TFTscreen.stroke(255, 255, 255);
  TFTscreen.text(numRotationString, ROTATION_TEXT_OFFSET, ROTATION_TEXT_HEIGHT);
}

bool checkAlarm(DateTime now, DateTime alarmTime)
{
  return ((now.hour() == alarmTime.hour()) && (now.minute() == alarmTime.minute()) && (now.second() > alarmTime.second()))
}

bool clearAlarm(DateTime now, DateTime alarmTime)
{
  return ((now.hour() == alarmTime.hour()) && (now.minute() > alarmTime.minute()) && (now.second() == alarmTime.second()))
}

// Main logic of your circuit. It defines the interaction between the components you selected. After setup, it runs over and over again, in an eternal loop.
void loop()
{
  // Get an initial reading and use this variable to store the previous time
  static DateTime prevTime = rtcPCF.now();

  // Read the current time
  DateTime now = rtcPCF.now();

  updateTimeOnScreen(prevTime, now);

  // Increment the amount of rotations
  if (pushButton_1.onPress())
  {
    updateRotations(numRotations == MAX_ROTATIONS ? MAX_ROTATIONS : (numRotations + 1));
  }

  // Rotate the servo when button pressed
  if (pushButton_2.onPress())
  {
    // Feed the cats immediately
    rotateServo(numRotations);
  }

  // Decrement the number of rotations
  if (pushButton_3.onPress())
  {
    updateRotations(numRotations == 0U ? 0U : (numRotations - 1));
  }

  // Alarm check - morning
  if (!catsFedMorning && checkAlarm(now, morningFeedingTime))
  {
    // Feed the cats
    rotateServo(numRotations);
    catsFedMorning = true;
  }

  // Alarm check - evening
  if (!catsFedEvening && checkAlarm(now, eveningFeedingTime))
  {
    // Feed the cats
    rotateServo(numRotations);
    catsFedEvening = true;
  }

  // Having a delay seems to help the system not get bogged down. Since the time is much less than one minute though we should not skip the alarm
  delay(250);

  // Only clear the fed boolean once a minute has elapsed
  if (catsFedMorning && clearAlarm(now, morningFeedingTime))
  {
    catsFedMorning = false;
  }

  if (catsFedEvening && clearAlarm(now, eveningFeedingTime))
  {
    catsFedEvening = false;
  }

  // Update old time
  prevTime = now;
}

// HERE FOR TESTING
// if (prevTime.second() != now.second())
// {
//   // Update second
//   TFTscreen.setTextSize(TIME_TEXT_SIZE);
//   // First clear old second
//   snprintf(timeString, 4, "%0.2d", prevTime.second());
//   TFTscreen.stroke(0, 0, 0);
//   TFTscreen.text(timeString, MINUTE_TEXT_OFFSET, TIME_TEXT_HEIGHT);

//   // Then print new second
//   snprintf(timeString, 4, "%0.2d", now.second());
//   TFTscreen.stroke(255, 255, 255);
//   TFTscreen.text(timeString, MINUTE_TEXT_OFFSET, TIME_TEXT_HEIGHT);
// }

//  static int servoOnTime = 2000;
//    // If button 1 is pushed increment the longevity of the servo rotation
//    if (pushButton_1.onPress()) {
//      servoOnTime += 1000;
//      Serial.print("Servo Time is now: ");
//      Serial.println(servoOnTime, DEC);
//    }
//
//    // If button 2 is pushed decrement the longevity of the servo rotation
//    if (pushButton_2.onPress()) {
//      servoOnTime -= 1000;
//      Serial.print("Servo Time is now: ");
//      Serial.println(servoOnTime, DEC);
//    }
//
//    // If button 3 is pushed print the delay and rotate the servo CW and then CCW for the delay on time amount
//    if (pushButton_3.onPress()) {
//      Serial.print("Servo Time is: ");
//      Serial.println(servoOnTime, DEC);
//      Serial.println("Rotating ClockWise");
//      servo360Micro.write(180);
//      delay(servoOnTime);
//      Serial.println("Rotating CounterClockWise");
//      servo360Micro.write(0);
//      delay(servoOnTime);
//      // Stop the servo
//      servo360Micro.write(90);
//      delay(10);
//      Serial.println("Stopping Servo");
//    }
//    if(menuOption == '1') {
//    // Mini Pushbutton Switch #1 - Test Code
//    //Read pushbutton state.
//    //if button is pressed function will return HIGH (1). if not function will return LOW (0).
//    //for debounce funtionality try also pushButton_1.onPress(), .onRelease() and .onChange().
//    //if debounce is not working properly try changing 'debounceDelay' variable in Button.h
//    bool pushButton_1Val = pushButton_1.read();
//    Serial.print(F("Val: ")); Serial.println(pushButton_1Val);
//
//    }
//    else if(menuOption == '2') {
//    // Mini Pushbutton Switch #2 - Test Code
//    //Read pushbutton state.
//    //if button is pressed function will return HIGH (1). if not function will return LOW (0).
//    //for debounce funtionality try also pushButton_2.onPress(), .onRelease() and .onChange().
//    //if debounce is not working properly try changing 'debounceDelay' variable in Button.h
//    bool pushButton_2Val = pushButton_2.read();
//    Serial.print(F("Val: ")); Serial.println(pushButton_2Val);
//
//    }
//    else if(menuOption == '3') {
//    // Mini Pushbutton Switch #3 - Test Code
//    //Read pushbutton state.
//    //if button is pressed function will return HIGH (1). if not function will return LOW (0).
//    //for debounce funtionality try also pushButton_3.onPress(), .onRelease() and .onChange().
//    //if debounce is not working properly try changing 'debounceDelay' variable in Button.h
//    bool pushButton_3Val = pushButton_3.read();
//    Serial.print(F("Val: ")); Serial.println(pushButton_3Val);
//
//    }
//    else if(menuOption == '4') {
//    // Adafruit PCF8523 Real Time Clock Assembled Breakout Board - Test Code
//    //This will display the time and date of the RTC. see RTC.h for more functions such as rtcPCF.hour(), rtcPCF.month() etc.
//    DateTime now = rtcPCF.now();
//    Serial.print(now.month(), DEC);
//    Serial.print('/');
//    Serial.print(now.day(), DEC);
//    Serial.print('/');
//    Serial.print(now.year(), DEC);
//    Serial.print("  ");
//    Serial.print(now.hour(), DEC);
//    Serial.print(':');
//    Serial.print(now.minute(), DEC);
//    Serial.print(':');
//    Serial.print(now.second(), DEC);
//    Serial.println();
//    delay(1000);
//    }
//    else if(menuOption == '5') {
//    // Continuous Rotation Micro Servo - FS90R - Test Code
//    // The servo will rotate CW in full speed, CCW in full speed, and will stop  with an interval of 2000 milliseconds (2 seconds)
//    servo360Micro.attach(SERVO360MICRO_PIN_SIG);         // 1. attach the servo to correct pin to control it.
//    servo360Micro.write(180);  // 2. turns servo CW in full speed. change the value in the brackets (180) to change the speed. As these numbers move closer to 90, the servo will move slower in that direction.
//    delay(2000);                              // 3. waits 2000 milliseconds (2 sec). change the value in the brackets (2000) for a longer or shorter delay in milliseconds.
//    servo360Micro.write(0);    // 4. turns servo CCW in full speed. change the value in the brackets (0) to change the speed. As these numbers move closer to 90, the servo will move slower in that direction.
//    delay(2000);                              // 5. waits 2000 milliseconds (2 sec). change the value in the brackets (2000) for a longer or shorter delay in milliseconds.
//    servo360Micro.write(90);    // 6. sending 90 stops the servo
//    delay(2000);                              // 7. waits 2000 milliseconds (2 sec). change the value in the brackets (2000) for a longer or shorter delay in milliseconds.
//    servo360Micro.detach();                    // 8. release the servo to conserve power. When detached the servo will NOT hold it's position under stress.
//    }
//
//    if (millis() - time0 > timeout)
//    {
//        menuOption = menu();
//    }

// snprintf(timeString, 9, "%0.2d:%0.2d:%0.2d", now.hour(), now.minute(), now.second());

// TFTscreen.stroke(255, 255, 255);
// TFTscreen.text(timeString, 8, 45);
// delay(250);
// TFTscreen.stroke(0, 0, 0);
// TFTscreen.text(timeString, 8, 45);