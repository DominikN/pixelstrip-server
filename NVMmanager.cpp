#include <Preferences.h>
#include "NVMmanager.h"

#define NS_SETTINGS   "settings"
#define NS_THEMES     "themes"

Preferences preferences;
SemaphoreHandle_t semNVM = NULL;

int nvmInit()
{
  semNVM = xSemaphoreCreateMutex();
  xSemaphoreGive(semNVM);
}

int saveSettings(Settings_t* s)
{
  if (xSemaphoreTake(semNVM, ( TickType_t)1000)) {
    preferences.begin(NS_SETTINGS, false);
    preferences.putString("strip_mode", s->mode.c_str());
    preferences.putString("strip_theme", s->current_theme.c_str());
    preferences.putInt("strip_delay", s->delay);
    preferences.end();
    xSemaphoreGive(semNVM);
  } else {
    Serial.printf("NVM error WS\r\n");
  }

  return 0;
}

int loadSettings(Settings_t* s)
{
  bool fullreset = false;
  if (xSemaphoreTake(semNVM, ( TickType_t)1000)) {
    preferences.begin(NS_SETTINGS, false);
    s->mode = preferences.getString("strip_mode");

    if (s->mode == "") {
      preferences.putString("strip_mode", "auto");
      preferences.putString("strip_theme", "none");
      preferences.putInt("strip_theme_num", -1);
      preferences.putInt("strip_delay", -1);
  
      fullreset = true;
    }

    s->current_theme = preferences.getString("strip_theme");
    s->themeNum = preferences.getInt("strip_theme_num");
    s->delay = preferences.getInt("strip_delay");
    
    preferences.end();

    if (fullreset == true) {
      resetThemes();
    }
    xSemaphoreGive(semNVM);
  } else {
    Serial.printf("NVM error WS\r\n");
  }

  return 0;
}
int saveThemesGlobal(ThemesGlobal_t* t)
{
  if (xSemaphoreTake(semNVM, ( TickType_t)1000)) {
    preferences.begin(NS_THEMES, false);

    for (int i = 0; i < (t->available); i++) {
      preferences.putString(String("theme[" + String(i) + "]n").c_str(), t->themeName[i].c_str());
      preferences.putInt(String("theme[" + String(i) + "]x").c_str(), t->pixelsNo[i]);
      preferences.putInt(String("theme[" + String(i) + "]y").c_str(), t->framesNo[i]);
    }
    preferences.putInt("available", t->available);

    preferences.end();
    xSemaphoreGive(semNVM);
  } else {
    Serial.printf("NVM error WS\r\n");
  }

  return 0;
}

int loadThemesGlobal(ThemesGlobal_t* t)
{
  if (xSemaphoreTake(semNVM, ( TickType_t)1000)) {
    preferences.begin(NS_THEMES, false);

    t->available = preferences.getInt("available");

    for (int i = 0; i < (t->available); i++) {
      t->themeName[i] = preferences.getString(String("theme[" + String(i) + "]n").c_str());
      t->pixelsNo[i] = preferences.getInt(String("theme[" + String(i) + "]x").c_str());
      t->framesNo[i] = preferences.getInt(String("theme[" + String(i) + "]y").c_str());
    }

    preferences.end();
    xSemaphoreGive(semNVM);
  } else {
    Serial.printf("NVM error WS\r\n");
  }

  return 0;
}

int saveThemeFrame(LedStripState* ls, int themeNo, int frameNo)
{
  if (xSemaphoreTake(semNVM, ( TickType_t)1000)) {
    preferences.begin(NS_THEMES, false);
    
    String key = String("theme[" + String(themeNo) + "][" + String(frameNo) + "]");

    preferences.putBytes(key.c_str(), (void*)ls, sizeof(LedStripState));
    preferences.end();

    xSemaphoreGive(semNVM);

    Serial.printf("write: %s\r\n", key.c_str());
  } else {
    Serial.printf("NVM error WS\r\n");
  }
}

int loadThemeFrame(LedStripState* ls, int themeNo, int frameNo)
{
  if (xSemaphoreTake(semNVM, ( TickType_t)1000)) {
    preferences.begin(NS_THEMES, false);
    
    String key = String("theme[" + String(themeNo) + "][" + String(frameNo) + "]");

    preferences.getBytes(key.c_str(), (void*)ls, sizeof(LedStripState));
    preferences.end();

    xSemaphoreGive(semNVM);

    Serial.printf("read: %s\r\n", key.c_str());
  } else {
    Serial.printf("NVM error WS\r\n");
  }
}


int resetThemes()
{
  if (xSemaphoreTake(semNVM, ( TickType_t)1000)) {
    preferences.begin(NS_THEMES, false);
    preferences.clear();
    preferences.putInt("available", 0);
    preferences.end();
    xSemaphoreGive(semNVM);
  } else {
    Serial.printf("NVM error WS\r\n");
  }

  return 0;
}

int getThemeNo(String theme)
{
  String theme1;
  int num = 0;
  int i = 0;

  Serial.printf("Finding \"%s\" theme number ... ", theme.c_str());

  if (xSemaphoreTake(semNVM, ( TickType_t)1000)) {
    preferences.begin(NS_THEMES, true);

    for (i = 0 ; i < MAXTHEMENO; i++) {
      String key = String("theme[" + String(i) + "]n");
      theme1 = preferences.getString(key.c_str());
      if (theme.equals(theme1)) {
        num = i;
        break;
      }
    }
    if (i == MAXTHEMENO) {
      num = -1;
    }
    preferences.end();
    xSemaphoreGive(semNVM);
  } else {
    Serial.printf("NVM error WS\r\n");
  }
  return num;
}