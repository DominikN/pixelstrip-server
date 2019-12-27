#include <WiFi.h>
#include <WiFiMulti.h>
#include <NeoPixelBus.h>
#include <Husarnet.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <PID_v1.h>

#include <Preferences.h>

/* ================ CONFIG SECTION START ==================== */
#define PIN           12   // Pin connected to PixelStrip

#define MAXNUMPIXELS  150   // How many NeoPixels are attached to the Arduino?
#define MAXBUFSIZE    40   // buffer size for storing LED strip state (each NUMPIXELS * 3 size)
#define MAXTHEMENO    10

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

#define NS_SETTINGS   "settings"
#define NS_THEMES     "themes"


/* ================ CONFIG SECTION END   ==================== */

Preferences preferences;
String mode_s;
String theme_s;
int theme_num_s;

int numpixels_s;
int numframes_s;
int delay_s;

//Define Variables we'll be connecting to
double Setpoint, Input, Output;

//Specify the links and initial tuning parameters
PID myPID(&Input, &Output, &Setpoint, 2.0, 0.5, 0.0, DIRECT);

typedef struct {
  uint8_t pixel[3 * MAXNUMPIXELS];
} LedStripState;

QueueHandle_t queue;
SemaphoreHandle_t sem = NULL;
SemaphoreHandle_t semNVM = NULL;

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(MAXNUMPIXELS, PIN);

WebSocketsServer webSocket = WebSocketsServer(WEBSOCKET_PORT);
WiFiMulti wifiMulti;

StaticJsonDocument<200> jsonDocument;

bool recording = false;

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

      Serial.printf("<%s -> %s>", key.c_str(), theme1.c_str());
      Serial.printf(".");
      if (theme.equals(theme1)) {
        num = i;
        Serial.printf("done, n = %d\r\n", num);
        break;
      }
    }
    if (i == MAXTHEMENO) {
      num = -1;
      Serial.printf("fail\r\n");
    }
    preferences.end();
    xSemaphoreGive(semNVM);
  } else {
    Serial.printf("NVM error WS\r\n");
  }
  return num;
}

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

        jsonDocument.clear();
        auto error = deserializeJson(jsonDocument, payload);

        if (!error) {
          if (jsonDocument["trigger"]) {
            String mode_l = jsonDocument["trigger"]["mode"];
            String theme_l = jsonDocument["trigger"]["theme"];
            int delay_l = jsonDocument["trigger"]["delay"];

            mode_s = mode_l;
            theme_s = theme_l;
            delay_s = delay_l;

            theme_num_s = getThemeNo(theme_s);

            if (mode_s == "infinite") {
              if (xSemaphoreTake(semNVM, ( TickType_t)1000)) {
                preferences.begin(NS_SETTINGS, false);
                preferences.putString("strip_mode", mode_s.c_str());
                preferences.putString("strip_theme", theme_s.c_str());
                preferences.putInt("strip_delay", delay_s);
                preferences.end();
                xSemaphoreGive(semNVM);
              } else {
                Serial.printf("NVM error WS\r\n");
              }
            }
          }
          if (jsonDocument["save"]) {

            String theme_l = jsonDocument["save"]["theme"];
            int  numpixels_l = jsonDocument["save"]["numpixels"];
            int numframes_l = jsonDocument["save"]["numframes"];

            recording = true;
            cnt = 0;
            numpixels_s = numpixels_l;
            numframes_s = numframes_l;

            if (xSemaphoreTake(semNVM, ( TickType_t)1000)) {
              preferences.begin(NS_THEMES, false);

              int avaiable = preferences.getInt("available");

              preferences.putString(String("theme[" + String(avaiable) + "]n").c_str(), theme_l);
              preferences.putInt(String("theme[" + String(avaiable) + "]x").c_str(), numpixels_l);
              preferences.putInt(String("theme[" + String(avaiable) + "]y").c_str(), numframes_l);

              avaiable++;
              preferences.putInt("available", avaiable);

              preferences.end();

              xSemaphoreGive(semNVM);
            } else {
              Serial.printf("NVM error WS\r\n");
            }

            Serial.printf("Free NVM: %d\r\n", preferences.freeEntries());
          }
          if (jsonDocument["clear"]) {
            int clr = jsonDocument["clear"];
            if (clr) {
              Serial.printf("Clear NVM ... ");
              if (xSemaphoreTake(semNVM, ( TickType_t)1000)) {

                preferences.begin(NS_SETTINGS, false);
                preferences.putString("strip_mode", "auto");
                preferences.putString("strip_theme", "none");
                preferences.putInt("strip_delay", -1);

                mode_s = preferences.getString("strip_mode");
                theme_s = preferences.getString("strip_theme");
                delay_s = preferences.getInt("strip_delay");
                preferences.end();

                preferences.begin(NS_THEMES, false);
                preferences.clear();
                preferences.end();

                xSemaphoreGive(semNVM);
              } else {
                Serial.printf("NVM error WS\r\n");
              }

              Serial.println();
              Serial.printf("Free NVM: %d\r\n", preferences.freeEntries());
            }
          }
        } else {
          Serial.printf("JSON parse error\r\n");
        }
      }
      break;

    case WStype_BIN: {
        for (int i = 0; i < numpixels_s; i++) {
          ledstrip.pixel[3 * i + 0] = (uint8_t)(*(payload + (i * 3 + 0)));
          ledstrip.pixel[3 * i + 1] = (uint8_t)(*(payload + (i * 3 + 1)));
          ledstrip.pixel[3 * i + 2] = (uint8_t)(*(payload + (i * 3 + 2)));
        }

        if ((mode_s == "auto") && (recording == false)) {
          xQueueSendToBack(queue, (void*)&ledstrip, 0);
          //Serial.printf("RAM: %d", esp_get_free_heap_size());
        }

        if ((recording == true)) {

          if (xSemaphoreTake(semNVM, ( TickType_t)1000)) {
            preferences.begin(NS_THEMES, false);

            int avaiable = preferences.getInt("available");
            String key = String("theme[" + String(avaiable - 1) + "][" + String(cnt) + "]");

            preferences.putBytes(key.c_str(), (void*)&ledstrip, sizeof(ledstrip));
            preferences.end();

            xSemaphoreGive(semNVM);

            Serial.printf("write: %s\r\n", key.c_str());
          } else {
            Serial.printf("NVM error WS\r\n");
          }

          cnt++;
          if (cnt >= numframes_s) {
            recording = false;
            Serial.printf("\r\n(%d x %d) frames written to NVM\r\n", numpixels_s, numframes_s);
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
  semNVM = xSemaphoreCreateMutex();
  xSemaphoreGive(semNVM);

  /* Load settings from NV memory */
  preferences.begin(NS_SETTINGS, false);
  mode_s = preferences.getString("strip_mode");  // "auto" or "sequence"
  if (mode_s == "") {
    preferences.putString("strip_mode", "auto");
    preferences.putString("strip_theme", "none");
    preferences.putInt("strip_delay", -1);
  }

  theme_s = preferences.getString("strip_theme");
  delay_s = preferences.getInt("strip_delay");

  Serial.printf("\r\nPixel Strip settings:\r\nmode = %s\r\ntheme = %s\r\ndelay = %d\r\n", mode_s.c_str(), theme_s.c_str(), delay_s);
  preferences.end();

  preferences.begin(NS_THEMES, false);
  if (preferences.getString("theme[0]n") == "") {
    preferences.putInt("available", 0);
  }
  preferences.end();

  theme_num_s = getThemeNo(theme_s);

  preferences.begin(NS_THEMES, false);
  numpixels_s = preferences.getInt(String("theme[" + String(numframes_s) + "]x").c_str());
  numframes_s = preferences.getInt(String("theme[" + String(numframes_s) + "]y").c_str());
  preferences.end();

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
    if ((mode_s == "auto") && (recording == false)) {
      if (pdTRUE == xQueueReceive(queue, (void*)&ls, 100 )) {
        Input = uxQueueMessagesWaiting(queue);
        myPID.Compute();
        myPID.SetSampleTime(int(Output));

        for (int i = 0; i < numpixels_s; i++) {
          red = ls.pixel[3 * i + 0];
          green = ls.pixel[3 * i + 1];
          blue = ls.pixel[3 * i + 2];
          strip.SetPixelColor(i, RgbColor(red, green, blue));
        }
        strip.Show();

        Serial.printf("%d;%f\r\n", uxQueueMessagesWaiting(queue), Output);
        delay(int(Output));
      }
    } else {
      if ((recording == false)) {

        String key = String("theme[" + String(theme_num_s) + "][" + String(cnt) + "]");

        Serial.printf("read: %s, %d\r\n", key.c_str(), delay_s);

        if (xSemaphoreTake(semNVM, ( TickType_t)1000)) {
          preferences.begin(NS_THEMES, true);
          preferences.getBytes(key.c_str(), (void*)&ls, sizeof(ls));
          preferences.end();
          xSemaphoreGive(semNVM);
        } else {
          Serial.printf("NVM error loop\r\n");
        }

        for (int i = 0; i < numpixels_s; i++) {
          red = ls.pixel[3 * i + 0];
          green = ls.pixel[3 * i + 1];
          blue = ls.pixel[3 * i + 2];
          strip.SetPixelColor(i, RgbColor(red, green, blue));
        }

        strip.Show();

        cnt++;
        if (cnt >= numframes_s) {
          if (mode_s == "infinite") {
            cnt = 0;
          }
          if (mode_s == "singleshot") {
            mode_s = "auto";
            cnt = 0;
          }
        }

        delay(delay_s);
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

  while (1) {
    while (WiFi.status() == WL_CONNECTED) {
      webSocket.loop();
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
