// EInk PDA V2.0
// @Ashtf 2025

//   .oooooo..o oooooooooooo ooooooooooooo ooooo     ooo ooooooooo.    //
//  d8P'    `Y8 `888'     `8 8'   888   `8 `888'     `8' `888   `Y88.  //
//  Y88bo.       888              888       888       8   888   .d88'  //
//   `"Y8888o.   888oooo8         888       888       8   888ooo88P'   //
//       `"Y88b  888    "         888       888       8   888          //
//  oo     .d8P  888       o      888       `88.    .8'   888          //
//  8""88888P'  o888ooooood8     o888o        `YbodP'    o888o         //  

// LIBRARIES
#include  <Arduino.h>
#include  <GxEPD2_BW.h>
#include  <U8g2lib.h>
#include  <Wire.h>
#include  <Adafruit_TCA8418.h>
#include  <vector>
#include  "RTClib.h"
#include  "freertos/FreeRTOS.h"
#include  "freertos/task.h"
#include  "assets.h"
#include  "FS.h"
#include  "SPIFFS.h"

// CONFIGURATION & SETTINGS
////////////////////////////////////////////////////////////////////////////////////////////////|
#define   KB_COOLDOWN                  50  // Keypress cooldown
#define   FULL_REFRESH_AFTER            5  // Full refresh after N lines (CHANGE WITH CAUTION)
#define   MAX_FILES                    10  // Number of files to store
#define   TIMEOUT                     300  // Time until automatic sleep (Seconds)
#define   FORMAT_SPIFFS_IF_FAILED    true  // Format the SPIFFS filesystem if mount fails
#define   DEBUG_VERBOSE              true  // Spit out some extra information
const     String SLEEPMODE =       "TEXT"; // TEXT, SPLASH, CLOCK
#define   TXT_APP_STYLE                 1  // 0: Old Style (NOT SUPPORTED), 1: New Style
#define   SET_CLOCK_ON_UPLOAD        true  // Should system clock be set automatically on code upload?
////////////////////////////////////////////////////////////////////////////////////////////////|

// FONTS
#include  <Fonts/FreeMonoBold9pt7b.h>

// PIN DEFINITION
#define   I2C_SCL         35
#define   I2C_SDA         36
#define   MPR121_ADDR   0x5A

#define   KB_IRQ           8
#define   PWR_BTN         38
#define   BAT_SENS         6
#define   CHRG_SENS       39
#define   RTC_INT          1

// DISPLAY AND KEYBOARD SETUP
GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT> display(GxEPD2_310_GDEQ031T10(/*CS=*/ 2, /*DC=*/ 21, /*RST=*/ 16, /*BUSY=*/ 37)); 
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

Adafruit_TCA8418 keypad;

char keysArray[4][10] = {
  {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p'},
  {'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',  8 },       //8:BKSP
  { 9 , 'z', 'x', 'c', 'v', 'b', 'n', 'm', '.', 13 },       //9:TAB, 13:CR
  { 0 , 17 , 18 , ' ', ' ', ' ', 19 , 20 , 21 ,  0 }        //17:SHFT, 18:FN, 19:<-, 20:SEL, 21:->
};

char keysArraySHFT[4][10] = {
  {'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'},
  {'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',  8 },       //8:BKSP
  { 9 , 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', 13 },       //9:TAB, 13:CR
  { 0 , 17 , 18 , ' ', ' ', ' ', 19 , 20 , 21 ,  0 }        //17:SHFT, 18:FN, 19:<-, 20:SEL, 21:->
};

char keysArrayFN[4][10] = {
  {'1', '2', '3', '4', '5', '6', '7' , '8' , '9', '0'},
  {'#', '!', '$', ':', ';', '(', ')', '\'', '\"', 12 },     //8:BKSP
  { 9 , '%', '-', '&', '+', '-', '/', '?' , ',' , 13 },     //9:TAB, 13:CR
  { 0 , 17 , 18 , ' ', ' ', ' ',  5 ,  7  ,  6  ,  0 }      //17:SHFT, 18:FN, 5:LOAD, 20:SEL, 6:SAVE, 7:FILE
};

// TOUCH SLIDER SETUP
bool touchStates[12];
const char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// RTC SETUP
RTC_PCF8563 rtc;

// VARIABLES
// GENERAL
volatile int    einkRefresh     = FULL_REFRESH_AFTER;
int             OLEDFPSMillis   = 0;
int             KBBounceMillis  = 0;
volatile int    timeoutMillis   = 0;
volatile int    prevTimeMillis  = 0;
volatile bool   TCA8418_event   = false;
volatile bool   PWR_BTN_event   = false;
volatile bool   SHFT            = false;
volatile bool   FN              = false;
volatile bool   newState        = true;
bool            noTimeout       = false;
volatile bool   OLEDPowerSave   = false;
volatile bool   disableTimeout  = false;
unsigned int    flashMillis     = 0;
int             prevTime        = 0;
uint8_t         prevSec         = 0;
TaskHandle_t einkHandlerTaskHandle = NULL;
char currentKB[4][10];
enum KBState {NORMAL, SHIFT, FUNC};
KBState CurrentKBState = NORMAL;

// <TXT.ino>
String          currentWord     = "";
String          allText         = "";
String          prevAllText     = "";
String          prevLastLine    = "";
bool            prevBKSP        = false;
int             scroll          = 0;
int             lines           = 0;
String          outLines[13];
String          lines_prev[13];
String          filesList[MAX_FILES];
uint8_t         fileIndex       = 0;
String          editingFile     = "";
String          prevEditingFile = "";
String          excludedFiles[] = {"/temp.txt", "/settings.txt"};

String          currentLine     = "";
GFXfont*        currentFont     = (GFXfont*)&FreeMonoBold9pt7b;
uint8_t         maxCharsPerLine = 0;
uint8_t         maxLines        = 0;
uint8_t         fontHeight      = 0;
uint8_t         lineSpacing     = 6;  // LINE SPACING IN PIXELS
volatile bool   newLineAdded    = true;
volatile bool   doFull          = false;
std::vector<String> allLines;

//        .o.       ooooooooo.   ooooooooo.    .oooooo..o  //
//       .888.      `888   `Y88. `888   `Y88. d8P'    `Y8  //  
//      .8"888.      888   .d88'  888   .d88' Y88bo.       //
//     .8' `888.     888ooo88P'   888ooo88P'   `"Y8888o.   //
//    .88ooo8888.    888          888              `"Y88b  //
//   .8'     `888.   888          888         oo     .d8P  //
//  o88o     o8888o o888o        o888o        8""88888P'   //

// ADD APPLICATION, NAME, AND ICON TO LISTS
enum AppState                     {HOME   , TXT   , FILEWIZ   , USB   , BT  , SETTINGS  , DEBUG   };
const String appStateNames[] =    {"HOME" , "TXT" , "FILEWIZ" , "USB" , "BT", "SETTINGS", "DEBUG" };
const unsigned char* appIcons[] = {_homeIcons2,_homeIcons3,_homeIcons4,_homeIcons5,_homeIcons6};

// SET BOOT APP (HOME)
AppState CurrentAppState = HOME;
enum HOMEState {HOME_HOME, NOWLATER};
HOMEState CurrentHOMEState = HOME_HOME;

// ADD E-INK HANDLER APP SCRIPTS HERE
void applicationEinkHandler() {
  switch (CurrentAppState) {
    case HOME:
      einkHandler_HOME();
      break;
    case TXT:
      switch (TXT_APP_STYLE) {
        case 0:
          einkHandler_TXT();
          break;
        case 1:
          einkHandler_TXT_NEW();
          break;
      }
      break;
    case FILEWIZ:
      einkHandler_FILEWIZ();
      break;
    case DEBUG:
      break;
    // ADD APP CASES HERE
    default:
      einkHandler_HOME();
      break;

  }
}

// ADD PROCESS/KEYBOARD APP SCRIPTS HERE
void processKB() {
  switch (CurrentAppState) {
    case HOME:
      processKB_HOME();
      break;
    case TXT:
      switch (TXT_APP_STYLE) {
        case 0:
          processKB_TXT();
          break;
        case 1:
          processKB_TXT_NEW();
          break;
      }
      break;
    case FILEWIZ:
      processKB_FILEWIZ();
      break;
    case DEBUG:
      break;
    // ADD APP CASES HERE
    default:
      einkHandler_HOME();
      break;
  }  
}

//  ooo        ooooo       .o.       ooooo ooooo      ooo  //
//  `88.       .888'      .888.      `888' `888b.     `8'  //
//   888b     d'888      .8"888.      888   8 `88b.    8   //
//   8 Y88. .P  888     .8' `888.     888   8   `88b.  8   //
//   8  `888'   888    .88ooo8888.    888   8     `88b.8   //
//   8    Y     888   .8'     `888.   888   8       `888   //
//  o8o        o888o o88o     o8888o o888o o8o        `8   //

// FUNCTION PROTOTYPES
// <sysFunc.ino>
// SYSTEM
void    checkTimeout();
void    TCA8418_irq();
char    updateKeypress();
void    printDebug();
String  vectorToString();
void    stringToVector();
void    saveFile();
void    loadFile();

// SPIFFS
void    listDir(fs::FS &fs, const char *dirname);
void    readFile(fs::FS &fs, const char *path);
String  readFileToString(fs::FS &fs, const char *path);
void    writeFile(fs::FS &fs, const char *path, const char *message);
void    appendFile(fs::FS &fs, const char *path, const char *message);
void    renameFile(fs::FS &fs, const char *path1, const char *path2);
void    deleteFile(fs::FS &fs, const char *path);

// <OLEDFunc.ino>
void    oledWord(String word);
void    oledLine(String line, bool doProgressBar = true);

// <einkFunc.ino>
void    refresh();
void    einkHandler(void *parameter);
void    statusBar(String input, bool fullWindow = false);
void    einkTextPartial(String text, bool noRefresh = false);
void    drawThickLine(int x0, int y0, int x1, int y1, int thickness);
int     countLines(String input, size_t maxLineLength = 29);
void    einkTextDynamic(bool doFull_, bool noRefresh = false);
void    setTXTFont(const GFXfont* font);

// SETUP
void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

  // EINK HANDLER SETUP
  xTaskCreatePinnedToCore(
    einkHandler,            // Function name (your user-defined function)
    "einkHandlerTask",      // Task name
    10000,                  // Stack size (in bytes)
    NULL,                   // Parameters (none in this case)
    1,                      // Priority (1 is low priority)
    &einkHandlerTaskHandle, // Task handle
    1                       // Core ID (0 for core 0, 1 for core 1)
  );
  display.init(115200);
  display.setRotation(3);
  display.setTextColor(GxEPD_BLACK);

  // MPR121 / SLIDER
  // mpr121_setup();

  // POWER SETUP
  pinMode(PWR_BTN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PWR_BTN), PWR_BTN_irq, FALLING);
  pinMode(CHRG_SENS, INPUT);

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_8, 0);

  // OLED SETUP
  u8g2.begin();
  u8g2.setPowerSave(0);
  u8g2.clearBuffer();	
  u8g2.sendBuffer();

  // KEYBOARD SETUP
  if (! keypad.begin(TCA8418_DEFAULT_ADDR, &Wire)) {
    Serial.println("Error Initializing the Keyboard");
    while (1);
  }
  keypad.matrix(4, 10);
  pinMode(KB_IRQ, INPUT);
  attachInterrupt(digitalPinToInterrupt(KB_IRQ), TCA8418_irq, CHANGE);
  keypad.flush();
  keypad.enableInterrupts();

  // SPIFFS SETUP
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("SPIFFS Mount Failed");
    oledWord("SPIFFS Failed");
    delay(1000);
  }

  // RTC SETUP
  pinMode(RTC_INT, INPUT);
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    oledWord("RTC Failed");
    delay(1000);
  }
  // SET CLOCK IF NEEDED
  if (SET_CLOCK_ON_UPLOAD || rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  rtc.start();
}

void loop() {
  if (!noTimeout)  checkTimeout();
  if (DEBUG_VERBOSE) printDebug();

  processKB();
}