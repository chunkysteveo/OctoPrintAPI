/* ___       _        ____       _       _      _    ____ ___ 
  / _ \  ___| |_ ___ |  _ \ _ __(_)_ __ | |_   / \  |  _ \_ _|
 | | | |/ __| __/ _ \| |_) | '__| | '_ \| __| / _ \ | |_) | | 
 | |_| | (__| || (_) |  __/| |  | | | | | |_ / ___ \|  __/| | 
  \___/ \___|\__\___/|_|   |_|  |_|_| |_|\__/_/   \_\_|  |___|
.......By Stephen Ludgate https://www.chunkymedia.co.uk.......

*/

#include "Arduino.h"
#include "OctoprintApi.h"

OctoprintApi::OctoprintApi(String octoPrintHost, int octoPrintPort, String apiKey)  {
  _apiKey = apiKey;
  _octoPrintHost = octoPrintHost;
  _octoPrintPort = octoPrintPort;
}

String OctoprintApi::sendGetToOctoprint(String command) {
//  Serial.println("OctoprintApi::sendGetToOctoprint() CALLED");
  String headers="";
  String body="";
  String payload="";
  bool finishedHeaders = false;
  bool currentLineIsBlank = true;
  unsigned long now;
  bool avail;
  
  HTTPClient http;

  http.begin("http://" + String(_octoPrintHost) + ":" + String(_octoPrintPort) + command);
//  http.addHeader("Content-Type", "application/json");  
//  http.addHeader("GET " + command + " HTTP/1.1");
//  http.addHeader("Host", OPAPI_HOST);
  http.addHeader("X-Api-Key", _apiKey);
  http.setUserAgent(USER_AGENT);
  
      int httpCode = http.GET();       
//      Serial.println(httpCode);

        // httpCode will be negative on error
  if(httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
       Serial.println("[OctoPrint] GET... code: " + String(httpCode) );

      if(httpCode == HTTP_CODE_CONFLICT) {
        payload = http.getString();
        if (String(payload) == "Printer is not operational") {
          payload = "Printer is not operational";
          Serial.println(payload);
        }
      }

      else if(httpCode == HTTP_CODE_OK) {
        payload = http.getString();

        
      }
      else {
        payload = http.getString();
        Serial.println( "[OctoPrint] HTTP_CODE_" + String(httpCode) + " : " + String(payload) );
      }
  } else {
      Serial.println("[OctoPrint] GET... failed, error: " + String(http.errorToString(httpCode).c_str()) );
  }
  http.end();

  return payload;
}

bool OctoprintApi::getPrinterStatistics(){
//  Serial.println("OctoprintApi::getPrinterStatistics() CALLED");
  String command="/api/printer";
//  Serial.print("command: ");
//  Serial.println(command);
  String response = sendGetToOctoprint(command);       //recieve reply from OctoPrint
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(response);
  if(root.success()) {
    if (root.containsKey("state")) {
      String printerState = root["state"]["text"];
      bool printerStateclosedOrError = root["state"]["flags"]["closedOrError"];
      bool printerStateerror = root["state"]["flags"]["error"];
      bool printerStateoperational = root["state"]["flags"]["operational"];
      bool printerStatepaused = root["state"]["flags"]["paused"];
      bool printerStatePrinting = root["state"]["flags"]["printing"];
      bool printerStateready = root["state"]["flags"]["ready"];
      bool printerStatesdReady = root["state"]["flags"]["sdReady"];
      
      printerStats.printerState = printerState;
      printerStats.printerStateclosedOrError = printerStateclosedOrError;
      printerStats.printerStateerror = printerStateerror;
      printerStats.printerStateoperational = printerStateoperational;
      printerStats.printerStatepaused = printerStatepaused;
      printerStats.printerStatePrinting = printerStatePrinting;
      printerStats.printerStateready = printerStateready;
      printerStats.printerStatesdReady = printerStatesdReady;
     
      return true;
    }
  }
  else{
      if(response == "Printer is not operational"){
        String printerState = response;
        printerStats.printerState = printerState;
        return true;
      }
    }

  return false;
}

bool OctoprintApi::getOctoprintVersion(){
//  Serial.println("OctoprintApi::getOctoprintVersion() CALLED");
  String command="/api/version";
//  Serial.print("command: ");
//  Serial.println(command);
  String response = sendGetToOctoprint(command);
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(response);
  if(root.success()) {
    if (root.containsKey("api")) {
      String octoprintApi = root["api"];
      String octoprintServer = root["server"];
      
      octoprintVer.octoprintApi = octoprintApi;
      octoprintVer.octoprintServer = octoprintServer;
      return true;
    }
  }
  return false;
}

bool OctoprintApi::getPrintJob(){
  // Serial.println("OctoprintApi::getPrintJob() CALLED");
  String command="/api/job";
//  Serial.print("command: ");
//  Serial.println(command);
  String response = sendGetToOctoprint(command);
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(response);
  if(root.success()) {
    String printerState = root["state"];
    printJob.printerState = printerState;   
    
    if (root.containsKey("job")) {
      long estimatedPrintTime  = root["job"]["estimatedPrintTime"];
      printJob.estimatedPrintTime = estimatedPrintTime;

      long jobFileDate = root["job"]["file"]["date"];
      String jobFileName = root["job"]["file"]["name"] | "";
      String jobFileOrigin = root["job"]["file"]["origin"] | "";
      long jobFileSize = root["job"]["file"]["size"];
      
      printJob.jobFileDate = jobFileDate;
      printJob.jobFileName = jobFileName;
      printJob.jobFileOrigin = jobFileOrigin;
      printJob.jobFileSize = jobFileSize;
    }
    if (root.containsKey("progress")) {
      float progressCompletion = root["progress"]["completion"] | 0.0;//isnan(root["progress"]["completion"]) ? 0.0 : root["progress"]["completion"];
      long progressFilepos = root["progress"]["filepos"];
      long progressPrintTime = root["progress"]["printTime"];
      long progressPrintTimeLeft = root["progress"]["printTimeLeft"];
      
      printJob.progressCompletion = progressCompletion;
      printJob.progressFilepos = progressFilepos;
      printJob.progressPrintTime = progressPrintTime;
      printJob.progressPrintTimeLeft = progressPrintTimeLeft;
    }
    return true;
  }
  return false;
}

String OctoprintApi::getOctoprintEndpointResults(String command) {
  Serial.println("OctoprintApi::getOctoprintEndpointResults() CALLED");
  String headers="";
  String body="";
  String payload="";
  bool finishedHeaders = false;
  bool currentLineIsBlank = true;
  unsigned long now;
  bool avail;
  HTTPClient http;
  http.begin("http://" + String(_octoPrintHost) + ":" + String(_octoPrintPort) + "/api/"+command); //HTTP

//  http.addHeader("Content-Type", "application/json");  
//  http.addHeader("GET " + command + " HTTP/1.1");
//  http.addHeader("Host", OPAPI_HOST);
  http.addHeader("X-Api-Key", _apiKey);
  http.setUserAgent(USER_AGENT);
  
      int httpCode = http.GET();       
      // Serial.println(httpCode);

        // httpCode will be negative on error
  if(httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
       Serial.println("[OctoPrint] GET... code: " + String(httpCode) );

      if(httpCode == HTTP_CODE_CONFLICT) {
        payload = http.getString();
        if (String(payload) == "Printer is not operational") {
          payload = "Printer is not operational";
          Serial.println(payload);
        }
      }

      else if(httpCode == HTTP_CODE_OK) {
        payload = http.getString();        
      }
      else {
        payload = http.getString();
        Serial.println( "[OctoPrint] HTTP_CODE_" + String(httpCode) + " : " + String(payload) );
      }
  } else {
      Serial.println("[OctoPrint] GET... failed, error: " + String(http.errorToString(httpCode).c_str()) );
  }
  http.end();
  return payload;
}