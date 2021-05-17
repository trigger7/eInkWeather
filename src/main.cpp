#include "settings.h"
#include "graphs.h"

#include <Arduino.h>
#include <Wire.h>

#include <TimeLib.h>
#include <Button2.h>
#include <EEPROM.h>

#include <gfxfont.h>
#include <Adafruit_GFX.h>

#include <GxEPD2_GFX.h>
#include <GxEPD2_3C.h>
#include <GxEPD2.h>
#include <GxEPD2_EPD.h>

#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>

#include <ArduinoJson.h>

#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>

GxEPD2_3C<GxEPD2_270c, GxEPD2_270c::HEIGHT> display(GxEPD2_270c(/*CS=*/ 5, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));

WiFiMulti wifiMulti;

uint8_t location_index = 0;
Location location = locations[location_index];

uint8_t mode = 0;

Button2 mode_button = Button2(37);
Button2 location_button = Button2(38);
Button2 button = Button2(39);

#define T0 273.15
#define MS_TO_KMH 3.6


String get_http_response(String type, String location)
{
  HTTPClient http;
  String payload;

  Serial.println("[HTTP] begin...");
  String url = "http://api.openweathermap.org/data/2.5/" + type + "?q=" + location + "&appid=" API_KEY;
  http.begin(url); //HTTP

  Serial.println("[HTTP] GET...");
  int httpCode = http.GET();

  // httpCode will be negative on error
  if(httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    // file found at server
    if(httpCode == HTTP_CODE_OK) {
      payload = http.getString();
      //Serial.println(payload);
      //Serial.println("GET done");
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();

  return payload;
}

void parse_json(String& response, DynamicJsonDocument& doc)
{
  // Parse JSON object
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
  }
}

void push_data(Graph& graph, DynamicJsonDocument& doc, uint8_t mode) {
  switch(mode)
  {
    case 0:
      for (int i = 0; i < 25; i++) {
        graph.push_a(doc["list"][i]["dt"].as<unsigned long>(), doc["list"][i]["main"]["temp"].as<float>() - T0);
        graph.push_b(doc["list"][i]["dt"].as<unsigned long>(), doc["list"][i]["rain"]["3h"].as<float>());
      }
      graph.set_parameter_a("T", "'C", line);
      graph.set_parameter_b("RR", "mm", bar);
      break;
    case 1:
      for (int i = 0; i < 25; i++) {
        graph.push_a(doc["list"][i]["dt"].as<unsigned long>(), doc["list"][i]["main"]["humidity"].as<int>());
        graph.push_b(doc["list"][i]["dt"].as<unsigned long>(), doc["list"][i]["main"]["pressure"].as<int>());
      }
      graph.set_parameter_a("HU", "%", line);
      graph.set_parameter_b("P", "hPa", line);
      break;
    case 2:
      for (int i = 0; i < 25; i++) {
        graph.push_a(doc["list"][i]["dt"].as<unsigned long>(), doc["list"][i]["wind"]["gust"].as<float>() * MS_TO_KMH);
        graph.push_b(doc["list"][i]["dt"].as<unsigned long>(), doc["list"][i]["wind"]["speed"].as<float>() * MS_TO_KMH);
      }
      graph.set_parameter_a("GU", "km/h", bar);
      graph.set_parameter_b("FF", "km/h", bar);
  }
}

void draw_graph() {
  String response = get_http_response("forecast", location.name);
  DynamicJsonDocument doc(30000);
  parse_json(response, doc);

  Graph graph = Graph(&display, location.title);

  push_data(graph, doc, mode);
  Serial.println("push done");

  display.setRotation(1);
  display.setFullWindow();

  display.firstPage();
  do
  {
    graph.draw();
  }
  while (display.nextPage());
}

void location_handler(Button2& btn) {
    location_index++;
    location_index = location_index % 3;
    EEPROM.write(0, location_index);
    EEPROM.commit();
    location = locations[location_index];
    draw_graph();
}

void mode_handler(Button2& btn) {
    mode++;
    mode = mode % 3;
    EEPROM.write(1, mode);
    EEPROM.commit();
    draw_graph();
}

void setup() {
  Serial.begin(115200);
  while (!Serial) continue;

  EEPROM.begin(2);

  delay(1000);
  display.init(115200);
  
  location_button.setClickHandler(location_handler);
  location_index = EEPROM.read(0);
  location = locations[location_index];

  mode_button.setClickHandler(mode_handler);
  mode = EEPROM.read(1);

  delay(4000);

  for (const WifiNetwork network : networks) wifiMulti.addAP(network.ssid, network.password);

  while(wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Waiting 10 seconds...");
    delay(10000);
  }

  draw_graph();

  // Doze off (forever);
  //display.hibernate();
  //esp_deep_sleep_start();
}

void loop() {
  location_button.loop();
  mode_button.loop();
}
