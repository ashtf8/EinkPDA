#include "globals.h"
#include "sdmmc_cmd.h"

//   .oooooo..o oooooooooooo ooooooooooooo ooooo     ooo ooooooooo.    //
//  d8P'    `Y8 `888'     `8 8'   888   `8 `888'     `8' `888   `Y88.  //
//  Y88bo.       888              888       888       8   888   .d88'  //
//   `"Y8888o.   888oooo8         888       888       8   888ooo88P'   //
//       `"Y88b  888    "         888       888       8   888          //
//  oo     .d8P  888       o      888       `88.    .8'   888          //
//  8""88888P'  o888ooooood8     o888o        `YbodP'    o888o         //

// Display setup
GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT> display(GxEPD2_310_GDEQ031T10(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));
volatile bool GxEPD2_310_GDEQ031T10::useFastFullUpdate = true;
U8G2_SSD1326_ER_256X32_F_4W_HW_SPI u8g2(U8G2_R2, OLED_CS, OLED_DC, OLED_RST); //256x32

// Keypad setup
Adafruit_TCA8418 keypad;

// Buzzer
Buzzer buzzer(17);

char keysArray[4][10] = {
  { 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p' },
  { 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',   8 },  //8:BKSP
  {   9, 'z', 'x', 'c', 'v', 'b', 'n', 'm', '.',  13 },  //9:TAB, 13:CR
  {   0,  17,  18, ' ', ' ', ' ',  19,  20,  21,   0 }   //17:SHFT, 18:FN, 19:<-, 20:SEL, 21:-> 
};

char keysArraySHFT[4][10] = {
  { 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P' },
  { 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',   8 },
  {   9, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',',  13 },
  {   0,  17,  18, ' ', ' ', ' ',  19,  20,  21,   0 }
};

char keysArrayFN[4][10] = {
  { '1', '2', '3', '4', '5', '6', '7',  '8',  '9', '0' },
  { '#', '!', '$', ':', ';', '(', ')', '\'', '\"',   8 },
  {  14, '%', '_', '&', '+', '-', '/',  '?',  ',',  13 },
  {   0,  17,  18, ' ', ' ', ' ',  12,    7,    6,   0 }
};

// Touch slider setup
Adafruit_MPR121 cap = Adafruit_MPR121();

// RTC setup
RTC_PCF8563 rtc;
const char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

// USB
USBMSC msc;
bool mscEnabled = false;
sdmmc_card_t* card = nullptr;

// VARIABLES
// GENERAL
Preferences prefs;
int TIMEOUT;              // Time until automatic sleep (Seconds)
bool DEBUG_VERBOSE;       // Spit out some extra information
bool SYSTEM_CLOCK;        // Enable a small clock on the bottom of the screen.
bool SHOW_YEAR;           // Show the year on the clock
bool SAVE_POWER;          // Enable a slower CPU clock speed to save battery with little cost to performance
bool ALLOW_NO_MICROSD;    // Allow the device to operate with no SD card
bool HOME_ON_BOOT;        // Always start the home app on boot
int OLED_BRIGHTNESS;      // Brightness of the OLED (0-255)
int OLED_MAX_FPS;         // Define the max oled FPS

volatile int einkRefresh = FULL_REFRESH_AFTER;
int OLEDFPSMillis = 0;
int KBBounceMillis = 0;
volatile int timeoutMillis = 0;
volatile int prevTimeMillis = 0;
volatile bool TCA8418_event = false;
volatile bool PWR_BTN_event = false;
volatile bool SHFT = false;
volatile bool FN = false;
volatile bool newState = false;
bool noTimeout = false;
volatile bool OLEDPowerSave = false;
volatile bool disableTimeout = false;
volatile int battState = 0;
volatile int prevBattState = 0;
unsigned int flashMillis = 0;
int prevTime = 0;
uint8_t prevSec = 0;
TaskHandle_t einkHandlerTaskHandle = NULL;
char currentKB[4][10];
KBState CurrentKBState = NORMAL;
uint8_t partialCounter = 0;
volatile bool forceSlowFullUpdate = false;
volatile bool SDCARD_INSERT = false;
bool noSD = false;
volatile bool SDActive = false;

const String appStateNames[] = { "txt", "filewiz", "usb", "bt", "settings", "tasks", "calendar", "journal", "lexicon", "calc" };
const unsigned char *appIcons[10] = { _homeIcons2, _homeIcons3, _homeIcons4, _homeIcons5, _homeIcons6, taskIconTasks0, _homeIcons7, _homeIcons8, _homeIcons9,_homeIcons10};

AppState CurrentAppState;

// <TXT.cpp>
String currentWord = "";
String allText = "";
String prevAllText = "";
String prevLastLine = "";
bool prevBKSP = false;
int scroll = 0;
int lines = 0;
String outLines[13];
String lines_prev[13];
String filesList[MAX_FILES];
uint8_t fileIndex = 0;
String editingFile;
String prevEditingFile = "";
String excludedFiles[3] = { "/temp.txt", "/settings.txt", "/tasks.txt" };
TXTState CurrentTXTState = TXT_;

String currentLine = "";
const GFXfont *currentFont = (GFXfont *)&FreeSerif9pt7b;
uint8_t maxCharsPerLine = 0;
uint8_t maxLines = 0;
uint8_t fontHeight = 0;
uint8_t lineSpacing = 6;  // LINE SPACING IN PIXELS
volatile bool newLineAdded = true;
volatile bool doFull = false;
std::vector<String> allLines;
volatile long int dynamicScroll = 0;
volatile long int prev_dynamicScroll = 0;
int lastTouch = -1;
unsigned long lastTouchTime = 0;

// <TASKS.cpp>
std::vector<std::vector<String>> tasks;
uint8_t selectedTask = 0;
TasksState CurrentTasksState = TASKS0;
uint8_t newTaskState = 0;
uint8_t editTaskState = 0;
String newTaskName = "";
String newTaskDueDate = "";

// <HOME.cpp>
HOMEState CurrentHOMEState = HOME_HOME;

// <FILEWIZ.cpp> 
FileWizState CurrentFileWizState = WIZ0_;
String workingFile = "";

// <SETTINGS.cpp>
SettingsState CurrentSettingsState = settings0;

// <CALENDAR.cpp>
CalendarState CurrentCalendarState = MONTH;

// <LEXICON.cpp>
LexState CurrentLexState = MENU;


// <CALC.cpp>
CALCState CurrentCALCState = CALC0;
int calcSwitchedStates = 0;
int trigType = 1;
int refresh_count = 0;
std::vector<String> allLinesCalc;
String cleanExpression = "";
String calculatedResult = "";
String prevLine = "";
char bufferString[20];
std::map<String, float> variables= {};
std::set<String> constantsCalc = {
       "inf", "-inf", "pi", "e", "ans"
};
std::set<String> operatorsCalc = {
      "+", "-", "'", "/", "%", "=", "!", "\""
};
std::set<String> functionsCalc = {
      // trig
      "sin", "cos", "tan", "asin", "acos", "atan",
      "sinh", "cosh", "tanh", "sec", "csc", "cot", 
      "sech", "csch", "coth", "asec", "acsc", "acot",
      // scientific
      "ln", "log", "sqrt", "cbrt", "abs", "exp",
      "round", "min", "max", "pow", "rand",
      // fun
      "dice","pick"
};
std::map<String, int> precedenceCalc = {
      {":", 0}, {"+", 1}, {"-", 1}, {"'", 2}, {"/", 2}, {"%", 2}, {"\"", 3}, {"!", 4}, {"~neg~",4}
  };
std::vector<String>  helpText = {
  "\n",
  "    vvv scroll down vvv\n",
  "This is the help screen\n",
  "\n",
  "press enter to exit help\n",
  "\n",
  "NOTES:\n",
  "  /6 -> EXIT APP\n",
  "  scroll <- ->\n",
  "\n",
  "    vvv scroll down vvv\n",
  "\n",
  "\n",
  "commands:\n",
  "\n",
  "    '/' + <command> \n",
  " 0 : standard\n",
  "  \n",
  " 1 : programming\n",
  "    (not implemented) \n",
  " 2 : scientific \n",
  "    (not implemented) \n",
  " 3 : conversions \n",
  "    (not implemented) \n",
  " 4 : help\n",
  "  \n",
  " 5 : export to txt\n",
  "  \n",
  " 6 : EXIT\n",
  "  \n",
  " rad : radian mode\n",
  "  \n",
  " deg : degree mode\n",
  "  \n",
  " grad : gradian mode\n",
  "  \n",
  "keyboard changes:\n",
  "  default kb state:FUNC\n",
  "  tab && fn(tab) == font\n",
  "  bksp == fn(bskp)\n",
  "  left arrow scroll  ^ \n",
  "  right arrow scroll v \n",
  "operators:\n",
  "  \n",
  " - (unary included)\n",
  " +\n",
  " * (type: ' or a(b))\n",
  " /\n",
  " %\n",
  " !\n",
  " !! repeat prev line\n",
  " ^ (type: \")\n",
  " = (type: :)\n",
  "\n",
  "functions: \n",
  "\n",
  " sin(a) asin(a) sinh(a)\n",
  " csc(a) acsc(a) csch(a)\n",
  " cos(a) acos(a) cosh(a)\n",
  " sec(a) asec(a) sech(a)\n",
  " tan(a) atan(a) tanh(a)\n",
  " cot(a) acot(a) coth(a)\n",
  " sqrt(a)\n",
  " exp(a)     log(a)\n",
  " pow(a,b)   log10(a\n",
  " floor(a)   ceil(a)\n",
  " min(a)     max(a)\n",
  " round(a)\n",
  " abs(a)\n",
  " rand(a,b) from min a -> b\n",
  " dice(a,b) a: num b:sides\n",
  " pick(a,n) a: choices\n",
  "           n: no. choices\n",
  "\n",
  "variables: \n",
  "\n",
  "must start with alpha char \n",
  "excluding constants \n",
  "    ^^^ scroll up ^^^"
};

// <JOURNAL.cpp>
JournalState CurrentJournalState = J_MENU;

