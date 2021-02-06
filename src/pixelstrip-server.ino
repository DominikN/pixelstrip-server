#include <WiFi.h>
#include <WiFiMulti.h>
#include <NeoPixelBus.h>
#include <Husarnet.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <PID_v1.h>

#include <WebServer.h>

//#include "NVMmanager.h"
#include "FSmanager.h"

#include "time.h"

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

void printLocalTime()
{
  struct tm timeinfo;
  time_t rawtime;

  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  rawtime = mktime(&timeinfo);
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S"); //see http://www.cplusplus.com/reference/ctime/strftime/
  Serial.printf("raw time: %d\r\n", rawtime);
}

bool getTime(int& h, int& m) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return 0;
  } else {
    h = timeinfo.tm_hour;
    m = timeinfo.tm_min;
    return 1;
  }
}

/* ================ CONFIG SECTION START ==================== */
#define PIN           12   // Pin connected to PixelStrip

#define WEBSOCKET_PORT 8001

#if __has_include("credentials.h")
#include "credentials.h"
#else
#define NUM_NETWORKS  2   // number of Wi-Fi networks

// Add your networks credentials here
const char* ssidTab[NUM_NETWORKS] = {
"ssid-1",
"ssid-2",
};
const char* passwordTab[NUM_NETWORKS] = {
"pass-1",
"pass-1",
};

// Husarnet credentials
const char* hostName = "esp32strip";  //this will be the name of the 1st ESP32 device at https://app.husarnet.com

/* to get your join code go to https://app.husarnet.com
-> select network
-> click "Add element"
-> select "join code" tab
Keep it secret!
*/
const char* husarnetJoinCode = "xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx/xxxxxxxxxxxxxxxxxxxxxx";
#endif
/* ================ CONFIG SECTION END   ==================== */

const char* html =
#include "html.h"
  ;
const char* pixelstrip_js =
#include "pixelstrip_js.h"
  ;


WebServer server(8000);

//Define Variables we'll be connecting to
double Setpoint, Input, Output;

//Specify the links and initial tuning parameters
PID myPID(&Input, &Output, &Setpoint, 2.0, 0.5, 0.0, DIRECT);

QueueHandle_t queue;
SemaphoreHandle_t sem = NULL;

Settings_t stgs_g = {"auto", "none", -1, -1, 0, 0, 23, 59};
ThemesGlobal_t thms_g;

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(MAXNUMPIXELS, PIN);

WebSocketsServer webSocket = WebSocketsServer(WEBSOCKET_PORT);
WiFiMulti wifiMulti;

StaticJsonDocument<200> jsonDoc;
StaticJsonDocument<800> jsonDocTx;

bool recording = false;

void printCurrentSettings(int line)
{
  Serial.printf("> %d: print current settings:\r\n", line);
  Serial.printf("mode: %s\r\n", stgs_g.mode.c_str());
  Serial.printf("theme: %s (%d)\r\n", stgs_g.current_theme.c_str(), stgs_g.themeNum);
  Serial.printf("delay: %d\r\n", stgs_g.delay);
  Serial.printf("working time: [%d:%d] -> [%d:%d]\r\n", stgs_g.timeStartH, stgs_g.timeStartM, stgs_g.timeStopH, stgs_g.timeStopM);
  Serial.println();
}

void printCurrentThemes(int line)
{
  Serial.printf("> %d: print current themes (%d):\r\n", line, thms_g.available);
  for (int i = 0; i < thms_g.available; i++) {
    Serial.printf("%s (%d x %d)\r\n", thms_g.themeName[i].c_str(), thms_g.framesNo[i], thms_g.pixelsNo[i]);
  }
  Serial.println();
}

void printFS(int line)
{
  String settingsJson;
  String themesDescJson;

  loadSettingsJson(settingsJson);
  loadThemesDescJson(themesDescJson);

  Serial.printf("> %d: Files content:\r\n", line);
  Serial.printf("Settings: [%s]\r\n", settingsJson.c_str());
  Serial.printf("Themes:   [%s]\r\n", themesDescJson.c_str());
  Serial.println();
}


void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  LedStripState ledstrip;
  static uint16_t cnt = 0;
  static int n;

  switch (type) {
    case WStype_DISCONNECTED: {
        Serial.printf("[%u] Disconnected\r\n", num);
      }
      break;

    case WStype_CONNECTED: {
        Serial.printf("[%u] Connected\r\n", num);
      }
      break;

    case WStype_TEXT: {
        Serial.printf("[%s]\r\n", (char*)payload);

        jsonDoc.clear();
        auto error = deserializeJson(jsonDoc, payload);

        if (!error) {
          if (jsonDoc["validTimeSet"]) {
            String settingsJson;
            stgs_g.timeStartH = jsonDoc["validTimeSet"]["startH"];
            stgs_g.timeStartM = jsonDoc["validTimeSet"]["startM"];
            stgs_g.timeStopH = jsonDoc["validTimeSet"]["stopH"];
            stgs_g.timeStopM = jsonDoc["validTimeSet"]["stopM"];

            settingsToJson(stgs_g, settingsJson);
            saveSettingsJson(settingsJson);
          }
          if (jsonDoc["getConfig"]) {
            int getConfig = jsonDoc["getConfig"];
            if (getConfig) {
              String jsonStgs;
              String jsonThms;

              loadSettingsJson(jsonStgs);
              loadThemesDescJson(jsonThms);

              webSocket.sendTXT(num, jsonStgs);
              webSocket.sendTXT(num, jsonThms);
            }
          }
          if (jsonDoc["trigger"]) {
            String settingsJson;

            String mode_l = jsonDoc["trigger"]["mode"];
            String theme_l = jsonDoc["trigger"]["theme"];
            int delay_l = jsonDoc["trigger"]["delay"];

            stgs_g.mode = mode_l;
            stgs_g.current_theme = theme_l;

            int n = -1;

            Serial.printf("find theme number:\r\n");
            for (int i = 0; i < thms_g.available; i++) {
              Serial.printf("%s , %s\r\n", thms_g.themeName[i].c_str(), theme_l.c_str());
              if (thms_g.themeName[i] == theme_l) {
                n = i;
              }
            }
            stgs_g.themeNum = n;

            stgs_g.delay = delay_l;

            settingsToJson(stgs_g, settingsJson);
            saveSettingsJson(settingsJson);

            printCurrentSettings(__LINE__);
            printCurrentThemes(__LINE__);

            cnt = 0;
          }
          if (jsonDoc["save"]) {
            String theme_l = jsonDoc["save"]["theme"];
            int  numpixels_l = jsonDoc["save"]["numpixels"];
            int numframes_l = jsonDoc["save"]["numframes"];

            //n = getThemeNo(theme_l);
            n = -1;
            for (int i = 0; i < thms_g.available; i++) {
              if (thms_g.themeName[i] == theme_l) {
                n = i;
              }
            }

            if (n < 0) {  //received a new them, saving in memory
              n = thms_g.available;
              thms_g.available = thms_g.available + 1;
            }

            thms_g.themeName[n] = theme_l;
            thms_g.pixelsNo[n] = numpixels_l;
            thms_g.framesNo[n] = numframes_l;

            recording = true;
            cnt = 0;
          }
          if (jsonDoc["clear"]) {
            int clr = jsonDoc["clear"];
            if (clr) {
              String settingsJson;
              String themesDescJson;

              resetThemes();

              stgs_g.mode = "auto";
              stgs_g.current_theme = "none";
              stgs_g.themeNum = 0;
              stgs_g.delay = -1;

              settingsToJson(stgs_g, settingsJson);
              saveSettingsJson(settingsJson);

              thms_g.available = 0;

              themesDescToJson(thms_g, themesDescJson);
              saveThemesDescJson(themesDescJson);

              printCurrentSettings(__LINE__);
              printCurrentThemes(__LINE__);
            }
          }
        } else {
          Serial.printf("JSON parse error\r\n");
        }
      }
      break;

    case WStype_BIN: {
        //int n = thms_g.available - 1; //index of frame being saved in NVM

        for (int i = 0; i < thms_g.pixelsNo[n]; i++) {
          ledstrip.pixel[3 * i + 0] = (uint8_t)(*(payload + (i * 3 + 0)));
          ledstrip.pixel[3 * i + 1] = (uint8_t)(*(payload + (i * 3 + 1)));
          ledstrip.pixel[3 * i + 2] = (uint8_t)(*(payload + (i * 3 + 2)));
        }

        if ((stgs_g.mode == "auto") && (recording == false)) {
          xQueueSendToBack(queue, (void*)&ledstrip, 0);
          //Serial.printf("RAM: %d", esp_get_free_heap_size());
        }

        if ((recording == true)) {
          saveThemeFrame(&ledstrip, n, cnt, thms_g.pixelsNo[n]);
          cnt++;
          if (cnt >= thms_g.framesNo[n]) {

            String outJson;
            themesDescToJson(thms_g, outJson);
            saveThemesDescJson(outJson);  //save in memory only if binary data is saved successfully

            recording = false;

            printCurrentSettings(__LINE__);
            printCurrentThemes(__LINE__);
            Serial.printf("RAM: %d", esp_get_free_heap_size());
          }

        }
      }
      break;
    case WStype_ERROR: {
        Serial.printf("[%u] Error\r\n", num);
      }
      break;

    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;

    default:
      break;
  }
  xSemaphoreGive( sem );
}

//typedef struct {
//  String mode;
//  String current_theme;
//  int themeNum;
//  int delay;
//  int timeStartH;
//  int timeStartM;
//  int timeStopH;
//  int timeStopM;
//} Settings_t;
//
//typedef struct {
//  int available;
//  String themeName[MAXTHEMENO];
//  int pixelsNo[MAXTHEMENO];
//  int framesNo[MAXTHEMENO];
//} ThemesGlobal_t;

void fs_test_loop()
{
  String settingsJson;
  String themesDescJson;

  FSinit();

  stgs_g.mode = "mode1";
  stgs_g.current_theme = "theme1";
  stgs_g.themeNum = 0;
  stgs_g.delay = 50;
  stgs_g.timeStartH = 10;
  stgs_g.timeStartM = 15;
  stgs_g.timeStopH = 14;
  stgs_g.timeStopM = 30;

  thms_g.available = 1;
  thms_g.themeName[0] = "name0";
  thms_g.pixelsNo[0] = 100;
  thms_g.framesNo[0] = 100;

  settingsToJson(stgs_g, settingsJson);
  themesDescToJson(thms_g, themesDescJson);

  Serial.printf("\r\nsaveSettingsJson test ... \r\n");
  saveSettingsJson(settingsJson);

  Serial.printf("\r\nsaveThemesDescJson test ... \r\n");
  saveThemesDescJson(themesDescJson);

  printFS(__LINE__);

  while (1)
  {

  }
}

void setup() {
  Serial.begin(230400);

  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); //todo: move after wifi initialization

  Serial.printf("\r\nMAXBUFSIZE = %d\r\nMAXNUMPIXELS = %d\r\n", MAXBUFSIZE, MAXNUMPIXELS);

  /* Init queue for pixel strip frames (for "auto" mode only) */
  queue = xQueueCreate( MAXBUFSIZE, sizeof(LedStripState) );
  if ( queue == NULL ) {
    Serial.printf("Queue error\r\n");
    while (1);
  }
  xQueueReset(queue);

  sem = xSemaphoreCreateMutex();

  /* Load settings from NV memory */
  FSinit();

  String settingsJson;
  String themesDescJson;

  loadSettingsJson(settingsJson);
  if (loadThemesDescJson(themesDescJson) <= 0) {
    thms_g.available = 0;

    themesDescToJson(thms_g, themesDescJson);
    saveThemesDescJson(themesDescJson);
  }

  jsonToSettings(settingsJson, stgs_g);
  jsonToThemesDesc(themesDescJson, thms_g);

  printCurrentSettings(__LINE__);
  printCurrentThemes(__LINE__);

  printFS(__LINE__);

  fs_test_loop();


  /* Turn the PID on (for "auto" mode only) */
  myPID.SetOutputLimits(20, 200);
  myPID.SetMode(AUTOMATIC);
  Setpoint = MAXBUFSIZE / 2;


  /* Initialize a pixel strip */
  strip.Begin();
  strip.Show();


  xTaskCreatePinnedToCore(
    taskWifi,          /* Task function. */
    "taskWifi",        /* String with name of task. */
    (8192 + 4096),          /* Stack size in bytes. */
    NULL,             /* Parameter passed as input of the task */
    0,                /* Priority of the task. */
    NULL,             /* Task handle. */
    1);               /* Core where the task should run (z 0 działało) */

  xTaskCreatePinnedToCore(
    taskDisplay,          /* Task function. */
    "taskDisplay",        /* String with name of task. */
    8192,            /* Stack size in bytes. */
    NULL,             /* Parameter passed as input of the task */
    0,                /* Priority of the task. */
    NULL,            /* Task handle. */
    1);               /* Core where the task should run (z 0 działało) */
}

int tNow = 0;
bool timeValid = false;

void taskDisplay( void * parameter ) {
  LedStripState ls;

  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;

  uint16_t cnt = 0;

  while (1) {
    int n = stgs_g.themeNum;

    if ((stgs_g.mode == "auto") && (recording == false)) {
      if (pdTRUE == xQueueReceive(queue, (void*)&ls, 100 )) {
        Input = uxQueueMessagesWaiting(queue);
        myPID.Compute();
        myPID.SetSampleTime(int(Output));

        for (int i = 0; i < thms_g.pixelsNo[n]; i++) {
          //          red = ls.pixel[3 * i + 0];
          //          green = ls.pixel[3 * i + 1];
          green = ls.pixel[3 * i + 0];
          red = ls.pixel[3 * i + 1];

          blue = ls.pixel[3 * i + 2];
          strip.SetPixelColor(i, RgbColor(red, green, blue));
        }
        strip.Show();

        Serial.printf("%d;%f\r\n", uxQueueMessagesWaiting(queue), Output);
        delay(int(Output));
      }
    } else {
      if ((recording == false)) {
        int tStart = 60 * stgs_g.timeStartH + stgs_g.timeStartM;
        int tStop = 60 * stgs_g.timeStopH + stgs_g.timeStopM;

        //Serial.printf("%d ? [%d : %d], %d\r\n",tNow, tStart, tStop, timeValid);
        if (
          (timeValid == 0)
          || ((tStart <= tStop) && ((tNow >= tStart) && (tNow <= tStop)))
          || ((tStart > tStop) && ((tNow >= tStart) || (tNow <= tStop)))
        ) {
          loadThemeFrame(&ls, n, cnt, thms_g.pixelsNo[n]);
          //printLocalTime();

          for (int i = 0; i < thms_g.pixelsNo[n]; i++) {
            //          red = ls.pixel[3 * i + 0];
            //          green = ls.pixel[3 * i + 1];
            green = ls.pixel[3 * i + 0];
            red = ls.pixel[3 * i + 1];

            blue = ls.pixel[3 * i + 2];
            strip.SetPixelColor(i, RgbColor(red, green, blue));
          }

          strip.Show();

          cnt++;
          if (cnt >= thms_g.framesNo[n]) {
            if (stgs_g.mode == "infinite") {
              cnt = 0;
            }
            if (stgs_g.mode == "singleshot") {
              stgs_g.mode = "auto";
              cnt = 0;
            }
          }

          delay(stgs_g.delay);
        } else {
          for (int i = 0; i < thms_g.pixelsNo[n]; i++) {
            strip.SetPixelColor(i, RgbColor(0, 0, 0));
          }
          strip.Show();
          delay(100);
        }
      } else {
        delay(100);
      }
    }
  }
}

void taskWifi( void * parameter ) {
  uint8_t stat = WL_DISCONNECTED;
  int h, m;

  /* Add Wi-Fi network credentials */
  for (int i = 0; i < NUM_NETWORKS; i++) {
    wifiMulti.addAP(ssidTab[i], passwordTab[i]);
    Serial.printf("WiFi %d: SSID: \"%s\" ; PASS: \"%s\"\r\n", i, ssidTab[i], passwordTab[i]);
  }

  while (stat != WL_CONNECTED) {
    stat = wifiMulti.run();
    Serial.printf("WiFi status: %d\r\n", (int)stat);
    delay(100);
  }
  Serial.printf("WiFi connected: %d\r\n", (int)stat);

  Husarnet.join(husarnetJoinCode, hostName);
  Husarnet.start();

  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);

  server.on("/", HTTP_GET, []() {
    Serial.printf("HTTP_GET / [size %d B] [RAM: %d]\r\n", strlen(html), esp_get_free_heap_size());
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", html);
    xSemaphoreGive( sem );
  });
  server.on("/index.html", HTTP_GET, []() {
    Serial.printf("HTTP_GET / [size %d B] [RAM: %d]\r\n", strlen(html), esp_get_free_heap_size());
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", html);
    xSemaphoreGive( sem );
  });

  server.on("/pixelstrip.js", HTTP_GET, []() {
    Serial.printf("HTTP_GET /pixelstrip.js [%d B] [RAM: %d]\r\n", strlen(pixelstrip_js), esp_get_free_heap_size());
    server.sendHeader("Connection", "close");
    server.send(200, "application/javascript", pixelstrip_js);
    xSemaphoreGive( sem );
  });

  server.begin();

  while (1) {
    while (WiFi.status() == WL_CONNECTED) {
      webSocket.loop();
      server.handleClient();

      timeValid = getTime(h, m);
      tNow = 60 * h + m;
      xSemaphoreTake(sem, ( TickType_t)10);
      //delay(2);
    }
    Serial.printf("WiFi disconnected, reconnecting\r\n");
    stat = wifiMulti.run();
    Serial.printf("WiFi status: %d\r\n", (int)stat);
    delay(500);
  }
}

void loop() {
  delay(50);
}
