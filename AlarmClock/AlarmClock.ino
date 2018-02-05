#include <NTPClient.h>
// change next line to use with another board/shield
#include <ESP8266WiFi.h>
//#include <WiFi.h> // for WiFi shield
//#include <WiFi101.h> // for WiFi 101 shield or MKR1000
#include <WiFiUdp.h>

#include <TM1637Display.h>

const char *ssid     = "<SSID>";
const char *password = "<PASSWORD>";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// For Webserver
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

// Module connection pins (Digital Pins)
#define CLK D6
#define DIO D5

#define ALARM D1

TM1637Display display(CLK, DIO);

const uint8_t LETTER_A = SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G;
const uint8_t LETTER_B = SEG_C | SEG_D | SEG_E | SEG_F | SEG_G;
const uint8_t LETTER_C = SEG_A | SEG_D | SEG_E | SEG_F;
const uint8_t LETTER_D = SEG_B | SEG_C | SEG_D | SEG_E | SEG_G;
const uint8_t LETTER_E = SEG_A | SEG_D | SEG_E | SEG_F | SEG_G;
const uint8_t LETTER_F = SEG_A | SEG_E | SEG_F | SEG_G;

const uint8_t LETTER_O = SEG_C | SEG_D | SEG_E | SEG_G;

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

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

int alarmHour = 0;
int alarmMinute = 0;
bool alarmHandled = false;

void setup() {
  Serial.begin(115200);
  display.setBrightness(0xff);
  display.setSegments(SEG_BOOT);

  pinMode(ALARM, OUTPUT);
  digitalWrite(ALARM, LOW);

  // WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

    Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("alarm")) {
    Serial.println("MDNS Responder Started");
  }

  server.on("/", handleRoot);
  server.on("/setAlarm", handleSetAlarm);

  

  timeClient.begin();

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP Server Started");

}

void handleSetAlarm() {

  Serial.println("Setting Alarm");
  for (uint8_t i=0; i<server.args(); i++){
    if(server.argName(i) == "alarm") {
      String alarm = server.arg(i);
      int indexOfColon = alarm.indexOf(":");
      alarmHour = alarm.substring(0, indexOfColon).toInt();
      alarmMinute = alarm.substring(indexOfColon + 1).toInt();
      Serial.print("Setting Alarm to: ");
      Serial.print(alarmHour);
      Serial.print(":");
      Serial.print(alarmMinute);
    }
  }
  server.send(200, "text/html", "Set Alarm");
}

void soundAlarm(){
  digitalWrite(ALARM, HIGH);
  delay(1000);
  digitalWrite(ALARM, LOW);
}

bool dotsOn = false;

unsigned long oneSecondLoopDue = 0;

void loop() {
  unsigned long now = millis();
  if(now > oneSecondLoopDue){
    timeClient.update();
    displayTime(dotsOn);
    dotsOn = !dotsOn;
    checkForAlarm();
    oneSecondLoopDue = now + 1000;
  }
  
  server.handleClient();
}

int hour;
int minutes;

bool checkForAlarm()
{
  if(hour == alarmHour && minutes == alarmMinute){
    if(!alarmHandled)
    {
      soundAlarm();
      alarmHandled = true;
    }
  } else {
    alarmHandled = false;
  }
}

void displayTime(bool dotsVisible) {
  unsigned long epoch = timeClient.getEpochTime();
  hour = (epoch  % 86400L) / 3600;
  minutes = (epoch % 3600) / 60;

  uint8_t data[4];

  if(hour < 10) {
    data[0] = display.encodeDigit(0);
    data[1] = display.encodeDigit(hour);
  } else {
    data[0] = display.encodeDigit(hour /10);
    data[1] = display.encodeDigit(hour % 10);
  }

  if(dotsVisible){
    // Turn on double dots
  data[1] = data[1] | B10000000;
  }

  if(minutes < 10) {
    data[2] = display.encodeDigit(0);
    data[3] = display.encodeDigit(minutes);
  } else {
    data[2] = display.encodeDigit(minutes /10);
    data[3] = display.encodeDigit(minutes % 10);
  }

  display.setSegments(data);
}
