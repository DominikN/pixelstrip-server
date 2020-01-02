#define MAXTHEMENO    10
#define MAXNUMPIXELS  400   // How many NeoPixels are attached to the Arduino?
#define MAXBUFSIZE    2   // buffer size for storing LED strip state (each NUMPIXELS * 3 size)


typedef struct {
  uint8_t pixel[3 * MAXNUMPIXELS];
} LedStripState;

typedef struct {
  String mode;
  String current_theme;
  int themeNum;
  int delay;
} Settings_t;

typedef struct {
  int available;
  String themeName[MAXTHEMENO];
  int pixelsNo[MAXTHEMENO];
  int framesNo[MAXTHEMENO];
} ThemesGlobal_t;

int nvmInit();

int saveSettings(Settings_t* s);
int loadSettings(Settings_t* s);

int saveThemesGlobal(ThemesGlobal_t* t);
int loadThemesGlobal(ThemesGlobal_t* t);

int saveThemeFrame(LedStripState* ls, int themeNo, int frameNo);
int loadThemeFrame(LedStripState* ls, int themeNo, int frameNo);

int resetThemes();
int getThemeNo(String theme);
