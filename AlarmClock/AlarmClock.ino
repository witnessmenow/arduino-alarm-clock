#include <NTPClient.h>
// change next line to use with another board/shield
#include <ESP8266WiFi.h>
//#include <WiFi.h> // for WiFi shield
//#include <WiFi101.h> // for WiFi 101 shield or MKR1000
#include <WiFiUdp.h>

#include <TM1637Display.h>

#include "secret.h"

const char *ssid     = "<SSID>";
const char *password = "<PASSWORD>";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

#include <Timezone.h>    // https://github.com/JChristensen/Timezone Modified!

// For Webserver
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

// For WifiManager
#include <DNSServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>
// Available on the library manager (ArduinoJson)
// https://github.com/bblanchon/ArduinoJson

#include <GoogleMapsDirectionsApi.h>

#include <WiFiClientSecure.h>

bool enableTrafficAdjust = false;

//Free Google Maps Api only allows for 2500 "elements" a day, so carful you dont go over
unsigned long api_mtbs = 60000; //mean time between api requests
unsigned long api_due_time = 0;
bool firstTime = true;

String origin = "40.8359838,-73.8734402";
String destination = "40.8536064,-73.9667197";
String waypoints = ""; //You need to include the via: before your waypoint

//Optional
DirectionsInputOptions inputOptions;

// For storing configurations
#include "FS.h"

// Module connection pins (Digital Pins)
#define CLK D6
#define DIO D5

#define ALARM D1

#define BUTTON D2

TM1637Display display(CLK, DIO);

const uint8_t LETTER_A = SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G;
const uint8_t LETTER_B = SEG_C | SEG_D | SEG_E | SEG_F | SEG_G;
const uint8_t LETTER_C = SEG_A | SEG_D | SEG_E | SEG_F;
const uint8_t LETTER_D = SEG_B | SEG_C | SEG_D | SEG_E | SEG_G;
const uint8_t LETTER_E = SEG_A | SEG_D | SEG_E | SEG_F | SEG_G;
const uint8_t LETTER_F = SEG_A | SEG_E | SEG_F | SEG_G;

const uint8_t LETTER_O = SEG_C | SEG_D | SEG_E | SEG_G;

const uint8_t SEG_CONF[] = {
  LETTER_C,                                        // C
  LETTER_O,                                        // o
  SEG_C | SEG_E | SEG_G,                           // n
  LETTER_F                                         // F
  };

const uint8_t SEG_BOOT[] = {
  LETTER_B,                                        // b
  LETTER_O,                                        // o
  LETTER_O,                                        // o
  SEG_D | SEG_E | SEG_F | SEG_G                    // t - kinda
};

ESP8266WebServer server(80);

const char *webpage =
#include "alarmWeb.h"
  ;

void handleRoot() {

  server.send(200, "text/html", webpage);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

//MAPS_API_KEY

int alarmHour = 0;
int alarmMinute = 0;
bool alarmActive = false;
bool alarmHandled = false;
bool buttonPressed = false;

int trafficOffset = 0;

WiFiClientSecure client;
GoogleMapsDirectionsApi api(MAPS_API_KEY, client);

// From World clock example in timezone library
// United Kingdom (London, Belfast)
TimeChangeRule BST = {"BST", Last, Sun, Mar, 1, 60};        // British Summer Time
TimeChangeRule GMT = {"GMT", Last, Sun, Oct, 2, 0};         // Standard Time
Timezone UK(BST, GMT);

TimeChangeRule aEDT = {"AEDT", First, Sun, Oct, 2, 660};    // UTC + 11 hours
TimeChangeRule aEST = {"AEST", First, Sun, Apr, 3, 600};    // UTC + 10 hours
Timezone ausET(aEDT, aEST);

// US Eastern Time Zone (New York, Detroit)
TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, -240};  // Eastern Daylight Time = UTC - 4 hours
TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -300};   // Eastern Standard Time = UTC - 5 hours
Timezone usET(usEDT, usEST);

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());

  display.setSegments(SEG_CONF);
}

void setup() {
  Serial.begin(115200);

  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount FS");
    return;
  }

  loadConfig();

  display.setBrightness(0xff);
  display.setSegments(SEG_BOOT);

  pinMode(ALARM, OUTPUT);
  digitalWrite(ALARM, LOW);

  pinMode(BUTTON, INPUT_PULLUP);

  attachInterrupt(BUTTON, interuptButton, RISING);

  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.autoConnect("AlarmClock", "password");

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");

  IPAddress ipAddress = WiFi.localIP();
  Serial.println(ipAddress);

  display.showNumberDec(ipAddress[3], false);

  delay(1000);



  if (MDNS.begin("alarm")) {
    Serial.println("MDNS Responder Started");
  }

  server.on("/", handleRoot);
  server.on("/setAlarm", handleSetAlarm);



  timeClient.begin();

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP Server Started");

  //These are all optional (although departureTime needed for traffic)
  inputOptions.departureTime = "now"; //can also be a future timestamp
  inputOptions.trafficModel = "best_guess"; //Defaults to this anyways
  inputOptions.avoid = "ferries";
  inputOptions.units = "metric";

}

bool loadConfig() {
  File configFile = SPIFFS.open("/alarm.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  configFile.readBytes(buf.get(), size);

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    Serial.println("Failed to parse config file");
    return false;
  }

  alarmHour = json["alarmHour"];
  alarmMinute = json["alarmMinute"];
  alarmActive = json["alarmActive"];
  return true;
}

bool saveConfig() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["alarmHour"] = alarmHour;
  json["alarmMinute"] = alarmMinute;
  json["alarmActive"] = alarmActive;

  File configFile = SPIFFS.open("/alarm.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  json.printTo(configFile);
  return true;
}

void handleSetAlarm() {

  Serial.println("Setting Alarm");
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "alarm") {
      String alarm = server.arg(i);
      int indexOfColon = alarm.indexOf(":");
      alarmHour = alarm.substring(0, indexOfColon).toInt();
      alarmMinute = alarm.substring(indexOfColon + 1).toInt();
      alarmActive = true;
      saveConfig();
      Serial.print("Setting Alarm to: ");
      Serial.print(alarmHour);
      Serial.print(":");
      Serial.print(alarmMinute);
    }
  }
  server.send(200, "text/html", "Set Alarm");
}

void soundAlarm() {
  digitalWrite(ALARM, HIGH);
  delay(1000);
  digitalWrite(ALARM, LOW);
}

bool dotsOn = false;

unsigned long oneSecondLoopDue = 0;

void checkGoogleMaps() {
  Serial.println("Getting traffic for " + origin + " to " + destination);
  DirectionsResponse response = api.directionsApi(origin, destination, inputOptions);
  if (response.duration_value == 0) {
    delay(100);
    response = api.directionsApi(origin, destination, inputOptions);
  }
  Serial.println("Response:");
  Serial.print("Trafic from ");
  Serial.print(response.start_address);
  Serial.print(" to ");
  Serial.println(response.end_address);

  Serial.print("Duration in Traffic text: ");
  Serial.println(response.durationTraffic_text);
  Serial.print("Duration in Traffic in Seconds: ");
  Serial.println(response.durationTraffic_value);

  Serial.print("Normal duration text: ");
  Serial.println(response.duration_text);
  Serial.print("Normal duration in Seconds: ");
  Serial.println(response.duration_value);

  Serial.print("Distance text: ");
  Serial.println(response.distance_text);
  Serial.print("Distance in meters: ");
  Serial.println(response.distance_value);

  trafficOffset = (response.durationTraffic_value - response.duration_value) / 60 ;

  Serial.print("Traffic Offset: ");
  Serial.println(trafficOffset);
}

void loop() {
  unsigned long now = millis();
  if (now > oneSecondLoopDue) {
    timeClient.update();
    displayTime(dotsOn);
    dotsOn = !dotsOn;
    checkForAlarm();
    if (buttonPressed) {
      alarmHandled = true;
      buttonPressed = false;
    }
    oneSecondLoopDue = now + 1000;
  }

  if (enableTrafficAdjust)
  {
    if ((now > api_due_time))  {
      inputOptions.waypoints = waypoints;
      checkGoogleMaps();
      api_due_time = now + api_mtbs;
    }
  }

  server.handleClient();
}

int timeHour;
int timeMinutes;

int lastEffectiveAlarm = 0;

//bool checkForAlarm()
//{
//  int effectiveAlarmMinute = alarmMinute;
//  int effectiveAlarmHour = alarmHour;
//  int actualAlarmMinutesFromMidnight = (alarmHour * 60) + alarmMinute;
//  int effectiveAlarmMinutesFromMidnight = actualAlarmMinutesFromMidnight;
//  if (trafficOffset != 0)
//  {
//    effectiveAlarmMinutesFromMidnight -= trafficOffset;
//
//    if (effectiveAlarmHour > 1439)
//    {
//      effectiveAlarmMinutesFromMidnight = effectiveAlarmMinutesFromMidnight % 1440;
//    }
//
//    if (effectiveAlarmHour < 0)
//    {
//      effectiveAlarmMinutesFromMidnight = (1440 + effectiveAlarmMinutesFromMidnight) % 1440;
//    }
//  }
//
//  int minutesSinceMidnight = (hour * 60) + minutes;
//
//  if (alarmActive) {
//    if (minutesSinceMidnight >= effectiveAlarmMinutesFromMidnight) {
//      if (minutesSinceMidnight <= actualAlarmMinutesFromMidnight + 30) {
//        if (!alarmHandled)
//        {
//          soundAlarm();
//        }
//      }
//    }
//
//  } else if (minutesSinceMidnight = 0) {
//    alarmHandled = false;
//  }
//
//  lastEffectiveAlarm = effectiveAlarmMinutesFromMidnight;
//}

bool checkForAlarm()
{
  if (alarmActive && timeHour == alarmHour && timeMinutes == alarmMinute) {
    if (!alarmHandled)
    {
      soundAlarm();
    }
  } else {
    alarmHandled = false;
  }
}

void interuptButton()
{
  // Serial.println("interuptButton");
  buttonPressed = true;
  return;
}

void displayTime(bool dotsVisible) {

  unsigned long epoch = UK.toLocal(timeClient.getEpochTime());

  timeHour = (epoch  % 86400L) / 3600;
  timeMinutes = (epoch % 3600) / 60;

  uint8_t data[4];

  if (timeHour < 10) {
    data[0] = display.encodeDigit(0);
    data[1] = display.encodeDigit(timeHour);
  } else {
    data[0] = display.encodeDigit(timeHour / 10);
    data[1] = display.encodeDigit(timeHour % 10);
  }

  if (dotsVisible) {
    // Turn on double dots
    data[1] = data[1] | B10000000;
  }

  if (timeMinutes < 10) {
    data[2] = display.encodeDigit(0);
    data[3] = display.encodeDigit(timeMinutes);
  } else {
    data[2] = display.encodeDigit(timeMinutes / 10);
    data[3] = display.encodeDigit(timeMinutes % 10);
  }

  display.setSegments(data);
}
