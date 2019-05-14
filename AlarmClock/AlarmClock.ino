/*******************************************************************
    BLough Alarm Clock
    An Alarm clock that gets it's time from the interent

    Features:
    - Web interface for setting the alarm
    - Captive portal for setting WiFi Details
    - Automatically adjust for DST

    By Brian Lough

    For use with: https://www.tindie.com/products/15402/
    
    YouTube: https://www.youtube.com/brianlough
    Tindie: https://www.twitch.tv/brianlough
    Twitter: https://twitter.com/witnessmenow
 *******************************************************************/

//Included with ESP8266 Arduino Core

#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include "FS.h"

#include <ezTime.h>
// Library used for getting the time and adjusting for DST
// Search for "ezTime" in the Arduino Library manager
// https://github.com/ropg/ezTime

#include <TM1637Display.h>
// Library used for controlling the 7 Segment display
// Search for "TM1637" in the Arduino Library manager
// https://github.com/avishorp/TM1637

#include <ArduinoJson.h>
// Library used for parsing & saving Json to config
// NOTE: There is a breaking change in the 6.x.x version,
// install the 5.x.x version instead
// Search for "Arduino Json" in the Arduino Library manager
// https://github.com/bblanchon/ArduinoJson

#include <WiFiManager.h>
// Library used for creating the captive portal for entering WiFi Details
// Search for "Wifimanager" in the Arduino Library manager
// https://github.com/tzapu/WiFiManager

#include "secret.h"
#include "pitches.h"
#include "displayConf.h"

bool firstTime = true;

#include "alarmWeb.h";

// --- TimeZone (Change me!) ---

// You should be able to use the country code for
// countries that only span one timezone, but I'm having
// issues with it:
// https://github.com/ropg/ezTime/issues/47
//#define MYTIMEZONE "ie"

// Or to set a specific timezone, use this list:
// https://en.wikipedia.org/wiki/List_of_tz_database_time_zones
#define MYTIMEZONE "Europe/Dublin"

// -----------------------------


// --- Pin Configuration ---

//Display Pins
#define CLK D6
#define DIO D5

// Buzzer Pin
#define ALARM D1

// Buttons Pins
#define BUTTON D2
#define SNOOZE_BUTTON D3

// LDR Pin
#define LDR A0

//On board LED
#define LED LED_BUILTIN

// ------------------------

TM1637Display display(CLK, DIO);

ESP8266WebServer server(80);

boolean dotsOn;

unsigned long oneSecondLoopDue = 0;

int brightnessAdjustInterval = 100; //mS
unsigned long adjustBrightnessDue = 0;

Timezone myTZ;

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
void handleADC() {
 int a = analogRead(A0);
 String adcValue = String(a);
 
 server.send(200, "text/plane", adcValue); //Send ADC value only to client ajax request
}

void handleWiFi(){
 int8_t rssi = getWifiQuality();
 String rssiValue = String(rssi);
 server.send(200, "text/plane", rssiValue);
}

void handleLED() {
 String ledState = "OFF";
 String t_state = server.arg("LEDstate"); //Refer  xhttp.open("GET", "setLED?LEDstate="+led, true);
 Serial.println(t_state);
 if(t_state == "1")
 {
  digitalWrite(LED,LOW); //LED ON
  ledState = "<i class=\"fas fa-lightbulb\"></i> ON"; //Feedback parameter
 }
 else if(t_state == "2")
 {
//  digitalRead(LED); //READ LED
  if(digitalRead(LED) == 1){
    ledState = "<i class=\"far fa-lightbulb\"></i> OFF"; //Feedback parameter  
  }
  else{
    ledState = "<i class=\"fas fa-lightbulb\"></i> ON";
  }
 }
 else{
  digitalWrite(LED,HIGH); //LED OFF
  ledState = "<i class=\"far fa-lightbulb\"></i> OFF"; //Feedback parameter  
 }
 
 server.send(200, "text/plane", ledState); //Send web page
}

int alarmHour = 0;
int alarmMinute = 0;
bool alarmActive = false;
bool alarmHandled = false;
bool buttonPressed = false;

void handleGetAlarm() {
  char alarmString[5];
  sprintf(alarmString, "%02d:%02d", alarmHour, alarmMinute);
  server.send(200, "text/plain", alarmString);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());

  display.setSegments(SEG_CONF);
}

void setup() {
  Serial.begin(115200);

    //Onboard LED port Direction output
  pinMode(LED,OUTPUT); 
  digitalWrite(LED,HIGH); //LED OFF

  display.setBrightness(7);
  display.setSegments(SEG_BOOT);
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount FS");
    display.setSegments(SEG_FILE);
    while(true){
      // loop as we want to show there is an error
      // as it possible couldn't load an alarm
    }
  } else {
    loadConfig(); 
  }

  pinMode(ALARM, OUTPUT);
  digitalWrite(ALARM, LOW);

  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(SNOOZE_BUTTON, INPUT_PULLUP);

  attachInterrupt(BUTTON, interuptButton, RISING);

  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.autoConnect("AlarmClock", "password");

  Serial.println("");
  Serial.print("WiFi Connected");
  Serial.println("");
  Serial.print("IP address: ");

  IPAddress ipAddress = WiFi.localIP();
  Serial.println(ipAddress);

  display.showNumberDec(ipAddress[3], false);

  delay(1000);



  if (MDNS.begin("alarm")) {
    Serial.println("MDNS Responder Started");
  }

  // HTTP Server

  server.on("/", handleRoot);
  server.on("/setAlarm", handleSetAlarm);
  server.on("/getAlarm", handleGetAlarm);
  server.on("/setLED", handleLED);
  server.on("/getWiFi", handleWiFi);
  server.on("/readADC", handleADC);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP Server Started");

  // EZ Time
  setDebug(INFO);
  waitForSync();

  Serial.println();
  Serial.println("UTC:             " + UTC.dateTime());

  myTZ.setLocation(F(MYTIMEZONE));
  Serial.print(F("Time in your set timezone:         "));
  Serial.println(myTZ.dateTime());

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
      Serial.println(alarmMinute);
    }
  }
  server.send(200, "text/html", "Set Alarm");
}

// notes in the melody:
int melody[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};

void soundAlarm() {
  for (int thisNote = 0; thisNote < 8; thisNote++) {

    // to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(ALARM, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(ALARM);
  }
}

void adjustBrightness() {
  int sensorValue = analogRead(LDR);
  if (sensorValue < 300) {
    sensorValue = 300;
  }
  else if (sensorValue > 900) {
    sensorValue = 900;
  }

  int level = map(sensorValue, 300, 900, 0, 7);
  //Serial.print("seg brightness ");
  //Serial.println(level);
  display.setBrightness(level);
}

void loop() {
  unsigned long now = millis();
  if(now > adjustBrightnessDue){
    adjustBrightness();
    adjustBrightnessDue = now + brightnessAdjustInterval;
  }
  
  if(digitalRead(SNOOZE_BUTTON) == LOW && digitalRead(BUTTON) == LOW){
    int sensorValue = analogRead(LDR);
    display.showNumberDec(sensorValue, false);
    oneSecondLoopDue = now;
  } else if ( digitalRead(SNOOZE_BUTTON) == LOW) {
    IPAddress ipAddress = WiFi.localIP();
    display.showNumberDec(ipAddress[3], false);
    oneSecondLoopDue = now;
  } else {
    if (now > oneSecondLoopDue) {
      displayTime(dotsOn);
      dotsOn = !dotsOn;
      checkForAlarm();
      if (buttonPressed) {
        alarmHandled = true;
        buttonPressed = false;
      }
      oneSecondLoopDue = now + 1000;
    }
  }

  server.handleClient();
}

int timeHour;
int timeMinutes;

int lastEffectiveAlarm = 0;

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

// converts the dBm to a range between 0 and 100%
int8_t getWifiQuality() {
  int32_t dbm = WiFi.RSSI();
  if (dbm <= -100) {
    return 0;
  } else if (dbm >= -50) {
    return 100;
  } else {
    return 2 * (dbm + 100);
  }
}

void displayTime(bool dotsVisible) {

  //Requesting the time in a specific format
  // H = hours with leading 0
  // i = minutes with leading 0
  // so 9:34am would come back "0934"
  String timeString = myTZ.dateTime("Hi");

  timeHour = timeString.substring(0,2).toInt();
  timeMinutes = timeString.substring(2).toInt();

  uint8_t data[4];

  data[0] = display.encodeDigit(timeString.substring(0,1).toInt());
  data[1] = display.encodeDigit(timeString.substring(1,2).toInt());


  if (dotsVisible) {
    // Turn on double dots
    data[1] = data[1] | B10000000;
  }

  data[2] = display.encodeDigit(timeString.substring(2,3).toInt());
  data[3] = display.encodeDigit(timeString.substring(3).toInt());

  display.setSegments(data);
}
