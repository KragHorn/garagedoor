#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "ArduinoJson.h"
#include "ESPmDNS.h"
#include "Arduino.h"
#include "WiFiAP.h"
//------------setting constants for servers-----------------------------------------------------/
const char* ssid = "";
const char* password = "";
const char* ip = "";
const char* user = "";
const char* topic = "";
const char* mqtt_password = "";

//-------------access point mode constants------------------------------------------------------/
const char* AP_mode_ssid = "garage";
const char* AP_mode_password = "door";
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
int last_door_state;
int current_door_state;

AsyncWebServer server(80);


void setup() {

  Serial.begin(115200);
  
  set_ap_mode(); //starts ap mode. this prevented reboot loop with out wifi being started???
  //----------------starts MDNS server--------------------------------------------------/
  if (MDNS.begin("garage.local")) { //esp.local/
    Serial.println("MDNS responder started");
  };
  //------------starts SPIFFS so files can be read from memory--------------------/
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  };
  //----------------------handles webpage requests and pulls pages from SPIFFS----------------------/
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });
  server.on("/mqtt.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/mqtt.html", "text/html");
  });
  server.on("/mqtt.json", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/mqtt.json", "application/json");
  });
  server.on("/wifi.json", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/wifi.json", "application/json");
  });
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/script.js", "text/js");
  });
  server.on("/fontawesome.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/fontawesome.js", "text/js");
  });
  server.on("/solid.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/solid.js", "text/js");
  });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });
  server.on("/garagedoor.jpg", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/garagedoor.jpg", "text/css");
  });
  server.begin();
  //----------------------------- post from webpage--------------------------------/
  server.on("/mqtt.json", HTTP_POST, [](AsyncWebServerRequest * request) {}, NULL,
  [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
    for (size_t i = 0; i < len; i++)  {
      Serial.write(data[i]);
    }
    request->send(200);
    String _setting = (char*)data;
    setup_json(_setting, "/mqtt.json");

  } )  ;


  server.on("/wifi.json", HTTP_POST, [](AsyncWebServerRequest * request) {}, NULL,
  [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
    for (size_t i = 0; i < len; i++)  {
      Serial.write(data[i]);
    }
    request->send(200);
    String _setting = (char*)data;
    setup_json(_setting, "/wifi.json");

  } )  ;
  read_file("/wifi.json", "ssid", "password", "", "");//called to pull network settings out of spiffs
  pinMode(door_sensor_pin, INPUT_PULLUP);
  current_door_state = digitalRead(door_sensor_pin);
};
//----------------------main loop------------------------------------------------------------/
void loop() {
  last_door_state = current_door_state;
  current_door_state = digitalRead(door_sensor_pin);
  if (last_door_state != current_door_state){
    debugln("sending MQTT message");
    delay(500);
  }

}
//------------------write data from web page--------------------------------------------------/
String setup_json(String _data, String my_file) {
  File fileToWrite = SPIFFS.open(my_file, "w");
  if (fileToWrite.print(_data)) {
  };
  fileToWrite.close();
  if (my_file == "/mqtt.json") {
    read_file(my_file, "ip", "user", "topic", "password");
  }
  read_file(my_file, "ssid", "password", "", "");
};
//------------------read data from SPIFFS------------------------------------------------------/
void read_file(String new_file, String w, String x, String y, String z) {
  File fileToRead = SPIFFS.open(new_file);

  StaticJsonDocument<192> doc;

  DeserializationError error = deserializeJson(doc, fileToRead);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }
  fileToRead.close();
  if (y != "") {
    ip = doc[w]; // "192.16.15.46:8945"
    user = doc[x]; // "craig"
    topic = doc[y]; // "home/garage/"
    mqtt_password = doc[z]; // "mqtt"
  } else {
    ssid = doc[w];
    password = doc[x];

    startWifi();
  }

};
//---------------------setting up wifi-----------------------------------------------------------/
void startWifi() {
  debugln("ssid & password:");
  debug(ssid);
  debugln(password);
  if (WiFi.status() != WL_CONNECTED) {
    debugln("call set_station_mode");
    set_station_mode();
  } else {
    debugln("call set_ap_mode");
    set_ap_mode();


  }
};

//---------------------start wifi ap mode--------------------------------------------------------/
void set_ap_mode() {
  Serial.println("Switching to AP mode");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_mode_ssid);
  debugln(AP_mode_ssid);
  debugln(AP_mode_password);
  WiFi.softAPConfig(AP_ip, AP_gate, AP_netmask);
  debugln();
  Serial.print("IP address: ");
  debugln(WiFi.softAPIP());

}
//----------------------start wifi station mode(connect to home network)-------------------------/

void set_station_mode() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to wifi");

    delay(500);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < staTimeOut) {
      Serial.print(".");
      delay(500);
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    debugln();
    debugln("Connected");
    debugln(WiFi.localIP());
    WiFi.softAPdisconnect(true);
    return;
  }
  else if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_AP);
    debugln();
    debugln("Failed to connect.");
    debugln("set station function---WiFi.SOftAPIP=");
    Serial.print(WiFi.softAPIP());
    debugln();
    debug("AP_ip:");
    debugln(AP_ip);
    if (WiFi.softAPIP() != AP_ip) {
      debugln("going back to ap_setup");
      set_ap_mode();
    }
  }

}
