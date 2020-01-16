#define MAXTHEMENO    10
#define MAXNUMPIXELS  400   // How many NeoPixels are attached to the Arduino?
#define MAXBUFSIZE    1   // buffer size for storing LED strip state (each NUMPIXELS * 3 size)


typedef struct {
  uint8_t pixel[3 * MAXNUMPIXELS];
} LedStripState;

typedef struct {
  String mode;
  String current_theme;
  int themeNum;
  int delay;
  int timeStartH;
  int timeStartM;
  int timeStopH;
  int timeStopM;
} Settings_t;

typedef struct {
  int available;
  String themeName[MAXTHEMENO];
  int pixelsNo[MAXTHEMENO];
  int framesNo[MAXTHEMENO];
} ThemesGlobal_t;

int FSinit();

int saveSettingsJson(String& settingsJson);
int loadSettingsJson(String& settingsJson);
int jsonToSettings(String& settingsJsonIn, Settings_t& stgsOut);
int settingsToJson(Settings_t& stgsIn, String& settingsJsonOut );

int saveThemesDescJson(String& themesDescJson);
int loadThemesDescJson(String& themesDescJson);
int jsonToThemesDesc(String& themesJsonIn, ThemesGlobal_t& thmsStgs);
int themesDescToJson(ThemesGlobal_t& thmsStgs, String& themesJsonOut );

int saveThemeFrame(LedStripState* ls, int themeNo, int frameNo, int frameSize);
int loadThemeFrame(LedStripState* ls, int themeNo, int frameNo, int frameSize);

int resetThemes();
