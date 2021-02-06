#include <ArduinoJson.h>
#include <WiFi.h>

#include "FS.h"
#include "SPIFFS.h"

#include "FSmanager.h"

SemaphoreHandle_t semFS = NULL;

char* settingsFile = "/generalStgs.json";
char* themesDescFile = "/themeStgs2.json";

void listDir(fs::FS& fs, const char* dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.printf("- %d failed to open directory", __LINE__);
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

int FSinit() {
  semFS = xSemaphoreCreateMutex();
  xSemaphoreGive(semFS);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return -1;
  }

  // SPIFFS.remove(settingsFile);
  // SPIFFS.remove(themesDescFile);
  // SPIFFS.mkdir("/themes");
  listDir(SPIFFS, "/", 0);

  return 1;
}

int saveSettingsJson(String& settingsJson) {
  int size;
  if (xSemaphoreTake(semFS, (TickType_t)1000)) {
    Serial.printf("Opening file: %s\r\n", settingsFile);
    File file = SPIFFS.open(settingsFile, FILE_WRITE);
    if (!file) {
      Serial.printf("- %d: failed to open file for writing\r\n", __LINE__);
      size = -1;
    } else {
      // file.flush();
      size = file.print(settingsJson);
      file.close();

      Serial.printf("Save settings: [%s][%d Bytes]\r\n", settingsJson.c_str(),
                    size);
    }
    xSemaphoreGive(semFS);
  } else {
    Serial.printf("Sem error WS\r\n");
    size = -1;
  }

  return size;
}

int loadSettingsJson(String& settingsJson) {
  uint8_t buf[512];
  int size;

  if (xSemaphoreTake(semFS, (TickType_t)1000)) {
    Serial.printf("Opening file: %s\r\n", settingsFile);
    File file = SPIFFS.open(settingsFile, FILE_READ);
    if (!file || file.isDirectory()) {
      Serial.printf("- %d: failed to open file for reading\r\n", __LINE__);
      size = -1;
    } else {
      size = file.read(buf, file.available());
      file.close();

      if (size < 0) {
        settingsJson = String(
            "{\"config\":{\"mode\":\"auto\", \"currentTheme\":\"none\", "
            "\"themeNum\":0, \"delay\":50, \"startH\":0, \"startM\":0, "
            "\"stopH\":23, \"stopM\":59}}");
      } else {
        buf[size] = 0;  // make a C-string
        settingsJson = String((char*)buf);
      }
      Serial.printf("Load settings: %d bytes [%s]\r\n", size,
                    settingsJson.c_str());
    }

    xSemaphoreGive(semFS);
  } else {
    Serial.printf("Sem error WS\r\n");
    size = -1;
  }

  return size;
}

int jsonToSettings(String& settingsJsonIn, Settings_t& stgsOut) {
  StaticJsonDocument<512> jsonDoc;

  jsonDoc.clear();
  auto error = deserializeJson(jsonDoc, settingsJsonIn);

  if (!error) {
    String mode_l = jsonDoc["config"]["mode"];
    String ctheme_l = jsonDoc["config"]["currentTheme"];

    stgsOut.mode = mode_l;
    stgsOut.current_theme = ctheme_l;
    stgsOut.themeNum = jsonDoc["config"]["themeNum"];  // todo: redundant
    stgsOut.delay = jsonDoc["config"]["delay"];

    stgsOut.timeStartH = jsonDoc["config"]["startH"];
    stgsOut.timeStartM = jsonDoc["config"]["startM"];
    stgsOut.timeStopH = jsonDoc["config"]["stopH"];
    stgsOut.timeStopM = jsonDoc["config"]["stopM"];
  } else {
    Serial.printf("jsonToSettings: JSON deserialization error\r\n");
  }
}

int settingsToJson(Settings_t& stgsIn, String& settingsJsonOut) {
  StaticJsonDocument<512> jsonDoc;
  jsonDoc.clear();

  JsonObject configObj = jsonDoc.createNestedObject("config");
  configObj["mode"] = stgsIn.mode;
  configObj["currentTheme"] = stgsIn.current_theme;
  configObj["themeNum"] = stgsIn.themeNum;  // todo: redundant
  configObj["delay"] = stgsIn.delay;

  configObj["startH"] = stgsIn.timeStartH;
  configObj["startM"] = stgsIn.timeStartM;
  configObj["stopH"] = stgsIn.timeStopH;
  configObj["stopM"] = stgsIn.timeStopM;

  return serializeJson(jsonDoc, settingsJsonOut);
}

int saveThemesDescJson(String& themesDescJson) {
  int size;
  if (xSemaphoreTake(semFS, (TickType_t)1000)) {
    Serial.printf("Opening file: %s\r\n", themesDescFile);
    File file = SPIFFS.open(themesDescFile, FILE_WRITE);
    if (!file) {
      Serial.printf("- %d: failed to open file for writing\r\n", __LINE__);
      size = -1;
    } else {
      // file.flush();
      size = file.print(themesDescJson);
      file.close();
      Serial.printf("Save themes desc: [%s][%d Bytes]\r\n",
                    themesDescJson.c_str(), size);
    }

    xSemaphoreGive(semFS);
  } else {
    Serial.printf("Sem error WS\r\n");
    size = -1;
  }

  return size;
}

int loadThemesDescJson(String& themesDescJson) {
  uint8_t buf[512];
  int size;

  if (xSemaphoreTake(semFS, (TickType_t)1000)) {
    Serial.printf("Opening file: %s\r\n", themesDescFile);
    File file = SPIFFS.open(themesDescFile, FILE_READ);
    if (!file || file.isDirectory()) {
      Serial.printf("- %d: failed to open file for reading\r\n", __LINE__);
      size = -1;
    } else {
      size = file.read(buf, file.available());
      file.close();

      if (size < 0) {
        themesDescJson = String("");
      } else {
        buf[size] = 0;  // make a C-string
        themesDescJson = String((char*)buf);
      }

      Serial.printf("Load theme stgs: %d bytes [%s]\r\n", size,
                    themesDescJson.c_str());
    }
    xSemaphoreGive(semFS);
  } else {
    Serial.printf("Sem error WS\r\n");
    size = -1;
  }

  return size;
}

int jsonToThemesDesc(String& themesJsonIn, ThemesGlobal_t& thmsStgs) {
  StaticJsonDocument<2048> jsonDoc;  // about 100 bytes for one theme is needed

  jsonDoc.clear();
  auto error = deserializeJson(jsonDoc, themesJsonIn);

  if (!error) {
    thmsStgs.available = jsonDoc["themesConfig"]["themes"].size();

    Serial.printf("jsonToThemesStgs: available = %d\r\n", thmsStgs.available);

    for (int i = 0; i < thmsStgs.available; i++) {
      String name_l = jsonDoc["themesConfig"]["themes"][i]["name"];

      thmsStgs.themeName[i] = name_l;
      thmsStgs.pixelsNo[i] = jsonDoc["themesConfig"]["themes"][i]["pixelsNo"];
      thmsStgs.framesNo[i] = jsonDoc["themesConfig"]["themes"][i]["framesNo"];
    }

    for (int i = thmsStgs.available; i < MAXTHEMENO; i++) {
      thmsStgs.themeName[i] = "none";
      thmsStgs.pixelsNo[i] = 0;
      thmsStgs.framesNo[i] = 0;
    }
  } else {
    Serial.printf("jsonToThemesStgs: JSON deserialization error\r\n");
  }
}

int themesDescToJson(ThemesGlobal_t& thmsStgs, String& themesJsonOut) {
  StaticJsonDocument<2048> jsonDoc;
  jsonDoc.clear();

  JsonObject configObj = jsonDoc.createNestedObject("themesConfig");

  // configObj["available"] = thmsStgs.available;

  JsonArray themesObj = configObj.createNestedArray("themes");
  JsonObject theme[MAXTHEMENO];

  for (int i = 0; i < thmsStgs.available; i++) {
    theme[i] = themesObj.createNestedObject();
    theme[i]["name"] = thmsStgs.themeName[i];
    theme[i]["pixelsNo"] = thmsStgs.pixelsNo[i];
    theme[i]["framesNo"] = thmsStgs.framesNo[i];
  }

  return serializeJson(jsonDoc, themesJsonOut);
}

int saveThemeFrame(LedStripState* ls, int themeNo, int frameNo, int frameSize)
{
  int size;
  if (xSemaphoreTake(semFS, ( TickType_t)1000)) {
    String fileName = "/themes/theme_" + String(themeNo) + ".txt";
    File file = SPIFFS.open(fileName.c_str(), FILE_APPEND);
    if (!file) {
      Serial.printf("- %d: failed to open file for append\r\n", __LINE__);
      size =  -1;
    } else {
      if (frameNo == 0) {
        file.flush();
      }
      size = file.write((uint8_t*)ls, frameSize);
      file.close();

      Serial.printf("%s : Save theme: [%d][%d], RAM: %d\r\n", fileName.c_str(), themeNo, frameNo, esp_get_free_heap_size());
    }
    xSemaphoreGive(semFS);
  } else {
    Serial.printf("Sem error WS\r\n");
    size = -1;
  }

  return size;
}

// // TODO: save the whole frame at once. Ten append chyba coś psuje, bo każdy kolejny write dłużej trwa chyba
// int saveThemeFrame(LedStripState* ls, int themeNo, int frameNo, int frameSize, bool last) {
//   int size;
//   static File file;
//   if (xSemaphoreTake(semFS, (TickType_t)1000)) {
//     String fileName = "/themes/theme_" + String(themeNo) + ".txt";
//     if (frameNo == 0) {
//       file = SPIFFS.open(fileName.c_str(), FILE_APPEND);
//       file.flush();
//     }
//     size = file.write((uint8_t*)ls, frameSize);

//     if (last == true) {
//       file.close();
//     }

//     Serial.printf("%s : Save theme: [%d][%d], RAM: %d\r\n", fileName.c_str(),
//                   themeNo, frameNo, esp_get_free_heap_size());
//     xSemaphoreGive(semFS);
//   } else {
//     Serial.printf("Sem error WS\r\n");
//     size = -1;
//   }

//   return size;
// }

int loadThemeFrame(LedStripState* ls, int themeNo, int frameNo, int frameSize) {
  int size;
  if (xSemaphoreTake(semFS, (TickType_t)1000)) {
    String fileName = "/themes/theme_" + String(themeNo) + ".txt";
    File file = SPIFFS.open(fileName.c_str(), FILE_READ);

    if (!file || file.isDirectory()) {
      Serial.printf("- %d: failed to open file for readng\r\n", __LINE__);
      size = -1;
    } else {
      file.seek(frameNo * frameSize);
      size = file.read((uint8_t*)ls, frameSize);
      file.close();

      Serial.printf("%s : Load theme: [%d][%d], RAM: %d\r\n", fileName.c_str(),
                    themeNo, frameNo, esp_get_free_heap_size());
    }

    xSemaphoreGive(semFS);
  } else {
    Serial.printf("Sem error WS\r\n");
    size = -1;
  }

  return size;
}

int resetThemes() {
  String defaultTheme = "{\"themesConfig\":{\"themes\":[]}}";
  saveThemesDescJson(defaultTheme);
  if (xSemaphoreTake(semFS, (TickType_t)1000)) {
    // SPIFFS.remove(themesDescFile); //todo do not remove but only change
    SPIFFS.rmdir("/themes");
    SPIFFS.mkdir("/themes");

    listDir(SPIFFS, "/", 0);

    xSemaphoreGive(semFS);
  } else {
    Serial.printf("Sem error WS\r\n");
  }

  return 1;
}

int format_fs() {
  SPIFFS.format();
}
