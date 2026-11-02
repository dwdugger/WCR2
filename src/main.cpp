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

// #define FIRST_RUN // This define is really only for first compile. User will need to comment out and recompile to run the clock.

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

#define MatrixWidth 128
#define MatrixHeight 64
#define MAX_DIMENSION ((MatrixWidth > MatrixHeight) ? MatrixWidth : MatrixHeight)

/*****************************************************************************
******************************  INCLUDES  ************************************
*****************************************************************************/

#include <Arduino.h>
#include <Wire.h>                    // Core library - I2C
#include <LittleFS.h>                // Core library - File system
#include <Preferences.h>             // Core library - NVS Preferences
#include <RTClib.h>                  // https://github.com/adafruit/RTClib.git
#include <ezTime.h>                  // https://github.com/ropg/ezTime.git
#include <Adafruit_BME280.h>         // https://github.com/adafruit/Adafruit_BME280_Library.git
#include <WiFiManager.h>             // https://github.com/tzapu/WiFiManager.git
#include <Adafruit_FRAM_I2C.h>       // https://github.com/adafruit/Adafruit_FRAM_I2C.git
#include <SparkFun_u-blox_GNSS_v3.h> // https://github.com/sparkfun/SparkFun_u-blox_GNSS_v3.git
#include <Adafruit_Protomatter.h>    // https://github.com/adafruit/Adafruit_Protomatter.git
#include <WiFiManager.h>             // https://github.com/tzapu/WiFiManager.git
#define IR_RECEIVE_PIN A4            // ****Pin for IR receiver. Required for TinyIRReceiver.hpp below
#include <TinyIRReceiver.hpp>        // https://github.com/Arduino-IRremote/Arduino-IRremote.git
#include <TaskScheduler.h>           // https://github.com/arkhipenko/TaskScheduler.git
#include <ArduinoJson.h>             // https://github.com/bblanchon/ArduinoJson.git
#include <ElegantOTA.h>              // https://github.com/ayushsharma82/ElegantOTA.git
#include <SiderealPlanets.h>         // https://github.com/DavidArmstrong/SiderealPlanets.git
#include <MoonRise.h>                // https://github.com/signetica/MoonRise.git
#include <SunRise.h>                 // https://github.com/signetica/SunRise.git

#include <WiFiClientSecure.h> // Core library - Secure WiFi client for HTTPS requests
#include <HTTPClient.h>       // Core library - HTTP client

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
bool setupGNSS(void);
bool setupOTA(void);
bool setupNTP(void);
bool writeBlocks(void);
bool readBlocks(void);
void setupPControl(void);
bool bootBlockRead(void);
bool bootBlockWrite(void);
bool readBME280(void);
bool setupWiFi(void);
bool setupTaskScheduler(void);
bool readGNSS(u8_t wait = 30);
bool readGoogleTZAPI(void);
void setupColors(float factor);               // Function to adjust color brightness
u16_t RGB24To565(u8_t *tmpRGB, float factor); // Function to convert 24-bit RGB to 16-bit RGB565 with brightness factor

bool readMoonPhase(time_t time);

void programClock(bool is24hr);
void programStartupDisplay();
void programAstroData();
void programWeather();
void programTester();
void programSystemStatus();


void irLoop();
void irMenu(u8_t menuNumber);
void irMenu00();
void irMenu01();

/*****************************************************************************
******************************  GLOBALS  *************************************
*****************************************************************************/
// Way to many, will try slim it down.

u16_t noise[MAX_DIMENSION][MAX_DIMENSION];
static uint16_t x;
static uint16_t y;
static uint16_t z;
bool SwapPallet = false; // Swap the color palette for the video tests
u32_t ESPSketchSize = 0; // Size of the sketch in bytes, used for calculating free space for OTA updates

DateTime NOW;
DateTime gnssNOW;
DateTime ntpNOW;

u8_t pControlSubProgramTotal[10] = {0, 0, 0, 0, 2, 0, 0, 0, 0, 0}; // Array to hold the total number of sub programs
                                                                   // other then default for each program mode

u32_t TZADDR = 0x400;          // addr for TZAPI
u32_t pControlADDR = 0x600;    // addr for program control block
u32_t bootBlockADDR = 0x800;   // addr for bootBlock
u32_t dControlADDR = 0xA00;    // addr for dControl
u32_t cBlockADDR = 0xC00;      // addr for CBlock
u32_t firstBlockADDR = 0x1000; // addr for captured data block

bool wifiAlive = false;
time_t wifiLastUpdate = 0; // Timestamp of the last WiFi update
bool ntpAlive = false;
bool otaAlive = false; // Flag to indicate if OTA is active
time_t ntpLastUpdate = 0; // Timestamp of the last NTP update
bool googleTZAPIAlive = false;
bool googleTZAPICurrent = false;
time_t googleTZAPILastUpdate = 0; // Timestamp of the last Google TZ API update

bool irStatus = true;
bool irRepeat = false;

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
  u32_t lastBlockADDR = 0x1000; // Last time location was written to FRAM
  u32_t bootCount = 0;          // Number of boots
  u8_t bootMode = 0;            // 0 = factory reset, 1 = regular boot, 2 = settings
};

struct gnssData
{
  double latitude = 0;  // Latitude in degrees
  double longitude = 0; // Longitude in degrees
  double elevation = 0; // Elevation in meters
  u8_t SIV = 0;         // Satellites in view
  u16_t year = 0;       // GNSS year
  u8_t month = 0;       // GNSS month
  u8_t day = 0;         // GNSS day
  u8_t hour = 0;        // GNSS hour
  u8_t minute = 0;      // GNSS minute
  u8_t second = 0;      // GNSS second
  bool alive = false;
  bool current = false;
  time_t lastUpdate = 0; // Timestamp of the last GNSS update
};

struct irResult
{
  u8_t address;
  u8_t command;
  u8_t flags;
};

struct weatherData // Weather data struct
{
  long dayTime = 0;       // Daytime timestamp
  float temp = 0;         // Temperature
  float feelsLike = 0;    // Feels like temperature
  float humidity = 0;     // Humidity
  float hPaPressure = 0;  // Pressure
  float inHgPressure = 0; // Pressure in inches of mercury
  float mmHgPressure = 0; // Pressure in millimeters of mercury
  float windSpeed = 0;    // Wind speed
  float windBearing = 0;  // Wind direction in degrees
  float windGust = 0;     // Wind gust speed
  float pop = 0;          // Probability of precipitation
  /*
  float rain = 0;        // Rain volume
  float snow = 0;        // Snow volume
  float uvi = 0;         // UV index
  u8_t clouds = 0;       // Cloud cover percentage
  u8_t weatherId = 0;    // Weather condition ID
  String weatherIcon;    // Weather icon

  String weatherMain;        // Main weather condition
  String weatherDescription; // Weather description
  */
};

struct moonPhase
{
  double jDate = 0;            // Julian date
  double phase = 0;            // Phase of the moon (0-1)
  double age = 0;              // Age of the moon (days)
  double fraction = 0;         // Fraction of the moon illuminated
  double distance = 0;         // Distance from earth (radii)
  double latitude = 0;         // Latitude of the moon
  double longitude = 0;        // Longitude of the moon
  const char *phaseName = "";  // Name of the phase
  const char *zodiacName = ""; // Name of the zodiac sign
};

struct dailyWeather
{
  float tempMin = 0;   // Minimum temperature
  float tempMax = 0;   // Maximum temperature
  float humidity = 0;  // Humidity
  float pressure = 0;  // Pressure
  float windSpeed = 0; // Wind speed
  float windDeg = 0;   // Wind direction in degrees
  float windGust = 0;  // Wind gust speed
  float rain = 0;      // Rain volume
  float snow = 0;      // Snow volume
  float uvi = 0;       // UV index
  u8_t clouds = 0;     // Cloud cover percentage
  u8_t weatherId = 0;  // Weather condition ID
  String weatherIcon;  // Weather icon

  String weatherMain;        // Main weather condition
  String weatherDescription; // Weather description
};

struct WCdata // Captured data struct for WunderClock
{
  DateTime timeOfRead;        // Time of last read of weather data
  float outdoorTemp = 0;      // Outdoor temperature
  float outdoorHumidity = 0;  // Outdoor humidity
  float localPressure = 0;    // Outdoor pressure
  float servicePressure = 0;  // Service pressure
  float serviceTemp = 0;      // Service temperature
  u32_t nextLocationADDR = 0; // Next location update ADDR
};

struct bme280Data
{
  double temp_C;         // Temperature from the BME280 in Celsius
  double temp_F;         // Temperature from the BME280 in Fahrenheit
  double pressure_hPa;   // Pressure from the BME280 in hPa
  double pressure_inHg;  // Pressure from the BME280 in inches of mercury
  double pressure_mmHg;  // Pressure from the BME280 in mm of mercury
  double humidity;       // Humidity from the BME280 in %
  double altitude;       // Altitude calculated from the BME280 in meters
  double elevation;      // Elevation from settings in meters
  bool alive = false;    // Status of the BME280 sensor
  time_t lastUpdate = 0; // Timestamp of the last update from the BME280 sensor
};

struct bme280Data bmeData;          // BME280 sensor data
struct irResult irCurrent;          // Current IR command
struct irResult irLast;             // Last IR command
struct gnssData gBlock;             // GNSS data block
struct WCboot bootBlock;            // Boot status for WunderClock
struct programControl pControl[10]; // Array to hold the program control structures for each program mode
struct WCcontrol cBlock;            // Control block for WunderClock
struct moonPhase moonPhaseData; // Moon phase data

// Names of lunar phases
/*static const char *phaseNames[] = {"New", "Evening Crescent", "First Quarter",
                                   "Waxing Gibbous", "Full", "Waning Gibbous",
                                   "Last Quarter", "Morning Crescent"};*/

static const char *phaseNames[] = {"New Moon", "Eve Cres", "1st Qtr",
                                   "Wax Gibb", "Full Moon", "Wan Gibb",
                                   "3rd Qtr", "Morn Cres"};
// Names of Zodiac constellations
static const char *zodiacNames[] = {"Taurus", "Gemini", "Cancer",
                                    "Leo", "Virgo", "Libra", "Scorpio", "Sagittarius",
                                    "Capricorn", "Aquarius", "Pisces", "Aries"};
// Ecliptic angles of Zodiac constellations
static const float zodiacAngles[] = {33.18, 51.16, 93.44, 119.48, 135.30, 173.34,
                                     224.17, 242.57, 271.26, 302.49, 311.72, 348.58};

/*****************************************************************************
******************************     INIT     **********************************
*****************************************************************************/
// Objects declaration and initialization
Adafruit_FRAM_I2C fram = Adafruit_FRAM_I2C();  // Create the FRAM object
GFXcanvas16 canvas(MatrixWidth, MatrixHeight); // Create a canvas or copy for the display
RTC_DS3231 RTClock;                            // Create a RTC object I2C
Preferences pref;                              // Preferences object
SFE_UBLOX_GNSS gnss;                           // The gnss object
Timezone myTZ;                                 // Timezone object
Adafruit_BME280 bme;                           // I2C
WiFiManager wm;                                // WiFiManager object
JsonDocument doc;                              // JSON document for parsing
Scheduler taskS;                               // TaskScheduler object
AsyncWebServer server(80);                     // Create an AsyncWebServer object on port 80
WiFiClientSecure client;

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
#ifdef DEBUG1
  // Initialize Serial port
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  delay(2000); // comment out, delay to allow time for serial to start
  while (!Serial)
  {
    digitalWrite(LED_BUILTIN, 1); // Blink while waiting for Serial
    delay(100);
    digitalWrite(LED_BUILTIN, 0);
  }
  digitalWrite(LED_BUILTIN, 0);

  debugln("\nSerial is ready");
#endif

  // put your setup code here, to run once:
  if (!blindSetup()) // setup rtc, display, FRAM and littleFS file system
  {                  // this should hang in funtion but if not hang here
    matrixErrorPrint("Blind setup failed");
    while (1)
      ;
  }
  debugln("Blind setup complete");
  setupPControl();

#ifdef FIRST_RUN
  writeBlocks();
  matrixErrorPrint("First run: wrote blocks");
  debugln("First run complete");
  while (1)
    ; // Hang after first run to ensure blocks are written
#endif

  bootBlockRead(); // Read the boot block from FRAM

  if (bootBlock.bootMode == 0)
  {
    // Perform setup for boot mode 0
    matrixErrorPrint("Factory reset in progress");
    debugln("Performing factory reset");

    bootBlock.bootMode = 1; // Set to regular boot after initial setup

    if (!pref.begin("eeprom", false)) // Start Preferences with namespace "eeprom" and readOnly = false
    {
      debugln("factory reset of  NVS failed");
      matrixErrorPrint("NVS factory reset failed.");
      while (1) // no need to recover from NVS failure as we can not save settings without NVS, so hang forever.
        ;
    }
    else
    {
      debugln("NVS factory reset successful");
    }

    if (!pref.clear())
    {
      debugln("Failed to clear all preferences in NVS");
      matrixErrorPrint("Failed to clear NVS");
      while (1)
        ;
    } // Clear all preferences in NVS
    else
    {
      debugln("Cleared all preferences in NVS successfully");
    }

    pref.end(); // Close Preferences to ensure data is written

    wm.resetSettings(); // Reset WiFiManager
    //  myTZ.clearCache();   // Clear timezone cache

    // Add any other settings resets here as needed
    writeBlocks(); // Update blocks to save factory reset status

    delay(3000); // Delay to ensure cache is cleared before writing

    // esp_restart(); // Restart the ESP to complete factory reset

    esp_sleep_enable_timer_wakeup(1); // Wake up after 1 microsecond
    esp_deep_sleep_start();           // Enter deep sleep to complete factory reset

  } // end if (bootBlock.bootMode == 0) factory reset

  // we have now handled the factory reset and initial setup, proceed with other initializations
  // time to read settings from FRAM and initialize peripherals accordingly
  readBlocks(); // Read the configuration blocks from FRAM

  if (cBlock.irActive)
  {
    initPCIInterruptForTinyReceiver(); // Start the PCI interrupt for the IR receiver using IR_RECIVE_PIN
  }

  // check to see if setup menu is needed
  if (bootBlock.bootMode == 2)
  {
    // Perform setup for boot mode 2 (setup menu)
    cBlock.programMode = 0;
    return;
  }

  // gnss, nts, and other services can be initialized here if needed

  if (cBlock.gnssActive)
  {
    debugln("Initializing GNSS");
    if (setupGNSS()) // Initialize GNSS if active
    {
      debugln("GNSS setup successful");
      gBlock.alive = true;
    }
    if (gBlock.alive)
    {
      if (readGNSS())
      {
        gBlock.current = true;
      }
    }
  }

  if (cBlock.wifiActive)
  {
    if (setupWiFi())
    {
      wifiAlive = true;
    }
  }

  if (WiFi.status() == WL_CONNECTED && cBlock.googleTZAPIActive)
  {
    if (readGoogleTZAPI()) // Initialize Google Timezone API if active
    {
      googleTZAPIAlive = true;
    }
  }

  if (WiFi.status() == WL_CONNECTED && cBlock.ntpActive)
  {
    if (setupNTP())
    {
      ntpAlive = true;
    }
  }

  if (WiFi.status() == WL_CONNECTED && googleTZAPIAlive && ntpAlive)
  {
    // set up local TZ
    if (myTZ.setLocation(cBlock.timeZoneId))
    {
      debugln("Local timezone set successfully");
      myTZ.setDefault();
    }
    else
    {
      debugln("Failed to set local timezone");
    }
  }

  if (WiFi.status() == WL_CONNECTED && googleTZAPIAlive && gBlock.alive)
  {
    readGNSS(3); // Read the latest GNSS data before setting the time

    if (gnss.getDateValid() && gnss.getTimeValid())
    {
      // Set the local time based on GNSS date and time
      UTC.setTime(gnss.getHour(), gnss.getMinute(), gnss.getSecond(), gnss.getDay(), gnss.getMonth(), gnss.getYear());
      RTClock.adjust(UTC.now());
    }
  }

  if (cBlock.bme280Active)
  {
    if (bme.begin(BME280_ADDRESS, &Wire)) // Start the BME280 for temperature, humidity and pressure.
    {
      bmeData.alive = true;
    }
    if (bmeData.alive)
    {
      readBME280(); // Read the latest BME280 sensor data
    }
  }

  if (cBlock.otaActive)
  {
  if (setupOTA())
  {
    otaAlive = true;
    debugln("OTA setup successful"); // Set up the web server if active
  }
  }

  debug("Size of DateTime: ");
  debugln(sizeof(DateTime));
  debug("Size of time_t: ");
  debugln(sizeof(time_t));
  debug("Size of myTZ: ");
  debugln(sizeof(myTZ));
  debug("Size of bootBlock: ");
  debugln(sizeof(bootBlock));
  debug("Size of cBlock: ");
  debugln(sizeof(cBlock));
  debug("Size of int: ");
  debugln(sizeof(int));
  debug("Size of long: ");
  debugln(sizeof(long));
  debug("Size of short: ");
  debugln(sizeof(short));
  debug("Size of float: ");
  debugln(sizeof(float));
  debug("Size of double: ");
  debugln(sizeof(double));
  debug("Size of pControl: ");
  debugln(sizeof(pControl));
  debug("Size of gBlock: ");
  debugln(sizeof(gBlock));
  debug("Size of weatherData: ");
  debugln(sizeof(weatherData));
  debug("Size of WCdata: ");
  debugln(sizeof(WCdata));
  debug("Size of moonPhase: ");
  debugln(sizeof(moonPhase));
  debug("Size of dailyWeather: ");
  debugln(sizeof(dailyWeather));
  debug("Size of irResult: ");
  debugln(sizeof(irResult));
  debug("Size of bmeData: ");
  debugln(sizeof(bmeData));

  /////////////////////////
  debugln("Setup complete");

} // End setup()

void loop()
{
  // put your main code here, to run repeatedly:
  if (cBlock.irActive)
    irLoop(); // Process IR input each loop iteration


  // put your main code here, to run repeatedly:
  //
  //
  u8_t progMode = cBlock.programMode;
  // cBlock.programMode = 3;

  if (bootBlock.bootMode == 1)
  {
    taskS.execute(); // Execute scheduled tasks
   // events();     // Call the events function to handle any events that have occurred
    // irMenu(pControl[cBlock.programMode].irMenuLevel); // Handle the IR menu system, pass in the current menu level from the control block
    ElegantOTA.loop(); // Handle Over The Air (OTA) updates

    /*
    // Check Local Time to see if it is housekeeping time
    if (!HouseKeeping)
    {
      if (myTZ.hour() == 3 && myTZ.minute() == 15 && myTZ.isAM()) // 3:15 AM local time
      {
        debugln("Housekeeping time, do something here");
        updateBlocks(); // Update the data block in the FRAM with the new values
        debugln("Blocks updated");
        // Do something here, like update the NTP server or do some other housekeeping task
        // or just do nothing and let the clock run
        HouseKeeping = true;
      }
    }
    else if (myTZ.hour() == 3 && myTZ.minute() == 30 && myTZ.isAM()) // 3:30 AM local time
    {
      HouseKeeping = false; // Reset the housekeeping flag
      debugln("Housekeeping done, reset flag");
    }

    if (cBlock.wifiActive && wifiAlive && (WiFi.status() != WL_CONNECTED))
    {
      debugln("WiFi NOT connected");
      wm.autoConnect(); // Attempt to reconnect to WiFi if it is not connected
    }
  }
*/
  switch (progMode)
  {
  case 0:
    // debug("Menu Level: ");
    // debugln(menuLevel);
    // setupWunderClock_Menu(menuLevel); // Main setup menu
    programClock(cBlock.clock24hr); // Main clock program, pass in true for 24 hour clock, false for 12 hour clock

    break;
  case 1:
    programClock(cBlock.clock24hr); // Main clock program, pass in true for 24 hour clock, false for 12 hour clock
    break;
  case 2:
    programWeather();
    break;
  case 3:
    programAstroData();
    break;
  case 4:
    programSystemStatus();
    break;
  case 5:
    programStartupDisplay();
    break;
  case 6:
    programTester();
    break;
  case 7:
    // videoTests_Menu();
    programClock(cBlock.clock24hr); // Main clock program, pass in true for 24 hour clock, false for 12 hour clock

    break;
  case 8:
    // tester2();
    programClock(cBlock.clock24hr); // Main clock program, pass in true for 24 hour clock, false for 12 hour clock

    break;
  case 9:
    // controledRestart();
    programClock(cBlock.clock24hr); // Main clock program, pass in true for 24 hour clock, false for 12 hour clock

    break;
  default:
    break;
  }

  ////////////////////////////////////////////////
  // delay(1000); // Delay for 1 second before next update

  irCurrent.command = 0; // Clear the current IR command after processing










  matrix.fillScreen(0);
  matrix.setCursor(0, 16); // Move cursor to the first line for UTC time
  matrix.print(UTC.dateTime(ISO8601).c_str());
  matrix.setCursor(0, 32); // Move cursor to next line for additional info
  matrix.print(myTZ.dateTime(ISO8601).c_str());
  matrix.setCursor(0, 48); // Move cursor to next line for additional info

  // matrix.print("Loop running");
  matrix.show();
} //End if (bootBlock.bootMode == 1)
} // End loop()

/*****************************************************************************
***********************     TURN BACK!     ***********************************
*******************     HERE BE DRAGOONS!!!!     *****************************
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
  matrix.print(text);
  // debugln(text);
  // char buffer[64];
  // sprintf(buffer, "%s /n", text);
  // matrix.print(buffer);
  // matrix.print(printf("Error: "));
  // matrix.print("Error: ");

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
  setupColors(cBlock.matrixBrightness / 255.0); // Set up the colors for the matrix

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
  // Add code to update the CBlock, DBlock, and bootBlock structures in FRAM
  fram.write(cBlockADDR, (u8_t *)&cBlock, sizeof(cBlock));          // Write the CBlock structure to FRAM
  fram.write(bootBlockADDR, (u8_t *)&bootBlock, sizeof(bootBlock)); // Write the bootBlock structure to FRAM

  return true; // Return true if update is successful
} // end writeBlocks()

bool bootBlockRead()
{
  fram.read(bootBlockADDR, (u8_t *)&bootBlock, sizeof(bootBlock)); // Read the bootBlock structure from FRAM
  return true;
} // end bootBlockRead()

bool bootBlockWrite()
{
  fram.write(bootBlockADDR, (u8_t *)&bootBlock, sizeof(bootBlock)); // Write the bootBlock structure to FRAM
  return true;
} // end bootBlockWrite()

bool readBlocks()
{
  // Add code to update the CBlock, DBlock, and bootBlock structures in FRAM
  fram.read(cBlockADDR, (u8_t *)&cBlock, sizeof(cBlock));          // Read the CBlock structure from FRAM
  fram.read(bootBlockADDR, (u8_t *)&bootBlock, sizeof(bootBlock)); // Read the bootBlock structure from FRAM

  return true; // Return true if update is successful
} // end readBlocks()

bool setupGNSS()
{
  // Add code to initialize the GNSS module if active
  debugln("GNSS setup");

  // Initialize GNSS
  if (gnss.begin() == false)
  {
    debugln("GNSS not detected at default I2C address. Please check wiring.");
    return false;
  }
  debugln("GNSS detected!");

  // Set GNSS configuration settings
  gnss.setI2COutput(COM_TYPE_UBX); // Set the GNSS to output UBX only (turn off NMEA noise)
  gnss.setNavigationFrequency(1);  // Set output to 1 Hz

  return true; // Return true if setup is successful
} // end setupGNSS()

bool readGNSS(u8_t wait)
{
  // Read GNSS data
  u32_t end = millis() + (wait * 1000);

  while (millis() < end)
  {
    gnss.checkUblox(); // See if new data is available. Process bytes as they come in.

    if (gnss.getLatitude() && gnss.getLongitude() && gnss.getAltitude() && gnss.getDateValid() && gnss.getTimeValid() && (gnss.getSIV() > 0))
    {
      gBlock.latitude = gnss.getLatitude() / 10000000.0;   // Convert to Degrees
      gBlock.longitude = gnss.getLongitude() / 10000000.0; // Convert to Degrees
      gBlock.elevation = gnss.getAltitudeMSL() / 1000.0;   // Convert to Meters above mean sea level
      gBlock.SIV = gnss.getSIV();                          // Number of satellites in view
      cBlock.latitude = gBlock.latitude;
      cBlock.longitude = gBlock.longitude;
      cBlock.elevation = gBlock.elevation;
      gBlock.year = gnss.getYear();
      gBlock.month = gnss.getMonth();
      gBlock.day = gnss.getDay();
      gBlock.hour = gnss.getHour();
      gBlock.minute = gnss.getMinute();
      gBlock.second = gnss.getSecond();
      gnssNOW = DateTime(gBlock.year, gBlock.month, gBlock.day, gBlock.hour, gBlock.minute, gBlock.second);
      gBlock.lastUpdate = UTC.now();

      debugln("gnss date & time valid");

      debug("GNSS Latitude: ");
      debugln(gBlock.latitude);
      debug("GNSS Longitude: ");
      debugln(gBlock.longitude);
      debug("GNSS Elevation: ");
      debugln(gBlock.elevation);
      debug("SIV: ");
      debugln(gBlock.SIV);
      debug("GNSS Date: ");
      debug(gBlock.year);
      debug("-");
      debug(gBlock.month);
      debug("-");
      debugln(gBlock.day);
      debug("GNSS Time: ");
      debug(gBlock.hour);
      debug(":");
      debug(gBlock.minute);
      debug(":");
      debugln(gBlock.second);

      return true; // Return true if read is successful
    }
  }
  debugln("GNSS read failed after timeout");
  return false; // Return false if read failed after timeout
} // end readGNSS()

bool setupWiFi()
{
  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
#ifdef DEBUG2
  wm.resetSettings();
#endif
#ifdef DEBUG1
  wm.setDebugOutput(true);
#else
  wm.setDebugOutput(false);
#endif

  wm.setConfigPortalTimeout(180);
  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("WunderClock", "password"); // password protected ap
  if (!res)
  {
    debugln("Failed to connect to WiFi and hit timeout");
    return false;
  }
  return true; // Return true if WiFi setup is successful
} // end setupWiFi()

bool readGoogleTZAPI()
{
  // Implement the logic to read Google Timezone API
  // Return true if successful, false otherwise

  // Implement the actual API call here
  // If successful, set googleTZAPICurrent to true

  String URL = "https://maps.googleapis.com/maps/api/timezone/json?location=" + String(cBlock.latitude) + "," + String(cBlock.longitude) + "&timestamp=" + String(NOW.unixtime()) + "&key=" + KeyGoogleMapsAPI; // Google Maps API URL
  debugln(URL);

  WiFiClientSecure client;
  HTTPClient http; // Create an HTTP client object

  http.useHTTP10(true); // Use HTTP/1.0 for the request
  client.setInsecure(); // Disable SSL certificate verification for testing purposes

  http.setTimeout(10000); // Set the timeout for the HTTP request to 10 seconds

  http.begin(client, URL); // Start the HTTP client with the URL

  int httpCode = http.GET(); // Send the HTTP GET request and get the response code

  if (WiFi.status() == WL_CONNECTED)
  {
    debugln("WiFi connectedTZAPI");
    debug("HTTP code: ");
    debugln(httpCode); // Print the HTTP response code to the serial monitor
  }
  else
  {
    debugln("Must set clock manually");
    return false;
  }

  // Parse JSON object
  DeserializationError error = deserializeJson(doc, http.getStream()); // Read the response and parse it as a JSON object
  if (error)
  {
    debug(F("deserializeJson() failed: "));
    debugln(error.f_str());
    client.stop(); // Stop the HTTP client
    http.end();    // End the HTTP client
    return false;
  }

  debugln("JSON parsed successfully");
  // Print the JSON object to the serial monitor
  cBlock.dstOffset = doc["dstOffset"].as<int32_t>();      // Get the dstOffset value from the JSON object
  cBlock.rawOffset = doc["rawOffset"].as<int32_t>();      // Get the rawOffset value from the JSON object
  String timeZoneId = doc["timeZoneId"].as<String>();     // Get the timeZoneId value from the JSON object
  String timeZoneName = doc["timeZoneName"].as<String>(); // Get the timeZoneName value from the JSON object

  timeZoneId.toCharArray(cBlock.timeZoneId, 17); // Convert the time

  debugln("Response:");
  debugln(doc["rawOffset"].as<int32_t>()); // Print the rawOffset value to the serial monitor
  debugln(doc["dstOffset"].as<int32_t>());
  debugln(doc["timeZoneId"].as<String>());
  debugln(doc["timeZoneName"].as<String>());
  debugln("Timezone ID: " + String(cBlock.timeZoneId));

  client.stop(); // Stop the HTTP client
  http.end();    // End the HTTP client

  return true;

} // end readGoogleTZAPI()

bool setupOTA(void)
{
  // Implement OTA setup logic here
  // Return true if successful, false otherwise
  // Add code to set up the web server and define the routes for the web interface
  ElegantOTA.begin(&server);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "This is Wunderclock!"); });

  server.begin(); // Start the web server  url is http://<IP_ADDRESS>/ and OTA updates are at http://<IP_ADDRESS>/update

  return true; // Return true if setup is successful
} // End setupOTA()

void setupColors(float factor)
{ // Change brightness in the colors array
  int i;

  for (i = 0; i < 8; i++)
  {
    colors[i] = RGB24To565(colorRGB[i], factor);
  }
  // Change brightness in individual labled colors
  BLUE = RGB24To565(rgbBLUE, factor);
  RED = RGB24To565(rgbRED, factor);
  GREEN = RGB24To565(rgbGREEN, factor);
  CYAN = RGB24To565(rgbCYAN, factor);
  MAGENTA = RGB24To565(rgbMAGENTA, factor);
  YELLOW = RGB24To565(rgbYELLOW, factor);
  WHITE = RGB24To565(rgbWHITE, factor);

  NIGHT1 = RGB24To565(rgbNIGHT1, factor);
  NIGHT2 = RGB24To565(rgbNIGHT2, factor);
  DAY1 = RGB24To565(rgbDAY1, factor);
  DAY2 = RGB24To565(rgbDAY2, factor);
  MYYELLOW1 = RGB24To565(rgbMYYELLOW1, factor);
  MYYELLOW2 = RGB24To565(rgbMYYELLOW2, factor);

  debugln("SetupColors");
  // matrix.setTextColor(WHITE); // Set text color to adjusted white to match brightness
  return;
} // End setupColors()

u16_t RGB24To565(u8_t *tmpRGB, float factor)
{
  return ((int)round(tmpRGB[0] * factor) & 0x00f8) << 8 | ((int)round(tmpRGB[1] * factor) & 0x00fc) << 3 | (int)round(tmpRGB[2] * factor) >> 3;
} // End RGB24To565()

bool setupNTP(void)
{
  // Implement NTP setup logic here
  // Return true if successful, false otherwise
  setServer(NTP_SERVER);
  setInterval(0);
  updateNTP();
  if (timeStatus() == timeSet)
    return true;
  return false;
} // End setupNTP()

void irLoop()
{
  /******************************************************************
   * IR setup code
   ******************************************************************/

  if (TinyReceiverDecode())
  {
    irCurrent.address = TinyIRReceiverData.Address;
    irCurrent.command = TinyIRReceiverData.Command;
    irCurrent.flags = TinyIRReceiverData.Flags; // 0 = OK, 1 = Repeat flag set, 4 = Parity failed

    if (irCurrent.command && (irCurrent.flags != IRDATA_FLAGS_IS_REPEAT) && (irCurrent.flags != IRDATA_FLAGS_PARITY_FAILED))
    {
      // If the command is not a repeat and not a parity failed, then we have a new command
      irStatus = true;  // Set the IR status to true
      irRepeat = false; // Set the IR repeat to false
    }
    else if (irCurrent.command && (irCurrent.flags == IRDATA_FLAGS_IS_REPEAT) && (irCurrent.flags != IRDATA_FLAGS_PARITY_FAILED))
    {
      // If the command is not a repeat, then we have a new command
      irStatus = true; // Set the IR status to true
      irRepeat = true; // Set the IR repeat to false
    }
    else if (irCurrent.command && irCurrent.flags != IRDATA_FLAGS_IS_REPEAT && irCurrent.flags == IRDATA_FLAGS_PARITY_FAILED)
    {
      // If the command is not a repeat, then we have a new command
      irStatus = false; // Set the IR status to false
      irRepeat = false; // Set the IR repeat to false
    }
    else if (irCurrent.command && (irCurrent.flags & IRDATA_FLAGS_IS_REPEAT) && (irCurrent.flags & IRDATA_FLAGS_PARITY_FAILED))
    {
      // If the command is both a repeat and has parity failed, then we have an invalid command
      irStatus = false; // Set the IR status to false
      irRepeat = true;  // Set the IR repeat to true
    }

    irLast = irCurrent; // Save the current IR command to the last command

    if (TinyIRReceiverData.Flags == IRDATA_FLAGS_IS_REPEAT)
    {
      irRepeat = true;
    }
    if (TinyIRReceiverData.Flags == IRDATA_FLAGS_PARITY_FAILED)
    {
      irStatus = false;
      debugln(F(" Parity failed"));
    }
  }
  if (irCurrent.command)
  {
    // debug("IR command: ");
    // debugln(irCurrent.command);
  }
  /***********************************************************************
   * IR is takem care of, lets do the rest
   ***********************************************************************/

} // End irLoop()

void irMenu(u8_t menuNumber)
{
  // Add code to handle the IR menu based on the menu number passed as a parameter

  switch (menuNumber)
  {
  case 0:
    irMenu00(); // Call the function to handle menu 0
    break;      // Break out of the switch statement
  case 1:
    irMenu01(); // Call the function to handle menu 1
    break;      // Break out of the switch statement
  case 2:
    break; // Break out of the switch statement
  case 3:
    break; // Break out of the switch statement
  case 4:
    break; // Break out of the switch statement
  default:
    break; // Break out of the switch statement
  }

  return;
} // end irMenu()

void irMenu00()
{
  // debugln("IR Menu 0");
  if (!irRepeat)
  {
    // debugln("IR Menu 0, not repeat");
    switch (irCurrent.command)
    {
    case IRU:
      debug(" cBlock.matrixBrightness: ");
      debugln(cBlock.matrixBrightness);
      cBlock.matrixBrightness = cBlock.matrixBrightness + 8; // Increase brightness by 5
      if (cBlock.matrixBrightness > 255)
        cBlock.matrixBrightness = 255; // Set max brightness to 255
      debug(" cBlock.matrixBrightness: ");
      debugln(cBlock.matrixBrightness);
      setupColors(cBlock.matrixBrightness / 255.0); // Set the colors for the matrix display
      break;                                        // Break out of the switch statement
    case IRD:
      cBlock.matrixBrightness = cBlock.matrixBrightness - 8; // Decrease brightness by 5
      if (cBlock.matrixBrightness < 0)
        cBlock.matrixBrightness = 0;                // Set min brightness to 0
      setupColors(cBlock.matrixBrightness / 255.0); // Set the colors for the matrix display
      break;                                        // Break out of the switch statement
    case IRL:
      pControl[cBlock.programMode].programActive = false; // Set the program active flag to false for the old program mode
      cBlock.programMode -= 1;                            // Decrease program mode by 1
      if (cBlock.programMode < 1)
        cBlock.programMode = 9; // Set max program mode to 9
      break;                    // Break out of the switch statement
    case IRR:
      pControl[cBlock.programMode].programActive = false; // Set the program active flag to false for the old program mode
      cBlock.programMode += 1;                            // Increase program mode by 1
      if (cBlock.programMode > 9)
        cBlock.programMode = 1; // Set min program mode to 1
      break;                    // Break out of the switch statement
    case IR0:
      pControl[cBlock.programMode].programActive = false; // Set the program active flag to false for the old program mode
      cBlock.programMode = 0;
      break; // Break out of the switch statement
    case IR1:
      pControl[cBlock.programMode].programActive = false; // Set the program active flag to false for the old program mode
      cBlock.programMode = 1;
      break; // Break out of the switch statement
    case IR2:
      pControl[cBlock.programMode].programActive = false; // Set the program active flag to false for the old program mode
      cBlock.programMode = 2;
      break; // Break out of the switch statement
    case IR3:
      pControl[cBlock.programMode].programActive = false; // Set the program active flag to false for the old program mode
      cBlock.programMode = 3;
      break; // Break out of the switch statement
    case IR4:
      pControl[cBlock.programMode].programActive = false; // Set the program active flag to false for the old program mode
      cBlock.programMode = 4;
      break; // Break out of the switch statement
    case IR5:
      pControl[cBlock.programMode].programActive = false; // Set the program active flag to false for the old program mode
      cBlock.programMode = 5;
      break; // Break out of the switch statement
    case IR6:
      pControl[cBlock.programMode].programActive = false; // Set the program active flag to false for the old program mode
      cBlock.programMode = 6;
      break; // Break out of the switch statement
    case IR7:
      pControl[cBlock.programMode].programActive = false; // Set the program active flag to false for the old program mode
      cBlock.programMode = 7;
      break; // Break out of the switch statement
    case IR8:
      pControl[cBlock.programMode].programActive = false; // Set the program active flag to false for the old program mode
      cBlock.programMode = 8;
      break; // Break out of the switch statement
    case IR9:

      cBlock.programMode = 9;
      break; // Break out of the switch statement
    case IROK:
      break; // Break out of the switch statement
    case IRS:
      break; // Do nothing for IRS command
    case IRP:
      // Do nothing for IRP command
      break; // Break out of the switch statement
    default:
      break; // Break out of the switch statement
    }
  }
  else if (irRepeat && irCurrent.command == IRU) // Check if the menu flag is set and IR repeat is true
  {
    cBlock.matrixBrightness = cBlock.matrixBrightness + 8; // Increase brightness by 5
    if (cBlock.matrixBrightness > 255)
      cBlock.matrixBrightness = 255;              // Set max brightness to 255
    setupColors(cBlock.matrixBrightness / 255.0); // Set the colors for the matrix display
  }
  else if (irRepeat && irCurrent.command == IRD) // Check if the menu flag is not set and IR repeat is true
  {
    cBlock.matrixBrightness = cBlock.matrixBrightness - 8; // Decrease brightness by 5
    if (cBlock.matrixBrightness < 0)
      cBlock.matrixBrightness = 0;                // Set min brightness to 0
    setupColors(cBlock.matrixBrightness / 255.0); // Set the colors for the matrix display
  }
  // break;

  return;
} // end irMenu00()

void irMenu01()
{
  // debugln("IR Menu 1");
  // Add code to handle the IR menu for menu number 1
  if (!irRepeat)
  {
    switch (irCurrent.command)
    {
    case IRU:
      debug(" cBlock.matrixBrightness: ");
      debugln(cBlock.matrixBrightness);
      cBlock.matrixBrightness = cBlock.matrixBrightness + 8; // Increase brightness by 5
      if (cBlock.matrixBrightness > 255)
        cBlock.matrixBrightness = 255; // Set max brightness to 255
      debug(" cBlock.matrixBrightness: ");
      debugln(cBlock.matrixBrightness);
      setupColors(cBlock.matrixBrightness / 255.0); // Set the colors for the matrix display
      break;                                        // Break out of the switch statement
    case IRD:
      cBlock.matrixBrightness = cBlock.matrixBrightness - 8; // Decrease brightness by 5
      if (cBlock.matrixBrightness < 0)
        cBlock.matrixBrightness = 0;                // Set min brightness to 0
      setupColors(cBlock.matrixBrightness / 255.0); // Set the colors for the matrix display
      break;                                        // Break out of the switch statement
    case IRL:
      // pControl[cBlock.programMode].subProgramActive = false; // Set the program active flag to false for the old program mode
      pControl[cBlock.programMode].subProgram -= 1;    // Decrease program mode by 1
      if (pControl[cBlock.programMode].subProgram < 1) // Set min subprogram mode to zero
        pControl[cBlock.programMode].subProgram = 0;
      break; // Break out of the switch statement
    case IRR:
      // pControl[cBlock.programMode].subProgramActive = false; // Set the program active flag to false for the old program mode
      pControl[cBlock.programMode].subProgram += 1; // Increase program mode by 1
      if (pControl[cBlock.programMode].subProgram > pControl[cBlock.programMode].subProgramTotal)
        pControl[cBlock.programMode].subProgram = pControl[cBlock.programMode].subProgramTotal; // Set min program mode to 1
      break;                                                                                    // Break out of the switch statement
    case IR0:
      pControl[cBlock.programMode].programActive = false;    // Set the program active flag to false for the old program mode
      pControl[cBlock.programMode].subProgramActive = false; // Set the program active flag to false for the old program mode
      pControl[cBlock.programMode].subProgram = 0;           // Set the submenu to 0 for the old program mode
      pControl[cBlock.programMode].irMenuLevel = 0;          // Set the IR menu level to 0 for the old program mode
      cBlock.programMode = 0;
      break; // Break out of the switch statement
    case IR1:
      pControl[cBlock.programMode].programActive = false;    // Set the program active flag to false for the old program mode
      pControl[cBlock.programMode].subProgramActive = false; // Set the program active flag to false for the old program mode
      pControl[cBlock.programMode].subProgram = 0;           // Set the submenu to 0 for the old program mode
      pControl[cBlock.programMode].irMenuLevel = 0;          // Set the IR menu level to 0 for the old program mode
      cBlock.programMode = 1;

      break; // Break out of the switch statement
    case IR2:
      pControl[cBlock.programMode].programActive = false;    // Set the program active flag to false for the old program mode
      pControl[cBlock.programMode].subProgramActive = false; // Set the program active flag to false for the old program mode
      pControl[cBlock.programMode].subProgram = 0;           // Set the submenu to 0 for the old program mode
      pControl[cBlock.programMode].irMenuLevel = 0;          // Set the IR menu level to 0 for the old program mode
      cBlock.programMode = 2;

      break; // Break out of the switch statement
    case IR3:
      pControl[cBlock.programMode].programActive = false;    // Set the program active flag to false for the old program mode
      pControl[cBlock.programMode].subProgramActive = false; // Set the program active flag to false for the old program mode
      pControl[cBlock.programMode].subProgram = 0;           // Set the submenu to 0 for the old program mode
      pControl[cBlock.programMode].irMenuLevel = 0;          // Set the IR menu level to 0 for the old program mode
      cBlock.programMode = 3;
      break; // Break out of the switch statement
    case IR4:
      pControl[cBlock.programMode].programActive = false;    // Set the program active flag to false for the old program mode
      pControl[cBlock.programMode].subProgramActive = false; // Set the program active flag to false for the old program mode
      pControl[cBlock.programMode].subProgram = 0;           // Set the submenu to 0 for the old program mode
      pControl[cBlock.programMode].irMenuLevel = 0;          // Set the IR menu level to 0 for the old program mode
      cBlock.programMode = 4;
      break; // Break out of the switch statement
    case IR5:
      pControl[cBlock.programMode].programActive = false;    // Set the program active flag to false for the old program mode
      pControl[cBlock.programMode].subProgramActive = false; // Set the program active flag to false for the old program mode
      pControl[cBlock.programMode].subProgram = 0;           // Set the submenu to 0 for the old program mode
      pControl[cBlock.programMode].irMenuLevel = 0;          // Set the IR menu level to 0 for the old program mode
      cBlock.programMode = 5;
      break; // Break out of the switch statement
    case IR6:
      pControl[cBlock.programMode].programActive = false;    // Set the program active flag to false for the old program mode
      pControl[cBlock.programMode].subProgramActive = false; // Set the program active flag to false for the old program mode
      pControl[cBlock.programMode].subProgram = 0;           // Set the submenu to 0 for the old program mode
      pControl[cBlock.programMode].irMenuLevel = 0;          // Set the IR menu level to 0 for the old program mode
      cBlock.programMode = 6;
      break; // Break out of the switch statement
    case IR7:
      pControl[cBlock.programMode].programActive = false;    // Set the program active flag to false for the old program mode
      pControl[cBlock.programMode].subProgramActive = false; // Set the program active flag to false for the old program mode
      pControl[cBlock.programMode].subProgram = 0;           // Set the submenu to 0 for the old program mode
      pControl[cBlock.programMode].irMenuLevel = 0;          // Set the IR menu level to 0 for the old program mode
      cBlock.programMode = 7;
      break; // Break out of the switch statement
    case IR8:
      pControl[cBlock.programMode].programActive = false;    // Set the program active flag to false for the old program mode
      pControl[cBlock.programMode].subProgramActive = false; // Set the program active flag to false for the old program mode
      pControl[cBlock.programMode].subProgram = 0;           // Set the submenu to 0 for the old program mode
      pControl[cBlock.programMode].irMenuLevel = 0;          // Set the IR menu level to 0 for the old program mode
      cBlock.programMode = 8;
      break; // Break out of the switch statement
    case IR9:
      pControl[cBlock.programMode].programActive = false;    // Set the program active flag to false for the old program mode
      pControl[cBlock.programMode].subProgramActive = false; // Set the program active flag to false for the old program mode
      pControl[cBlock.programMode].subProgram = 0;           // Set the submenu to 0 for the old program mode
      pControl[cBlock.programMode].irMenuLevel = 0;          // Set the IR menu level to 0 for the old program mode
      cBlock.programMode = 9;
      break; // Break out of the switch statement

    case IROK:
      break; // Break out of the switch statement
    case IRS:
      break; // Do nothing for IRS command
    case IRP:
      // Do nothing for IRP command
      break; // Break out of the switch statement
    default:
      break; // Break out of the switch statement
    }
  }
  else if (irRepeat && irCurrent.command == IRU) // Check if the menu flag is set and IR repeat is true
  {
    cBlock.matrixBrightness = cBlock.matrixBrightness + 8; // Increase brightness by 5
    if (cBlock.matrixBrightness > 255)
      cBlock.matrixBrightness = 255;              // Set max brightness to 255
    setupColors(cBlock.matrixBrightness / 255.0); // Set the colors for the matrix display
  }
  else if (irRepeat && irCurrent.command == IRD) // Check if the menu flag is not set and IR repeat is true
  {
    cBlock.matrixBrightness = cBlock.matrixBrightness - 8; // Decrease brightness by 5
    if (cBlock.matrixBrightness < 0)
      cBlock.matrixBrightness = 0;                // Set min brightness to 0
    setupColors(cBlock.matrixBrightness / 255.0); // Set the colors for the matrix display
  }
  // break;

  return;
} // end irMenu01()

bool readBME280()
{
  // Add code to read data from BME280 sensor
  bmeData.temp_C = bme.readTemperature();
  bmeData.temp_F = bmeData.temp_C * 9.0 / 5.0 + 32.0;
  bmeData.pressure_hPa = bme.readPressure() / 100.0F;        // Read the pressure from the BME280 in Pascals then convert to hPa
  bmeData.pressure_inHg = bmeData.pressure_hPa * 0.02953;    // Convert to inches of mercury
  bmeData.pressure_mmHg = bmeData.pressure_hPa * 0.750062;   // Convert to mm of mercury
  bmeData.altitude = bme.readAltitude(SEALEVELPRESSURE_HPA); // Read the altitude from the BME280 in meters
  bmeData.humidity = bme.readHumidity();                     // Read the humidity from the BME280 in percent
  bmeData.lastUpdate = myTZ.now();

  debug("BME280 Temperature: ");
  debug(bmeData.temp_F);
  debug(" F / ");
  debug(bmeData.temp_C);
  debugln(" C");
  debug("BME280 Pressure: ");
  debug(bmeData.pressure_hPa);
  debug(" hPa / ");
  debug(bmeData.pressure_inHg);
  debug(" inHg / ");
  debug(bmeData.pressure_mmHg);
  debugln(" mmHg");
  debug("BME280 Altitude: ");
  debug(bmeData.altitude);
  debugln(" m");
  debug("BME280 Humidity: ");
  debug(bmeData.humidity);
  debugln(" %");

  return true; // Return true if read is successful
} // end readBME280()

bool setupTaskScheduler(void)
{
  // Add code to initialize the task scheduler
  taskS.init();

  return true; // Return true if setup is successful
} // end setupTaskScheduler()

bool readMoonPhase(time_t time)
{
  moonPhaseData.jDate = (time / 86400.0L + 2440587.5);
  debug("time: ");
  debugln(time);
  debug("Julian Date: ");
  debugln(moonPhaseData.jDate);
  // Calculate illumination (synodic) phase.
  // From number of days since new moon on Julian date MOON_SYNODIC_OFFSET
  // (1815UTC January 6, 2000), determine remainder of incomplete cycle.
  moonPhaseData.phase = (moonPhaseData.jDate - MOON_SYNODIC_OFFSET) / MOON_SYNODIC_PERIOD;
  moonPhaseData.phase -= floor(moonPhaseData.phase);

  // Calculate age and illumination fraction.
  moonPhaseData.age = moonPhaseData.phase * MOON_SYNODIC_PERIOD;
  moonPhaseData.fraction = (1.0 - cos(2 * M_PI * moonPhaseData.phase)) * 0.5;
  moonPhaseData.phaseName = phaseNames[(int)(moonPhaseData.phase * 8 + 0.5) % 8];

  // Calculate distance from anomalistic phase.
  double distancePhase = (moonPhaseData.jDate - MOON_DISTANCE_OFFSET) / MOON_DISTANCE_PERIOD;
  distancePhase -= floor(distancePhase);
  moonPhaseData.distance = 60.4 - 3.3 * cos(2 * M_PI * distancePhase) - 0.6 * cos(2 * 2 * M_PI * moonPhaseData.phase - 2 * M_PI * distancePhase) - 0.5 * cos(2 * 2 * M_PI * moonPhaseData.phase);

  // Calculate ecliptic latitude from nodal (draconic) phase.
  double latPhase = (moonPhaseData.jDate - MOON_LATITUDE_OFFSET) / MOON_LATITUDE_PERIOD;
  latPhase -= floor(latPhase);
  moonPhaseData.latitude = 5.1 * sin(2 * M_PI * latPhase);

  // Calculate ecliptic longitude from sidereal motion.
  double longPhase = (moonPhaseData.jDate - MOON_LONGITUDE_OFFSET) / MOON_LONGITUDE_PERIOD;
  longPhase -= floor(longPhase);
  moonPhaseData.longitude = 360 * longPhase + 6.3 * sin(2 * M_PI * distancePhase) + 1.3 * sin(2 * 2 * M_PI * moonPhaseData.phase - 2 * M_PI * distancePhase) + 0.7 * sin(2 * 2 * M_PI * moonPhaseData.phase);
  if (moonPhaseData.longitude > 360)
    moonPhaseData.longitude -= 360;

  // Select the Zodiac name.
  moonPhaseData.zodiacName = zodiacNames[0];
  for (int i = 0; i < sizeof(zodiacAngles) / sizeof(float); i++)
  {
    if (moonPhaseData.longitude < zodiacAngles[i])
    {
      moonPhaseData.zodiacName = zodiacNames[i];
      break;
    }
  }

  return true;
} // End readMoonPhase()

void programClock(bool is24hr)
{
  // Add code to program the clock with the specified 24-hour format setting
}

void programStartupDisplay()
{
  // Add code to program the startup display
}

void programAstroData()
{
  // Add code to program the astronomy data display
}

void programWeather()
{
  // Add code to program the weather display
}

void programTester()
{
  // Add code to program the tester display
}

void programSystemStatus()
{
  // Add code to program the system status display
}