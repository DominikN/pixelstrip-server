#include <WiFi.h>
#include <WiFiMulti.h>
#include <NeoPixelBus.h>
#include <Husarnet.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <PID_v1.h>

#include <WebServer.h>

#include "NVMmanager.h"

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


WebServer server(8000);

//Define Variables we'll be connecting to
double Setpoint, Input, Output;

//Specify the links and initial tuning parameters
PID myPID(&Input, &Output, &Setpoint, 2.0, 0.5, 0.0, DIRECT);

QueueHandle_t queue;
SemaphoreHandle_t sem = NULL;

Settings_t stgs_g;
ThemesGlobal_t thms_g;

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(MAXNUMPIXELS, PIN);

WebSocketsServer webSocket = WebSocketsServer(WEBSOCKET_PORT);
WiFiMulti wifiMulti;

StaticJsonDocument<200> jsonDoc;
StaticJsonDocument<400> jsonDocTx;

bool recording = false;


void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  LedStripState ledstrip;
  static uint16_t cnt = 0;

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
          if (jsonDoc["getConfig"]) {
            int getConfig = jsonDoc["getConfig"];
            if (getConfig) {
              String jsonTx;

              jsonDocTx.clear();
              JsonObject configObj  = jsonDocTx.createNestedObject("config");
              configObj["mode"] = stgs_g.mode;
              configObj["currentTheme"] = stgs_g.current_theme;
              configObj["themeNum"] = stgs_g.themeNum;    //todo: redundant
              configObj["delay"] = stgs_g.delay;
              configObj["available"] = thms_g.available;  //todo: redundant

              JsonArray themesObj = jsonDocTx.createNestedArray("themes");
              JsonObject theme[MAXTHEMENO];

              for (int i = 0; i < thms_g.available; i++) {
                theme[i] = themesObj.createNestedObject();
                theme[i]["name"] = thms_g.themeName[i];
                theme[i]["pixelsNo"] = thms_g.pixelsNo[i];
                theme[i]["framesNo"] = thms_g.framesNo[i];
              }

              serializeJson(jsonDocTx, jsonTx);
              Serial.printf("\r\njsonTx: [ %s ]\r\n", jsonTx.c_str());

              webSocket.sendTXT(num, jsonTx);
            }
          }
          if (jsonDoc["trigger"]) {
            String mode_l = jsonDoc["trigger"]["mode"];
            String theme_l = jsonDoc["trigger"]["theme"];
            int delay_l = jsonDoc["trigger"]["delay"];

            stgs_g.mode = mode_l;
            stgs_g.current_theme = theme_l;
            stgs_g.themeNum = getThemeNo(theme_l);
            stgs_g.delay = delay_l;

            saveSettings(&stgs_g);

            Serial.printf("Triggering:\r\nmode: %s\r\ntheme: %s [no: %d]\r\ndelay: %d",
                          stgs_g.mode.c_str(), stgs_g.current_theme.c_str(), stgs_g.themeNum, stgs_g.delay);

            cnt = 0;
          }
          if (jsonDoc["save"]) {
            int n = thms_g.available;

            String theme_l = jsonDoc["save"]["theme"];
            int  numpixels_l = jsonDoc["save"]["numpixels"];
            int numframes_l = jsonDoc["save"]["numframes"];

            thms_g.themeName[n] = theme_l;
            thms_g.pixelsNo[n] = numpixels_l;
            thms_g.framesNo[n] = numframes_l;
            n++;
            thms_g.available = n;

            Serial.printf("available themes:\r\n");
            for (int i = 0; i < n; i++) {
              Serial.printf("%s (%d x %d)\r\n", thms_g.themeName[i].c_str(), thms_g.framesNo[i], thms_g.pixelsNo[i]);
            }

            saveThemesGlobal(&thms_g);

            recording = true;
            cnt = 0;
          }
          if (jsonDoc["clear"]) {
            int clr = jsonDoc["clear"];
            if (clr) {
              stgs_g.mode = "auto";
              stgs_g.current_theme = "none";
              stgs_g.themeNum = -1;
              stgs_g.delay = -1;

              saveSettings(&stgs_g);
              resetThemes();
              loadThemesGlobal(&thms_g);
            }
          }
        } else {
          Serial.printf("JSON parse error\r\n");
        }
      }
      break;

    case WStype_BIN: {
        int n = thms_g.available - 1; //index of frame being saved in NVM

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

          saveThemeFrame(&ledstrip, n, cnt);
          cnt++;
          if (cnt >= thms_g.framesNo[n]) {
            recording = false;
            Serial.printf("\r\n(%d x %d) frames written to NVM\r\n", thms_g.framesNo[n], thms_g.pixelsNo[n]);
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

void setup() {
  Serial.begin(230400);

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
  nvmInit();
  loadSettings(&stgs_g);
  loadThemesGlobal(&thms_g);

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
    10000,            /* Stack size in bytes. */
    NULL,             /* Parameter passed as input of the task */
    4,                /* Priority of the task. */
    NULL,             /* Task handle. */
    1);               /* Core where the task should run (z 0 działało) */

  xTaskCreate(
    taskDisplay,          /* Task function. */
    "taskDisplay",        /* String with name of task. */
    10000,            /* Stack size in bytes. */
    NULL,             /* Parameter passed as input of the task */
    4,                /* Priority of the task. */
    NULL);             /* Task handle. */
}

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
        loadThemeFrame(&ls, n, cnt);

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
        delay(10);
      }
    }
  }
}

void taskWifi( void * parameter ) {
  uint8_t stat = WL_DISCONNECTED;

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
  Serial.printf("WiFi connected\r\n", (int)stat);

  Husarnet.join(husarnetJoinCode, hostName);
  Husarnet.start();

  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);

  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    //server.sendHeader("Connection", "keep-alive");
    server.send(200, "text/html", html);
  });

  server.on("/content.html", HTTP_GET, []() {
    String btns;

    for (int i = 0; i < thms_g.available; i++) {
      btns += String(
                String("<button type=\"button\" class=\"btn btn-lg btn-block btn-primary\" onmousedown='triggerMode(\"")
                + String(thms_g.themeName[i])
                + String("\", 50)' ontouchstart='triggerMode(\"")
                + String(thms_g.themeName[i])
                + String("\", 50)'>")
                + String(thms_g.themeName[i])
                + String("</button>")
              );
      //      if (i == stgs_g.themeNum) {
      //        btns += String(
      //                  String("<button type=\"button\" class=\"btn btn-lg btn-block btn-success\" onmousedown='triggerMode(\"")
      //                  + String(thms_g.themeName[i])
      //                  + String("\", 50)' ontouchstart='triggerMode(\"")
      //                  + String(thms_g.themeName[i])
      //                  + String("\", 50)'>")
      //                  + String(thms_g.themeName[i])
      //                  + String("</button>")
      //                );
      //      } else {
      //        btns += String(
      //                  String("<button type=\"button\" class=\"btn btn-lg btn-block btn-secondary\" onmousedown='triggerMode(\"")
      //                  + String(thms_g.themeName[i])
      //                  + String("\", 50)' ontouchstart='triggerMode(\"")
      //                  + String(thms_g.themeName[i])
      //                  + String("\", 50)'>")
      //                  + String(thms_g.themeName[i])
      //                  + String("</button>")
      //                );
      //      }
      Serial.printf("Button HTML: [%s]", btns.c_str());
    }

    server.sendHeader("Connection", "close");
    server.send(200, "text/html", btns);
  });

  server.begin();

  while (1) {
    while (WiFi.status() == WL_CONNECTED) {
      webSocket.loop();
      server.handleClient();
      xSemaphoreTake(sem, ( TickType_t)10);
    }
    Serial.printf("WiFi disconnected, reconnecting\r\n");
    stat = wifiMulti.run();
    Serial.printf("WiFi status: %d\r\n", (int)stat);
  }
}

void loop() {
  delay(50);
}
