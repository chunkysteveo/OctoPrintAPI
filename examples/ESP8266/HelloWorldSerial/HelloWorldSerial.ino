/*******************************************************************
 *  A "Hello World" sketch to make sure your Arduino and your 
 *  OctoPrint server can speak to each other. This will show your 
 *  OctoPrint Server, API and 3D printer statistics from the 
 *  OctoPrint API.
 * 
 *  You will need the IP or hostname of your OctoPrint server, a
 *  port number (will be 80 unless you are reaching it from an
 *  external source) and an API key from the OctoPrint 
 *  installation - http://docs.octoprint.org/en/master/api/general.html#authorization                              *
 *  You will also need to enable CORS - http://docs.octoprint.org/en/master/api/general.html#cross-origin-requests
 *                                                                 *
 *  By Stephen Ludgate https://www.youtube.com/channel/UCVEEuAouZ6ua4oetLjjHAuw 
 *******************************************************************/


#include <OctoPrintAPI.h> //This is where the magic happens...

#include <ESP8266WiFi.h>  
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
 
const char* ssid = "ENTER_YOUR_WIFI_SSID";
const char* password = "ENTER_YOUR_WIFI_PASSWORD";

const String octoprint_host = "ENTER_YOUR_SERVER_IP_OR_HOSTNAME_E.G._192.168.1.2_ETC";
const int octoprint_httpPort = 80;  //If you are connecting through a router this will work, but you need a random port forwarded to the OctoPrint server from your router. Enter that port here if you are external
String octoprint_apikey = "SEE_ABOVE_GET_YOUR_API_KEY";

String printerOperational;
String printerPaused;
String printerPrinting;
String printerReady;
String printerText;
String printerHotend;
String printerTarget;
String payload;


//WiFiClient client;
//HTTPClient http;
OctoprintApi api(octoprint_host, octoprint_httpPort, octoprint_apikey);

unsigned long api_mtbs = 60000; //mean time between api requests
unsigned long api_lasttime = 0;   //last time api request has been done

void setup () { 
  Serial.begin(115200);
  delay(10);

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  //if you get here you have connected to the WiFi
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); 
}


void loop() {

  if (millis() - api_lasttime > api_mtbs || api_lasttime==0) {
  
    if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status     
      if(api.getOctoprintVersion()){
        Serial.println("---------Version---------");
        Serial.print("Octoprint API: ");
        Serial.println(api.octoprintVer.octoprintApi);
        Serial.print("Octoprint Server: ");
        Serial.println(api.octoprintVer.octoprintServer);
        Serial.println("------------------------");
      }
      Serial.println();
      if(api.getPrinterStatistics()){
            Serial.println("---------Stats---------");
            Serial.print("Printer Current State: ");
            Serial.println(api.printerStats.printerState);
             Serial.print("Printer State - closedOrError:  ");
            Serial.println(api.printerStats.printerStateclosedOrError);
             Serial.print("Printer State - error:  ");
            Serial.println(api.printerStats.printerStateerror);
             Serial.print("Printer State - operational:  ");
            Serial.println(api.printerStats.printerStateoperational);
             Serial.print("Printer State - paused:  ");
            Serial.println(api.printerStats.printerStatepaused);
            Serial.print("Printer State - printing:  ");
            Serial.println(api.printerStats.printerStatePrinting);
             Serial.print("Printer State - ready:  ");
            Serial.println(api.printerStats.printerStateready);
             Serial.print("Printer State - sdReady:  ");
            Serial.println(api.printerStats.printerStatesdReady);
            Serial.println("------------------------");              
        }
    }
    api_lasttime = millis();
  }
}