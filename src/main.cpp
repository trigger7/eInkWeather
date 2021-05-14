#include "settings.h"

#include <Arduino.h>
#include <Wire.h>

#include <TimeLib.h>

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

#include "graphs.h"

GxEPD2_3C<GxEPD2_270c, GxEPD2_270c::HEIGHT> display(GxEPD2_270c(/*CS=*/ 5, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));

WiFiMulti wifiMulti;

#define T0 273.15
#define MS_TO_KMH 3.6

tmElements_t current_date;
const char* current_location;
float current_temperature;
float current_feels_like;
unsigned int current_pressure;
unsigned int current_humidity;
float current_wind;


String get_http_response(const char* type)
{
  HTTPClient http;
  String payload;

  Serial.println("[HTTP] begin...");
  String url = "http://api.openweathermap.org/data/2.5/forecast?q=" LOCATION "&appid=" API_KEY;
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
      Serial.println("GET done");
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();

  return payload;
}

void parse_json(String response, DynamicJsonDocument *doc)
{
  // Parse JSON object
  DeserializationError error = deserializeJson(*doc, response);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
  }
}

void _print_parameter(const char* parameter, int line)
{
  display.setFont(&FreeMono9pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(0, line);
  //strcat(parameter, " : ");
  display.print(parameter);
}

void _print_unit(const char* unit, int line)
{
  display.setFont(&FreeMono9pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(190, line);
  display.print(unit);
}

void display_parameter_float(const char* parameter, float value, const char* unit, int line)
{
  _print_parameter(parameter, line);

  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_RED);
  display.setCursor(140, line);
  display.printf("%4.1f", value);

  _print_unit(unit, line);
}

void display_parameter_int(const char* parameter, int value, const char* unit, int line)
{
  _print_parameter(parameter, line);

  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_RED);
  display.setCursor(140, line);
  display.printf("%4d", value);

  _print_unit(unit, line);
}

void show_current_weather(const void* pv)
{
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(0, 10);
  display.setFont(&FreeMonoBold9pt7b);
  display.print(current_location);
  display.setCursor(0, 40);
  display.setFont(&FreeMono9pt7b);
  display.printf("%02d/%02d/%d  %02d:%02d", current_date.Day,
                                            current_date.Month,
                                            1970 + current_date.Year,
                                            current_date.Hour,
                                            current_date.Minute);

  display_parameter_float("Temperature", current_temperature, "'C", 80);
  display_parameter_float("Ressentie", current_feels_like, "'C", 100);
  display_parameter_int("Pression", current_pressure, "hPa", 120);
  display_parameter_int("Humidite", current_humidity, "%", 140);
  display_parameter_float("Vent", current_wind, "km/h", 160);
}

void display_weather(DynamicJsonDocument *doc)
{
  Serial.println("read and convert");
  current_location = (*doc)["name"].as<const char*>();
  unsigned long timestamp = (*doc)["dt"].as<unsigned long>();
  breakTime(timestamp + 3600, current_date);
  current_temperature = (*doc)["main"]["temp"].as<float>() - T0;
  current_pressure = (*doc)["main"]["pressure"].as<unsigned int>();
  current_humidity = (*doc)["main"]["humidity"].as<unsigned int>();
  current_wind = (*doc)["wind"]["speed"].as<float>() * MS_TO_KMH;
  Serial.println("display");
  display.setRotation(1);
  display.setFullWindow();
  display.drawPaged(show_current_weather, 0);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) continue;

  delay(1000);
  
  display.init(115200);
  
  delay(4000);

  for (const WifiNetwork network : networks) wifiMulti.addAP(network.ssid, network.password);

  while(wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Waiting 10 seconds...");
    delay(10000);
  }

  //String response = get_http_response("weather");
  //DynamicJsonDocument doc(2000);
  //parse_json(response, &doc);
  //display_weather(&doc);

  String response = get_http_response("forecast");
  DynamicJsonDocument doc(30000);
  parse_json(response, &doc);
  //Graph graph = Graph(&display, "T", "'C", "P", "hPa");
  Graph graph = Graph(&display, "T", "'C", "RR", "mm");

  for (int i = 0; i < 25; i++) {
    graph.push_a(doc["list"][i]["dt"].as<unsigned long>(), doc["list"][i]["main"]["temp"].as<float>() - T0);
    //graph.push_b(doc["list"][i]["dt"].as<unsigned long>(), doc["list"][i]["main"]["pressure"].as<int>());
    graph.push_b(doc["list"][i]["dt"].as<unsigned long>(), doc["list"][i]["rain"]["3h"].as<float>());
  }
  Serial.println("push done");

  display.setRotation(1);
  display.setFullWindow();
  Serial.println("width height");
  Serial.println(display.width());
  Serial.println(display.height());
  /*
  float min_value, max_value;
  int step = graph.find_min_max_value(min_value, max_value);
  Serial.println("min max");
  Serial.println(step);
  Serial.println(min_value);
  Serial.println(max_value);
  Serial.println(graph.screen_y(min_value));
  Serial.println(graph.screen_y(max_value));
*/
  // auto it = graph._map_a.begin();
  // Serial.println("data");
  // while (it != graph._map_a.end())
  // {
  //   TimeElements t;
  //   breakTime(it->first, t);
  //   Serial.println(t.Hour);
  //   Serial.println(t.Hour % 6 == 0);
  //   it++;
  // }
//   Serial.print("X");
//   unsigned long t = graph._map_a.begin()->first;
//   Serial.println(t);
//   // Serial.println(graph._map_a.end()->first);
//   // unsigned long timestamp_min = graph._map_a.begin()->first;
//   // unsigned long a = graph._map_a.end()->first - timestamp_min;
//   // unsigned long b = graph._map_a.end()->first - timestamp_min;
//   // unsigned long c = a / b;
// //  c * (display.width() - graph._left_margin) + graph._left_margin;
//   // Serial.println(graph.screenX(t));
//   // Serial.println(graph.screenX(graph._map_a.end()->first));

//   Serial.print("Y");
//   float min_value, max_value;
//   graph.find_min_max_value(min_value, max_value);
//   Serial.println(graph.screenY(min_value));
//   Serial.println(graph.screenY(max_value));


  display.firstPage();
  do
  {
    graph.draw();
  }
  while (display.nextPage());

  // Doze off (forever);
  //display.hibernate();
  //esp_deep_sleep_start();
}

void loop() {

}
