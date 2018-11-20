/*******************************************************************
 *  Use the getPrintJob() function to get the current print job on 
 *  your 3D printer being managed by OctoPrint. Output the current 
 *  progress of the job on a strip (or circle!) of 'Neopixel' LEDs
 *  such as WS2812B. Uses the FastLED library to drive pixels.
 * 
 *  You will need the IP or hostname of your OctoPrint server, a
 *  port number (will be 80 unless you are reaching it from an
 *  external source) and an API key from the OctoPrint 
 *  installation - http://docs.octoprint.org/en/master/api/general.html#authorization
 *  You will also need to enable CORS - http://docs.octoprint.org/en/master/api/general.html#cross-origin-requests
 *                                                                 
 *  OctoprintApi by Stephen Ludgate https://www.youtube.com/channel/UCVEEuAouZ6ua4oetLjjHAuw 
 * 
 *  Example based on original work written by A. Fichtner - https://github.com/anfichtn
 *******************************************************************/
 
#include <OctoPrintAPI.h> //This is where the magic happens... shazam!
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#define FASTLED_ESP8266_RAW_PIN_ORDER
#define FASTLED_INTERRUPT_RETRY_COUNT 0
#include <FastLED.h>                // Uses FastLED Library to drive WS2812B 'Neopixel' LEDs

#define LED_PIN D4                  // The pin connected to Data In on the strip
#define NUM_LEDS 110                // Number of LED's on your strip
#define LED_BRIGHTNESS 150          // set Brightness value
#define MILLI_AMPS 2400             // Maximum current your power source can deliver in mA
#define CONNECTION_COLOR 0xFF00FF   // Purple WiFi connection status color
CRGB leds[NUM_LEDS];

const char* ssid = "SSID";          // your network SSID (name)
const char* password = "PASSWORD";  // your network password
WiFiClient client;

// You only need to set one of the of follwowing:
IPAddress ip(192, 168, 123, 123);                         // Your IP address of your OctoPrint server (inernal or external)
//char* octoprint_host = "octopi.example.com";  // Or your hostname. Comment out one or the other.
const int octoprint_httpPort = 80;  //If you are connecting through a router this will work, but you need a random port forwarded to the OctoPrint server from your router. Enter that port here if you are external
String octoprint_apikey = "API_KEY"; //See top of file or GIT Readme about getting API key

unsigned long api_mtbs = 10000; //mean time between api requests (10 seconds)
unsigned long api_lasttime = 0;   //last time api request has been done
byte connection_retry = 0;
byte point = 0;

long printed_timeout = 3600000; //60 mins in milliseconds - timeout after printing completed, to clear strip
long printed_timeout_timer = printed_timeout;

// Use one of the following:
OctoprintApi api(client, ip, octoprint_httpPort, octoprint_apikey);               //If using IP address
//OctoprintApi api(client, octoprint_host, octoprint_httpPort, octoprint_apikey); //If using hostname. Comment out one or the other.

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
          
          //Set at least one pixel on when the job starts, as the strip may look 'off' for some time.
          if(progress<1){
            progress = 1;
          }
          
          fill_solid(leds, progress, CRGB::Green);
          delay(100);
          FastLED.show();

          printed_timeout_timer = 0;
        }
        else if (api.printJob.progressCompletion == 100 && api.printJob.printerState == "Operational" && printed_timeout_timer < printed_timeout)
        {
          // 100% complete is no longer "Printing" but "Operational"
          FastLED.clear();
          Serial.print("Progress:\t");
          Serial.print(api.printJob.progressCompletion);
          Serial.println(" %");
          
          int progress = int(NUM_LEDS); //FULL LEDs
        
          fill_solid(leds, progress, CRGB::Green);
          delay(100);
          FastLED.show();

          printed_timeout_timer+= api_mtbs;
        }
        else if (api.printJob.printerState == "Offline" || api.printJob.printerState == "Operational" && printed_timeout_timer >= printed_timeout)
        {
          // we are without working printer.... or the printer is no longer printing.... lights off
          fill_solid(leds, NUM_LEDS, CRGB::Black);
          delay(100);
          FastLED.show();
        }
      }
      api_lasttime = millis();  //Set api_lasttime to current milliseconds run
    }
  }
}
