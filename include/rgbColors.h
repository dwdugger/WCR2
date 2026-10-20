/*
 * These are the RGB rererence representation of colors used
 * in the clock.
 * RGB values will be mulltiplied by a brightness factor
 * then will transformed into working color565 tables
 */

// These are 16 bit "RGB565" colors in 3x8 bit RGB format
// #define BLUE 0x001F
uint8_t rgbBLUE[3]{0, 0, 0xff};

// #define RED 0xF800
uint8_t rgbRED[3]{0xff, 0, 0};

// #define GREEN 0x07E0
uint8_t rgbGREEN[3]{0, 0xff, 0};

// #define CYAN 0x07FF
uint8_t rgbCYAN[3]{0, 0xff, 0xff};

// #define MAGENTA 0xF81F
uint8_t rgbMAGENTA[3]{0xff, 0, 0xff};

// #define YELLOW 0xFFE0
uint8_t rgbYELLOW[3]{0xff, 0xff, 0};

// #define WHITE 0xFFFF
uint8_t rgbWHITE[3]{0xff, 0xff, 0xff};

// #define BLACK 0x0000 // Black
uint8_t rgbBLACK[3]{0x00, 0x00, 0x00}; // Black

// #define NIGHT 0x2805 // Dark Blue
uint8_t rgbNIGHT1[3]{0xc5, 0x20, 0xff}; // Might Sky

// #define NIGHT2 0x6878 // Darker Blue
uint8_t rgbNIGHT2[3]{0x41, 0x09, 0x75}; // Darker Blue

// #define DAY1 0x120c // Light Blue
uint8_t rgbDAY1[3]{0xbd, 0x10, 0xff}; // Day 1

// #define DAY2 0x01ec // Light Cyan
uint8_t rgbDAY2[3]{0x00, 0xec, 0x01}; // Day 2

// #define MYYELLOW1 0xff20 // Yellow
uint8_t rgbMYYELLOW1[3]{0xff, 0xce, 0x42}; // My Yellow 1

// #define MYYELLOW2 0xfeac // Light Yellow
uint8_t rgbMYYELLOW2[3]{0xff, 0xce, 0x00}; // My Yellow 2

// Reference array of RGB format colors that will be used for sand
// These will be converted to a table modified for brightness
uint8_t colorRGB[8][3]{
    64, 64, 64,  // Dark Gray
    120, 79, 23, // Brown
    228, 3, 3,   // Red
    255, 140, 0, // Orange
    255, 237, 0, // Yellow
    0, 128, 38,  // Green
    0, 77, 255,  // Blue
    117, 7, 135  // Purple
};

// These are the *working* color tables and can be modified
// at runtime to control the display brightness
u16_t BLUE;
u16_t RED;
u16_t GREEN;
u16_t CYAN;
u16_t MAGENTA;
u16_t YELLOW;
u16_t WHITE;
u16_t colors[8];
u16_t BLACK = 0x0000; // Black is always black
u16_t NIGHT1;
u16_t NIGHT2;
u16_t DAY1;
u16_t DAY2;
u16_t MYYELLOW1;
u16_t MYYELLOW2;
