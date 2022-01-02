#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "ArduinoJson.h"
#include "ESPmDNS.h"
#include "Arduino.h"
#include "WiFiAP.h"
#include "PubSubClient.h"
#include "ArduinoOTA.h"
//------------setting constants for servers-----------------------------------------------------/
const char *ssid = "";
const char *password = "";
const char *mqtt_ip = "";
const char *mqttPort = "";
const char *user = "";
const char *mqtt_topic = "";
const char *mqtt_password = "";
unsigned long previousMillis = 0;
const long interval = 30000;
WiFiClient espClient;
PubSubClient client(espClient);
#define mqtt_read read_file("/mqtt.json", "ip", "port", "user", "topic", "password")
#define wifi_read read_file("/wifi.json", "ssid", "password", "", "", "")
//-------------access point mode constants------------------------------------------------------/
const char *AP_mode_ssid = "garage";
const char *AP_mode_password = "door";
#define staTimeOut 10000
IPAddress AP_ip(192, 168, 4, 2);
IPAddress AP_gate(192, 168, 4, 1);
IPAddress AP_netmask(255, 255, 255, 0);
//----------------------------------------------------------------------------------------------/
//--------------------------setup debug---------------------------------------------------------/
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)

//---------------------------set door sersor----------------------------------------------------/
#define door_sensor_pin 19
#define opener_pin 22
#define light_pin 21
#define led_pin 2
bool ledStatus = false;
int last_door_state;
int current_door_state;
const char *door_state;
AsyncWebServer server(80);

void setup()
{

  Serial.begin(115200);
  ArduinoOTA.setHostname("ESP_Garage_Door");
  ArduinoOTA.setPassword("0987");

  set_ap_mode(); // starts ap mode. this prevented reboot loop with out wifi being started???
  //----------------starts MDNS server--------------------------------------------------/

  ArduinoOTA
      .onStart([]()
               {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type); })
      .onEnd([]()
             { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed"); });
  ArduinoOTA.begin();
  Serial.println("OTA ready");
  //------------starts SPIFFS so files can be read from memory--------------------/
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  };
  //----------------------handles webpage requests and pulls pages from SPIFFS----------------------/
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              int paramsNr = request->params();
              Serial.println(paramsNr);
              String param_value;
              String door_pos;
              String door_relay;
              for (int i = 0; i < paramsNr; i++)
              {

                AsyncWebParameter *p = request->getParam(i);
                Serial.print("Param name: ");
                Serial.println(p->name());
                Serial.print("Param value: ");
                Serial.println(p->value());
                Serial.println("------");
                param_value = p->name();
                const char *door_pos(param_value.c_str());
                debugln("-----------");
                request->send(200, door_pos, "text/plain");
              }
              debugln("door_pos:");
              debug(param_value);
              if (param_value == "door_open")
              {
                digitalWrite(opener_pin, LOW);
                delay(200);
                digitalWrite(opener_pin, HIGH);
              }
              else if (param_value == "door_closed")
              {
                digitalWrite(opener_pin, HIGH);
                delay(200);
                digitalWrite(opener_pin, LOW);
              }

              request->send(SPIFFS, "/index.html", "text/html"); });
  server.on("/network.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/network.html", "text/html"); });
  server.on("/mqtt.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/mqtt.html", "text/html"); });
  server.on("/mqtt.json", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/mqtt.json", "application/json"); });
  server.on("/wifi.json", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/wifi.json", "application/json"); });
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/script.js", "text/js"); });
  server.on("/fontawesome.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/fontawesome.js", "text/js"); });
  server.on("/solid.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/solid.js", "text/js"); });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/style.css", "text/css"); });
  server.on("/garagedoor.jpg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/garagedoor.jpg", "text/css"); });
  server.on("/garage_state", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", garage_state().c_str()); });
  server.begin();

  //----------------------------- post from webpage--------------------------------/
  server.on(
      "/mqtt.json", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
      {
        for (size_t i = 0; i < len; i++)
        {
          Serial.write(data[i]);
        }
        request->send(200);
        String _setting = (char *)data;
        setup_json(_setting, "/mqtt.json");
      });

  server.on(
      "/wifi.json", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
      {
        for (size_t i = 0; i < len; i++)
        {
          Serial.write(data[i]);
        }
        request->send(200);
        String _setting = (char *)data;
        setup_json(_setting, "/wifi.json");
      });
  wifi_read; // called to pull network settings out of spiffs and connect to local network if avalible

  pinMode(door_sensor_pin, INPUT_PULLUP);
  pinMode(opener_pin, OUTPUT);
  pinMode(light_pin, OUTPUT);
  pinMode(led_pin, OUTPUT);
  digitalWrite(opener_pin, HIGH);
  digitalWrite(light_pin, HIGH);
  current_door_state = digitalRead(door_sensor_pin);
};

//----------------------main loop------------------------------------------------------------/
void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    startWifi();
    toggleLED();
  }
  if (!client.connected())
  {
    mqtt_read;
  }
  client.loop();

  garage_state();
  ArduinoOTA.handle();
}
void toggleLED()
{
  if (!ledStatus)
  {
    digitalWrite(led_pin, HIGH);
    ledStatus = true;
  }
  digitalWrite(led_pin, LOW);
  ledStatus = false;
  delay(1000);
  return;
}
//------------------door state----------------------------------------------------------------/
String garage_state()
{
  last_door_state = current_door_state;
  current_door_state = digitalRead(door_sensor_pin);
  if (current_door_state == 0)
  {
    door_state = "closed";
  }
  else
  {
    door_state = "open";
  }

  if (last_door_state != current_door_state)
  {
    debugln("sending MQTT message");
    debugln(mqtt_topic);
    client.publish("homeassistant/cover/garage", door_state);
  }
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    client.publish("homeassistant/cover/garage", door_state);
    previousMillis = currentMillis;
  }
  return door_state;
  delay(500);
}
//------------------write data from web page--------------------------------------------------/
String setup_json(String _data, String my_file)
{
  File fileToWrite = SPIFFS.open(my_file, "w");
  if (fileToWrite.print(_data))
  {
  };
  fileToWrite.close();
  if (my_file == "/mqtt.json")
  {
    read_file(my_file, "ip", "port", "user", "topic", "password");
  }
  else
  {
    read_file(my_file, "ssid", "password", "", "", "");
  }
};
//------------------read data from SPIFFS------------------------------------------------------/
void read_file(String new_file, String v, String w, String x, String y, String z)
{

  File fileToRead = SPIFFS.open(new_file);

  StaticJsonDocument<192> doc;

  DeserializationError error = deserializeJson(doc, fileToRead);

  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }
  fileToRead.close();
  if (new_file == "/mqtt.json")
  {
    mqtt_ip = doc[v]; // "192.16.15.46:8945"
    mqttPort = doc[w];
    user = doc[x];          // "craig"
    mqtt_topic = doc[y];    // "home/garage/"
    mqtt_password = doc[z]; // "mqtt"
    start_mqtt();
  }
  else if (new_file == "/wifi.json")
  {
    ssid = doc[v];
    password = doc[w];

    startWifi();
  }
};
//---------------------setting up wifi-----------------------------------------------------------/
void startWifi()
{
  debugln("ssid & password:");
  debug(ssid);
  debugln(password);
  if (WiFi.status() != WL_CONNECTED)
  {
    debugln("call set_station_mode");
    set_station_mode();
  }
  else
  {
    debugln("call set_ap_mode");
    set_ap_mode();
  }
  if (!MDNS.begin("garage"))
  { // esp.local/
    Serial.println("MDNS responder not started");
  };
  MDNS.addService("_http", "_tcp", 80);
  MDNS.addServiceTxt("_http", "_tcp", "board", "ESP32");
};

//---------------------start wifi ap mode--------------------------------------------------------/
void set_ap_mode()
{
  Serial.println("Switching to AP mode");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_mode_ssid);
  debugln(AP_mode_ssid);
  debugln(AP_mode_password);
  WiFi.softAPConfig(AP_ip, AP_gate, AP_netmask);
  debugln();
  Serial.print("IP address: ");
  debugln(WiFi.softAPIP());
};
//----------------------start wifi station mode(connect to home network)-------------------------/

void set_station_mode()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Connecting to wifi");

    delay(500);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < staTimeOut)
    {
      Serial.print(".");
      delay(500);
    }
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    debugln();
    debugln("Connected");
    debugln(WiFi.localIP());
    WiFi.softAPdisconnect(true);
    return;
  }
  else if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.mode(WIFI_AP);
    debugln();
    debugln("Failed to connect.");
    debugln("set station function---WiFi.SOftAPIP=");
    Serial.print(WiFi.softAPIP());
    debugln();
    debug("AP_ip:");
    debugln(AP_ip);
    if (WiFi.softAPIP() != AP_ip)
    {
      debugln("going back to ap_setup");
      set_ap_mode();
    }
  }
};
// -------------------------starting mqtt------------------------------------------------ /
void start_mqtt()
{
  if (mqtt_ip == "")
  {
    mqtt_read;
  }

  int i = atoi(mqttPort);

  client.setServer(mqtt_ip, i);
  client.setCallback(callback);
  if (!client.connected())
  {
    Serial.println("Connecting to MQTT...");

    if (client.connect("ESP32Client", user, mqtt_password))
    {

      Serial.println("connected");
    }
    else
    {

      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
    debugln("CLIENTS CALLED");
    client.publish(mqtt_topic, "");
    debugln(mqtt_topic);
    client.subscribe(mqtt_topic);
  };
};
void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println(messageTemp);
  if (messageTemp == "toggle")
  {
    digitalWrite(opener_pin, LOW);
    delay(500);
    digitalWrite(opener_pin, HIGH);
  }
}
