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

  timeClient.begin();

  soundAlarm();

}

void soundAlarm(){
  digitalWrite(ALARM, HIGH);
  delay(1000);
  digitalWrite(ALARM, LOW);
}

bool dotsOn = false;

void loop() {
  delay(1000);
  timeClient.update();
  displayTime(dotsOn);
  dotsOn = !dotsOn;

}

void displayTime(bool dotsVisible) {
  unsigned long epoch = timeClient.getEpochTime();
  int hour = (epoch  % 86400L) / 3600;
  int minutes = (epoch % 3600) / 60;

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
