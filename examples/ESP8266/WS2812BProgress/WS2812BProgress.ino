/*******************************************************************
 *  Use the getPrintJob() function to get the current print job on 
 *  your 3D printer being managed by OctoPrint.
 * 
 *  Example uses Time.h and TimeLib.h to work out some human readable 
 *  time values from the file timestamps taken from the API.
 * 
 *  You will need the IP or hostname of your OctoPrint server, a
 *  port number (will be 80 unless you are reaching it from an
 *  external source) and an API key from the OctoPrint 
 *  installation - http://docs.octoprint.org/en/master/api/general.html#authorization                              *
 *  You will also need to enable CORS - http://docs.octoprint.org/en/master/api/general.html#cross-origin-requests
 *                                                                 *
 *  OctoprintApi by Stephen Ludgate https://www.youtube.com/channel/UCVEEuAouZ6ua4oetLjjHAuw 
 *  Example written by A. Fichtner
 *******************************************************************/
 
#include <OctoPrintAPI.h> //This is where the magic happens... shazam!
#include <Time.h>         //We will need these two just to do some rough time math on the timestamps we get
#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#define FASTLED_ESP8266_RAW_PIN_ORDER
#define FASTLED_INTERRUPT_RETRY_COUNT 0
#include <FastLED.h>

#define LED_PIN D4
#define NUM_LEDS 110       // Number of LED's on your strip
#define LED_BRIGHTNESS 150 // set Brightness value
#define MILLI_AMPS 2400    // Maximum current your power source can deliver in mA
#define CONNECTION_COLOR 0xFF00FF
CRGB leds[NUM_LEDS];

const char* ssid = "SSID";          // your network SSID (name)
const char* password = "PASSWORD";  // your network password
WiFiClient client;

// You only need to set one of the of follwowing:
IPAddress ip(192, 168, 179, 4);                         // Your IP address of your OctoPrint server (inernal or external)
//char* octoprint_host = "octopi.example.com";  // Or your hostname. Comment out one or the other.
const int octoprint_httpPort = 80;  //If you are connecting through a router this will work, but you need a random port forwarded to the OctoPrint server from your router. Enter that port here if you are external
String octoprint_apikey = "API_KEY"; //See top of file or GIT Readme about getting API key

unsigned long api_mtbs = 10000; //mean time between api requests (60 seconds)
unsigned long api_lasttime = 0;   //last time api request has been done
byte connection_retry = 0;
byte point = 0;
// Use one of the following:
OctoprintApi api(client, ip, octoprint_httpPort, octoprint_apikey);               //If using IP address
//OctoprintApi api(client, octoprint_host, octoprint_httpPort, octoprint_apikey);//If using hostname. Comment out one or the other.

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setDither(false);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(LED_BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  delay(100);
  FastLED.show();
  fill_solid(leds, NUM_LEDS, CRGB::Red);
  delay(1000);
  FastLED.show();
  fill_solid(leds, NUM_LEDS, CRGB::Yellow);
  delay(1000);
  FastLED.show();
  fill_solid(leds, NUM_LEDS, CRGB::Green);
  delay(1000);
  FastLED.show();
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  // disconnect if previously connected
  WiFi.disconnect();
  delay(100);
  Serial.println("");
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    leds[connection_retry] = CONNECTION_COLOR;
    delay(100);
    FastLED.show();
    if (connection_retry == NUM_LEDS) {
      connection_retry = 0;
    } else {
      connection_retry++;
    }
  }
   //if you get here you have connected to the WiFi
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
 
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // put your main code here, to run repeatedly:
  if (millis() - api_lasttime > api_mtbs || api_lasttime == 0) { //Check if time has expired to go check OctoPrint
    if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
      if (api.getPrintJob()) { //Get the print job API endpoint
        if ((api.printJob.printerState == "Printing")) {
          FastLED.clear();
          // we are printing something....
          Serial.print("Progress:\t");
          Serial.print(api.printJob.progressCompletion);
          Serial.println(" %");
          int progress = int((NUM_LEDS * int(api.printJob.progressCompletion)) / 100);
          fill_solid(leds, progress, CRGB::Green);
          delay(10);
          FastLED.show();
        }
        else if (api.printJob.printerState == "Offline")
        {
          // we are without working printer.... lights off
          fill_solid(leds, NUM_LEDS, CRGB::Black);
          delay(10);
          FastLED.show();
        }
      }
      api_lasttime = millis();  //Set api_lasttime to current milliseconds run
    }
  }
}
