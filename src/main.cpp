//  src/main.cpp
//  Copyright (c) 2026 by Darin W Dugger
//  Created by Darin W Dugger
//  Written by Darin W Dugger
//  noone else to blame but Darin W Dugger
//  All rights reserved.
//  This code is licensed under the MIT License (MIT).
//  See LICENSE file for license information.
//
//
//  WunderClock is a project to create a clock with various features such as weather, astronomy, and more.
//  This file contains the main code for the WunderClock project, including setup and loop functions,
//  as well as various helper functions and global variables.
//  The code uses various libraries for functionality such as WiFi management, weather data retrieval, and display control.
//  The code is written in C++ and is intended to run on an ESP32-S3 microcontroller using the Arduino framework.
//  Varios assumptions are made RTC < NTP < GNSS for time sources, and BME280 is used for
//  temperature, humidity, and pressure data.
//  The code also includes functionality for over-the-air updates, IR remote control, and more.
//  The code is still in development and may contain bugs or incomplete features, but it serves as a starting point
//  for the WunderClock project.
//  Google Maps Timezone API: https://developers.google.com/maps/documentation/timezone/overview
//  OpenWeather One Call API: https://openweathermap.org/api/one-call-api
//  Both are free to a certain number of calls per day, which should be more than sufficient for this project.
//
//
//  Let's get this Furshlugginer staeted!!!
//  Klaatu Barada Nikto.
//  Epp Opp Ork Ah-Ah.
//  That means I love you.
//  Thank you for supporting Open Source projects like this one.
//  If you find this code useful, please consider donating to support the project and its continued development.
//
//
//
/*****************************************************************************
*************************    DEBUG/COMPILER DEFINES    ***********************
*****************************************************************************/

#define FIRST_RUN // This define is really only for first compile. User will need to comment out and recompile to run the clock.

#define TESTING // Define for testing

#ifdef TESTING
#define DEBUG1 // Uncomment to enable debugging
//  #define DEBUG2 // Uncomment to turn on WiFiManager.resetSettings()
//  #define DEBUG3 // Uncomment to turn on info debugging is NTPSetup
//  #define DEBUG4 // Uncomment to turn on frame time in noise and video tests
// Add other debug defines as needed
#endif

//  #define PANEL1 // If defined, use one 64x64 panel, else use two 64x64 panels for 128 W x 64 H
#define SWAP // Uncomment to swap red and blue colors

#ifdef DEBUG1
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#define debughex(x, y) Serial.print(x, y)
#define debuglnhex(x, y) Serial.println(x, y)
#else
#define debug(x)
#define debugln(x)
#define debughex(x, y)
#define debuglnhex(x, y)
#endif

#define u8_t uint8_t
#define u16_t uint16_t
#define u32_t uint32_t
#define s8_t int8_t
#define s16_t int16_t
#define s32_t int32_t

#define NTP_SERVER "pool.ntp.org"

#define SEALEVELPRESSURE_HPA 1013.25 // Sea level pressure in hPa

#define MOON_SYNODIC_PERIOD 29.530588853 // Period of moon cycle in days.
#define MOON_SYNODIC_OFFSET 2451550.26   // Reference cycle offset in days.
#define MOON_DISTANCE_PERIOD 27.55454988 // Period of distance oscillation
#define MOON_DISTANCE_OFFSET 2451562.2
#define MOON_LATITUDE_PERIOD 27.212220817 // Latitude oscillation
#define MOON_LATITUDE_OFFSET 2451565.2
#define MOON_LONGITUDE_PERIOD 27.321582241 // Longitude oscillation
#define MOON_LONGITUDE_OFFSET 2451555.8

#define FRAM_I2C_ADDRESS 0x50 // I2C address for the Adafruit FRAM

/*****************************************************************************
******************************  INCLUDES  ************************************
*****************************************************************************/

#include <Arduino.h>
#include <Wire.h>                    // Core library - I2C
#include <LittleFS.h>                // Core library - File system
#include <RTClib.h>                  // https://github.com/adafruit/RTClib.git
#include <ezTime.h>                  // https://github.com/ropg/ezTime.git
#include <Adafruit_BME280.h>         // https://github.com/adafruit/Adafruit_BME280_Library.git
#include <WiFiManager.h>             // https://github.com/tzapu/WiFiManager.git
#include <Adafruit_FRAM_I2C.h>       // https://github.com/adafruit/Adafruit_FRAM_I2C.git
#include <SparkFun_u-blox_GNSS_v3.h> // https://github.com/sparkfun/SparkFun_u-blox_GNSS_v3.git
#include <Adafruit_Protomatter.h>    // https://github.com/adafruit/Adafruit_Protomatter.git
#include <WiFiManager.h>             // https://github.com/tzapu/WiFiManager.git

#include "rgbColors.h"       // Color definitions
#include "ir.h"              // IR remote control handling
#include "Secret-API-Keys.h" // API keys and secrets
//  #include "OpenWeatherDefault.h"      // Default values for OpenWeather

/*****************************************************************************
******************************  PROTOTYPES  **********************************
*****************************************************************************/

// put function declarations here:
bool blindSetup(void);
void matrixErrorPrint(const char *text);
bool setupFRAM(void);
bool setupDisplayMatrix(void);
bool setupRTC(void);
bool writeBlocks(void);
bool readBlocks(void);
void setupPControl(void);
bool bBlockRead(void);
bool bBlockWrite(void);

/*****************************************************************************
******************************  GLOBALS  *************************************
*****************************************************************************/
// Way to many, will try slim it down.

bool statusBlindSetup = false;

const u16_t MatrixWidth = 128;
const u16_t MatrixHeight = 64;
// defines for Video tests and noise
#define MAX_DIMENSION ((MatrixWidth > MatrixHeight) ? MatrixWidth : MatrixHeight)
u16_t noise[MAX_DIMENSION][MAX_DIMENSION];
static uint16_t x;
static uint16_t y;
static uint16_t z;
bool SwapPallet = false; // Swap the color palette for the video tests
u32_t ESPSketchSize = 0; // Size of the sketch in bytes, used for calculating free space for OTA updates

DateTime NOW;
u8_t pControlSubProgramTotal[10] = {0, 0, 0, 0, 2, 0, 0, 0, 0, 0}; // Array to hold the total number of sub programs
                                                                   // other then default for each program mode

u32_t TZADDR = 0x400;          // addr for TZAPI
u32_t pControlADDR = 0x600;    // addr for program control block
u32_t bBlockADDR = 0x800;      // addr for BBlock
u32_t dControlADDR = 0xA00;    // addr for dControl
u32_t cBlockADDR = 0xC00;      // addr for CBlock
u32_t firstBlockADDR = 0x1000; // addr for captured data block

/***************************************************************************
*******************************  STRUCTS  **********************************
***************************************************************************/

struct programControl // Control struct for program flow
{
  u8_t program = 0;              //  Mode for loop() 1=Clock,2=Weather,3=Astro,4=GNSS,5=Startup,6=System,7=Video,8=Tester,9=Reset,0=setup
  bool programActive = false;    // Flag to indicate if a program is active
  u8_t subProgram = 0;           // Sub program for loop() if needed
  bool subProgramActive = false; // Flag to indicate if a sub program is active
  u8_t subProgramTotal = 0;      // Total number of sub programs for the current program
  u8_t irMenuLevel = 0;          // Level of the IR menu, 0 is main menu
};

struct WCcontrol // Control struct for WunderClock
{
  bool bme280Active = true;                                     // True if BME280 is Active
  bool rtcActive = true;                                        // True if RTC is Active
  bool ntpActive = true;                                        // True if NTP is Active
  bool wifiActive = true;                                       // True if WiFi is Active
  bool gnssActive = true;                                       // True if GNSS is Active
  bool owocActive = true;                                       // True if OpenWeatherMap is Active
  bool timezoneActive = true;                                   // True if Timezone is Active or UTC is used
  bool displayTZLocal = true;                                   // what TZ to display true = local, false = other TZ
  bool irActive = true;                                         // True if IR receiver is Active
  bool astroActive = true;                                      // True if Astro data is Active
  bool dstActive = true;                                        // True if location observes Daylight Saving Time
  bool dstCurrent = false;                                      // True if Daylight Saving Time is currently in effect
  bool googleTZAPIActive = true;                                // True to use Google Timezone API
  bool otaActive = true;                                        // True to enable Over The Air updates
  bool digitalClock = true;                                     // false = analog, true = digital
  bool unitsImperial = true;                                    // false = metric/Celsius, true = imperial/Fahrenheit
  bool clock24hr = false;                                       // false = 12 hour, true = 24 hour
  bool menuRotate = false;                                      // false = no menu rotation, true = rotate menu
  u8_t matrixBrightness = 128;                                  // Brightness of matrix 1-255
  u8_t programMode = 1;                                         // Mode for loop() 1=Clock,2=Weather,3=Astro,4=GNSS,5=Startup,6=System,7=Video,8=Tester,9=Reset,0=setup
  char timeZoneId[17] = "America/Denver";                       // Timezone name for ezTime
  s32_t rawOffset = -25200;                                     // Offset in seconds from UTC, default is -25200 for Mountain Standard Time
  s32_t dstOffset = 3600;                                       // Daylight Saving Time offset in seconds, default is 3600 for Mountain Daylight Time
  float gmtOffset = -7.0;                                       // GMT offset in hours
  double latitude = 43.837;                                     // Latitude in degrees
  double longitude = -103.624;                                  // Longitude in degrees
  double elevation = 1982;                                      // Elevation in meters
  u8_t fontChoiceClock = 1;                                     // Font choice for clock display
  u16_t videoScale = 20;                                        // Scale factor for video tests, 1-4k
  u16_t videoSpeed = 10;                                        // Speed of video tests, 1-100
  u16_t menuRotateTime = 600;                                   // Time to rotate menu in seconds
  u8_t menuRotateArray[11] = {1, 2, 3, 4, 5, 6, 7, 0, 0, 0, 0}; // Array for menu rotation
};

struct WCboot // Boot status struct for WunderClock
{

  double permTempHi = 0;        // Permanent temperature record
  double permTempLo = 0;        // Permanent temperature record
  double permHumidityHi = 0;    // Permanent humidity record
  double permHumidityLo = 0;    // Permanent humidity record
  double permPressureHi = 0;    // Permanent pressure record
  double permPressureLo = 0;    // Permanent pressure record
  double permServiceTempHi = 0; // Permanent service temperature record
  double permServiceTempLo = 0; // Permanent service temperature record
  u32_t lastBlockADDR = 0;      // Last time location was written to FRAM
  u32_t bootCount = 0;          // Number of boots
  u8_t bootMode = 0;            // 0 = factory reset, 1 = regular boot, 2 = settings
};

struct WCboot bBlock;               // Boot status for WunderClock
struct programControl pControl[10]; // Array to hold the program control structures for each program mode
struct WCcontrol cBlock;            // Control block for WunderClock

/*****************************************************************************
******************************     INIT     **********************************
*****************************************************************************/
// Objects declaration and initialization
Adafruit_FRAM_I2C fram = Adafruit_FRAM_I2C();  // Create the FRAM object
GFXcanvas16 canvas(MatrixWidth, MatrixHeight); // Create a canvas or copy for the display
RTC_DS3231 RTClock;                            // Create a RTC object I2C

// Protomatter matrix create
#ifdef SWAP
u8_t rgbPins[] = {40, 41, 42, 37, 39, 38}; // Red, Blue swapped
#else
u8_t rgbPins[] = {42, 41, 40, 38, 39, 37}; // Normal order
#endif
u8_t addrPins[] = {45, 36, 48, 35, 21};
u8_t clockPin = 2;
u8_t latchPin = 47;
u8_t oePin = 14;
// Using five bit planes for smoother display
Adafruit_Protomatter matrix(
    MatrixWidth, 5, 1, rgbPins, 5, addrPins, clockPin, latchPin, oePin, true);

u16_t *videoBufferM = matrix.getBuffer();
u16_t *videoBufferC = canvas.getBuffer();

// Task tShowMatrix(TASK_SECOND, TASK_FOREVER, &showMatrixTask); // Task to show the matrix display
// Task tShowMatrix(1000, TASK_FOREVER, &showMatrixTask); // Task to show the matrix display

/*****************************************************************************
*****************************  SETUP  ****************************************
*****************************************************************************/

void setup()
{
  // put your setup code here, to run once:
  if (!blindSetup())
  { // this should hang in funtion but if not hang here
    matrixErrorPrint("Blind setup failed");
    while (1)
      ;
  }

  setupPControl();

#ifdef FIRST_RUN
  writeBlocks();
  matrixErrorPrint("First run: wrote blocks");
  while (1)
    ; // Hang after first run to ensure blocks are written
#endif

  bBlockRead(); // Read the boot and control blocks from FRAM

  if (bBlock.bootMode == 0)
  {
    // Perform setup for boot mode 0

    bBlock.bootMode = 1; // Set to regular boot after initial setup
    writeBlocks();       // Save the updated boot mode to FRAM
  }

  // so gnss, nts, and other services can be initialized here if needed

  // im gona set up IR before the check to see if a setup is needed

} // End setup()

void loop()
{
  // put your main code here, to run repeatedly:
}

/*****************************************************************************
***********************     TURN BACK!     ***********************************
*******************     HERE BE DRAGONS!!!!!     *****************************
*****************************************************************************/

// put function definitions here:

bool blindSetup(void)
{
  if (!setupDisplayMatrix())
  { // Can't send error message to display as no display is available
    // matrixErrorPrint("Display matrix setup failed");
    while (1)
      ;
  }

  if (!setupFRAM())
  {
    matrixErrorPrint("FRAM setup failed");
    while (1)
      ;
  }

  if (!setupRTC())
  {
    matrixErrorPrint("RTC setup failed");
    while (1)
      ;
  }

  return true;
} // End blindSetup()

bool setupRTC()
{

  if (!RTClock.begin())
  {
    debugln("RTC begin failed");
    return false;
  }
  debugln("RTC begin successful");

  // Check if RTC is valid if not set to compile time so we have valid time
  NOW = RTClock.now();
  if (NOW.isValid())
  {
    debugln("RTC is valid");
  }
  else
  {
    debugln("RTC is NOT valid setting to compile time");
    RTClock.adjust(DateTime(F(__DATE__), F(__TIME__))); // sets RTC to compile time
    NOW = RTClock.now();                                // set time now that we know RTC is valid
  }

  // Add code to initialize RTC
  return true; // Return true if setup is successful
} // end setupRTC()

void matrixErrorPrint(const char *text)
{
  matrix.fillScreen(0);
  matrix.setFont();
  matrix.setTextSize(1);
  matrix.setTextWrap(false);
  matrix.setRotation(0);
  matrix.setTextColor(WHITE, BLACK);
  matrix.setCursor(2, 16);
  matrix.print(printf("Error: %s", text));
  matrix.show();
} // End matrixErrorPrint()

bool setupFRAM(void)
{
  // Start the LittleFS file system
  if (!LittleFS.begin(true)) // If LittleFS failed to start
  {
    debugln("LittleFS failed to start");
    matrixErrorPrint("LittleFS failed to start"); // Print to display
    while (1)                                     // can not recover from LittleFS failure so hang forever.
      ;
  }
  else
  {
    debugln("LittleFS started successfully");
  }

  // begin FRAM
  if (!fram.begin(FRAM_I2C_ADDRESS)) // Initialize the FRAM
  {
    debugln("FRAM not found");
    matrixErrorPrint("FRAM not found"); // Print to display
    while (1)                           // can not recover from FRAM failure so hang forever.
      ;
  }
  else
  {
    debugln("FRAM found");
  }
  // Add code to initialize FRAM
  return true; // Return true if setup is successful
} // end setupFRAM()

bool setupDisplayMatrix()
{
  debugln("Display matrix setup");

  ProtomatterStatus status = matrix.begin();
  if (status != PROTOMATTER_OK)
  {
    debugln("Protomatter begin failed");
    return false;
  }
  debugln("Protomatter begin successful");

  // setupColors(CBlock.MatrixBrightness / 255.0); // Set up the colors for the matrix
  // setupColors(cBlock.matrixBrightness / 255.0); // Set up the colors for the matrix

  matrix.setFont(); // Use built-in font
  matrix.fillScreen(0);
  matrix.setTextSize(1);
  matrix.setTextWrap(false);
  matrix.setRotation(0);
  matrix.setTextColor(NIGHT1, BLACK);

  matrix.setCursor(2, 24);
  matrix.print("Klaatu");
  matrix.setCursor(12, 36);
  matrix.print("Barada");
  matrix.setCursor(22, 48);
  matrix.print("Nikto");
  matrix.show();

  debugln("Display matrix setup complete");
  return true;
} // end setupDisplayMatrix()

void setupPControl()
{
  // Add code to set up the program control structure and initialize the program modes
  for (int i = 0; i < 10; i++)
  {
    pControl[i].program = i;           // Set all program modes to inactive
    pControl[i].programActive = false; // Set all program modes to inactive
    pControl[i].subProgram = 0;        // Initialize other fields as needed
    pControl[i].subProgramActive = false;
    pControl[i].subProgramTotal = pControlSubProgramTotal[i];
  }

  return;
} // End setupPControl()

bool writeBlocks()
{
  // Add code to update the CBlock, DBlock, and BBlock structures in FRAM
  fram.write(cBlockADDR, (u8_t *)&cBlock, sizeof(cBlock));       // Write the CBlock structure to FRAM
  fram.write(bBlockADDR, (u8_t *)&bBlock, sizeof(bBlock)); // Write the BBlock structure to FRAM


  return true; // Return true if update is successful
} // end writeBlocks()

bool bBlockRead()
{
  fram.read(bBlockADDR, (u8_t *)&bBlock, sizeof(bBlock)); // Read the BBlock structure from FRAM
  return true;
} // end bBlockRead()

bool bBlockWrite()
{
  fram.write(bBlockADDR, (u8_t *)&bBlock, sizeof(bBlock)); // Write the BBlock structure to FRAM
  return true;
} // end bBlockWrite()

bool readBlocks()
{
  // Add code to update the CBlock, DBlock, and BBlock structures in FRAM
  fram.read(cBlockADDR, (u8_t *)&cBlock, sizeof(cBlock));       // Read the CBlock structure from FRAM
  fram.read(bBlockADDR, (u8_t *)&bBlock, sizeof(bBlock)); // Read the BBlock structure from FRAM


  return true; // Return true if update is successful
} // end readBlocks()