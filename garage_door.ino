#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "WebSocketsServer.h"
#include "SPIFFS.h"
#include "ArduinoJson.h"
#include "ESPmDNS.h"
#include "Arduino.h"
//------------setting constants for servers-----------------------------------------------------/
const char* ssid = "spoon";
const char* password = "hyy5-8pc7-8cwq";
const char* ip = "";
const char* user = ""; 
const char* topic = "";
const char* mqtt_password = "";
//----------------------------------------------------------------------------------------------/

AsyncWebServer server(80);
WebSocketsServer websockets(81);

void setup() {

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //----------------starts MDNS server--------------------------------------------------/
  if (MDNS.begin("garage")) { //esp.local/
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
  /* server.on("/mqtt.json", HTTP_GET, [](AsyncWebServerRequest * request) {
     request->send(SPIFFS, "/mqtt.json", "application/json");
    });
    server.on("/wifi.json", HTTP_GET, [](AsyncWebServerRequest * request) {
     request->send(SPIFFS, "/wifi.json", "application/json");
    });*/
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

};

void loop() {};
//------------------write data from web page--------------------------------------------------/
String setup_json(String _data, String my_file) {
  File fileToWrite = SPIFFS.open(my_file, "w");
  if (fileToWrite.print(_data)) {
  };
  fileToWrite.close();
  if(my_file == "/mqtt.json"){
  read_file(my_file, "ip", "user", "topic", "password");
  }
  read_file(my_file, "ssid","password","","");
};
//------------------read data from SPIFFS------------------------------------------------------/
void read_file(String new_file,String w, String x,String y,String z) {
  File fileToRead = SPIFFS.open(new_file);

StaticJsonDocument<192> doc;

DeserializationError error = deserializeJson(doc, fileToRead);

if (error) {
  Serial.print("deserializeJson() failed: ");
  Serial.println(error.c_str());
  return;
}
if (y!=""){
  ip = doc[w]; // "192.16.15.46:8945"
  user = doc[x]; // "craig"
  topic = doc[y]; // "home/garage/"
  mqtt_password = doc[z]; // "mqtt"
  }else{
    ssid = doc[w];
    password = doc[x];
  }

fileToRead.close();
};
