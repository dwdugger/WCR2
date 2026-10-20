
/*************************************************************************************************
 * OpenWeatherOneCall object
 *************************************************************************************************/

char ONECALLKEY[] = "Temp Key";

//*************** LOCATION ************************

int myTimeZone = -6; //<--------- GMT OFFSET

// Location MODES are used to tell the program how you are delivering your coordinates:
//  1 LAT/LON  <----- Can be from GPS, manual as below, or whatever method gets you a latitude and longitude except IP location
//  2 CITYID   <----- From OpenWeatherMap City ID list. Not very accurate.
//  3 IP ADDRESS [NOT REALLY SUGGESTED CAN BE WILDLY OFFSET LOCATION IF HOTSPOT]

int locationMode = 1;

// For Latitude and Longitude Location MODE 1 setting if used
float myLATITUDE = 43.837; //?? just random coordinates for testing
float myLONGITUDE = -103.624;

// For City ID Location MODE setting if used
int myCITYID = 4504476; //<-----Toms River, NJ USA

//*************** END LOCATION **********************

//*************** FLAGS *****************************
int myCURRENT = 1;                     //<-----0 is CURRENT off, 1 is on
int myAIRQUALITY = 1;                  //<-----0 is Air Quality off, 1 is on
int myTIMESTAMP = 1;                   //<-----0 is TIMESTAMP off, 1 is TIMESTAMP on
char timestampDate[11] = "03/08/1974"; //<-----MM/DD/YYYY right here limit is 4 days in future, past is 01/01/1979
int myOVERVIEW = 1;                    //<-----0 is OVERVIEW off, 1 is OVERVIEW on
char overviewDate[9] = "TOMORROW";     //<-----"TODAY" or "TOMORROW"
//***************************************************

//************** EXCLUDES ***************************
// Excludes are EXCL_D, EXCL_H, EXCL_M, EXCL_A
// Those are DAILY, HOURLY, MINUTELY, and ALERTS
// You set them like this: myEXCLUDES = EXCL_D+EXCL_H+EXCL_M+EXCL_A
// If you leave them set to 0 as below you get a JSON file with ALL CURRENT WEATHER measurements which is a huge file

int myEXCLUDES = 0; //<-----0 Excludes is default
//***************************************************

//*************** UNITS OF MEASURMENT and DATE TIME FORMAT ***************

int myUNITS = IMPERIAL; //<-----METRIC, IMPERIAL, KELVIN (IMPERIAL is default)

// Date Time Format
int myDTF = 1; /*
                 1 M/D/Y 24H
                 2 D/M/Y 24H
                 3 M/D/Y 12H
                 4 D/M/Y 12H
                 5/6 TIME ONLY 24H
                 7/8 TIME ONLY 12H
                 9 DAY SHORTNAME
                 10 M/D/Y ONLY,
                 11 D/M/Y ONLY

                 ISO8601 options:
                 12 YYYY-MM-DD ONLY,
                 13 THH:MM:SS ONLY
                 14 YYYY-MM-DDTHH:MM:SSY 24H
                 */
//************************************************************************

// for debugging loop counting
int nextCall = 0;
// int callAttempt = 1;


/***************************************************************************************
 * The OpenWeatherOneCall object ends here
 ****************************************************************************************/