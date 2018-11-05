/* ___       _        ____       _       _      _    ____ ___
  / _ \  ___| |_ ___ |  _ \ _ __(_)_ __ | |_   / \  |  _ \_ _|
 | | | |/ __| __/ _ \| |_) | '__| | '_ \| __| / _ \ | |_) | |
 | |_| | (__| || (_) |  __/| |  | | | | | |_ / ___ \|  __/| |
  \___/ \___|\__\___/|_|   |_|  |_|_| |_|\__/_/   \_\_|  |___|
.......By Stephen Ludgate https://www.chunkymedia.co.uk.......

*/

#include "Arduino.h"
#include "OctoPrintAPI.h"
#include <ESP8266HTTPClient.h>

/** OctoprintApi()
 * IP address version of the client connect function
 * */
OctoprintApi::OctoprintApi(Client& client, IPAddress octoPrintIp, int octoPrintPort, String apiKey)  {
  _client = &client;
  _apiKey = apiKey;
  _octoPrintIp = octoPrintIp;
  _octoPrintPort = octoPrintPort;
  _usingIpAddress = true;
}


/** OctoprintApi()
 * Hostname version of the client connect function
 * */
OctoprintApi::OctoprintApi(Client& client, char* octoPrintUrl, int octoPrintPort, String apiKey)  {
  _client = &client;
  _apiKey = apiKey;
  _octoPrintUrl = octoPrintUrl;
  _octoPrintPort = octoPrintPort;
  _usingIpAddress = false;
}


/** GET YOUR ASS TO OCTOPRINT... 
 * 
 * **/
String OctoprintApi::sendGetToOctoprint(String command) {
  if (_debug) Serial.println("OctoprintApi::sendGetToOctoprint() CALLED");

  HTTPClient http;
  String url = "http://";
  if (_usingIpAddress) {
    url = url + _octoPrintIp.toString();
  } else {
    url = url + _octoPrintUrl;
  }
  url = url + ":" + _octoPrintPort + command;

  int result = -1;
  String body = "";

  if (http.begin(url)) {
    http.addHeader("X-Api-Key", _apiKey);
    result = http.GET();
    body = http.getString();
  }
  http.end();

  httpStatusCode = result;
  return body;
}

/** getOctoprintVersion()
 * http://docs.octoprint.org/en/master/api/version.html#version-information
 * Retrieve information regarding server and API version. Returns a JSON object with two keys, api containing the API version, server containing the server version.
 * Status Codes:	
 * 200 OK – No error
 * */
bool OctoprintApi::getOctoprintVersion(){
  String command="/api/version";
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

/** getPrinterStatistics()
 * http://docs.octoprint.org/en/master/api/printer.html#retrieve-the-current-printer-state
 * Retrieves the current state of the printer.
 * Returns a 200 OK with a Full State Response in the body upon success.
 * */
bool OctoprintApi::getPrinterStatistics(){
  String command="/api/printer";
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

      
    }
    if(root.containsKey("temperature")){
      float printerBedTempActual = root["temperature"]["bed"]["actual"];
      float printerTool0TempActual = root["temperature"]["tool0"]["actual"];
      float printerTool1TempActual = root["temperature"]["tool1"]["actual"];
      printerStats.printerBedTempActual = printerBedTempActual;
      printerStats.printerTool0TempActual = printerTool0TempActual;
      printerStats.printerTool1TempActual = printerTool1TempActual;
    }
    return true;
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

/***** PRINT JOB OPPERATIONS *****/
/**
 * http://docs.octoprint.org/en/devel/api/job.html#issue-a-job-command
 * Job commands allow starting, pausing and cancelling print jobs. 
 * Available commands are: start, cancel, restart, pause - Accepts one optional additional parameter action specifying
 * which action to take - pause, resume, toggle
 * If no print job is active (either paused or printing), a 409 Conflict will be returned.
 * Upon success, a status code of 204 No Content and an empty body is returned.
 * */
bool OctoprintApi::octoPrintJobStart(){
  String command = "/api/job";
  char* postData = "{\"command\": \"start\"}";
  String response = sendPostToOctoPrint(command,postData);
  if(httpStatusCode == 204) return true;
  return false;
}

bool OctoprintApi::octoPrintJobCancel(){
  String command = "/api/job";
  char* postData = "{\"command\": \"cancel\"}";
  String response = sendPostToOctoPrint(command,postData);
  if(httpStatusCode == 204) return true;
  return false;
}

bool OctoprintApi::octoPrintJobRestart(){
  String command = "/api/job";
  char* postData = "{\"command\": \"restart\"}";
  String response = sendPostToOctoPrint(command,postData);
  if(httpStatusCode == 204) return true;
  return false;
}

bool OctoprintApi::octoPrintJobPauseResume(){
  String command = "/api/job";
  char* postData = "{\"command\": \"pause\"}";
  String response = sendPostToOctoPrint(command,postData);
  if(httpStatusCode == 204) return true;
  return false;
}
//bool OctoprintApi::octoPrintJobPause(String actionCommand){}

/** getPrintJob
 * http://docs.octoprint.org/en/master/api/job.html#retrieve-information-about-the-current-job
 * Retrieve information about the current job (if there is one).
 * Returns a 200 OK with a Job information response in the body.
 * */
bool OctoprintApi::getPrintJob(){
  String command="/api/job";
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

/** getOctoprintEndpointResults()
 * General function to get any GET endpoint of the API and return body as a string for you to format or view as you wish.
 * */
String OctoprintApi::getOctoprintEndpointResults(String command) {
  if (_debug) Serial.println("OctoprintApi::getOctoprintEndpointResults() CALLED");
  return sendGetToOctoprint("/api/" + command);
}


/** POST TIME 
 * 
 * **/
String OctoprintApi::sendPostToOctoPrint(String command, char* postData) {
  if (_debug) Serial.println("OctoprintApi::sendPostToOctoPrint() CALLED");

  HTTPClient http;
  String url = "http://";
  if (_usingIpAddress) {
    url = url + _octoPrintIp.toString();
  } else {
    url = url + _octoPrintUrl;
  }
  url = url + ":" + _octoPrintPort + command;

  int result = -1;
  String body = "";

  if (http.begin(url)) {
    http.addHeader("X-Api-Key", _apiKey);
    result = http.POST((uint8_t*)postData, strlen(postData));
    body = http.getString();
  }
  http.end();

  httpStatusCode = result;
  return body;
}


/***** CONNECTION HANDLING *****/
/**
 * http://docs.octoprint.org/en/master/api/connection.html#issue-a-connection-command
 * Issue a connection command. Currently available command are: connect, disconnect, fake_ack
 * Status Codes: 
 * 204 No Content – No error
 * 400 Bad Request – If the selected port or baudrate for a connect command are not part of the available options.
 * */
bool OctoprintApi::octoPrintConnectionAutoConnect(){
  String command = "/api/connection";
  char* postData = "{\"command\": \"connect\"}";
  String response = sendPostToOctoPrint(command,postData);
  if(httpStatusCode == 204) return true;
  return false;
}
bool OctoprintApi::octoPrintConnectionDisconnect(){
  String command = "/api/connection";
  char* postData = "{\"command\": \"disconnect\"}";
  String response = sendPostToOctoPrint(command,postData);
  if(httpStatusCode == 204) return true;
  return false;
}
bool OctoprintApi::octoPrintConnectionFakeAck(){
  String command = "/api/connection";
  char* postData = "{\"command\": \"fake_ack\"}";
  String response = sendPostToOctoPrint(command,postData);
  if(httpStatusCode == 204) return true;
  return false;
}


/***** PRINT HEAD *****/
/**
 * http://docs.octoprint.org/en/master/api/printer.html#issue-a-print-head-command
 * Print head commands allow jogging and homing the print head in all three axes. 
 * Available commands are: jog, home, feedrate
 * All of these commands except feedrate may only be sent if the printer is currently operational and not printing. Otherwise a 409 Conflict is returned.
 * Upon success, a status code of 204 No Content and an empty body is returned.
 * */
bool OctoprintApi::octoPrintPrintHeadHome(){
  String command = "/api/printer/printhead";
  //   {
  //   "command": "home",
  //   "axes": ["x", "y", "z"]
  // }
  char* postData = "{\"command\": \"home\",\"axes\": [\"x\", \"y\"]}";
  String response = sendPostToOctoPrint(command,postData);
  if(httpStatusCode == 204) return true;
  return false;
}

/***** PRINT BED *****/
/** octoPrintGetPrinterBed()
 * http://docs.octoprint.org/en/master/api/printer.html#retrieve-the-current-bed-state
 * Retrieves the current temperature data (actual, target and offset) plus optionally a (limited) history (actual, target, timestamp) for the printer’s heated bed.
 * It’s also possible to retrieve the temperature history by supplying the history query parameter set to true. 
 * The amount of returned history data points can be limited using the limit query parameter.
 * Returns a 200 OK with a Temperature Response in the body upon success.
 * If no heated bed is configured for the currently selected printer profile, the resource will return an 409 Conflict.
 * */ 
bool OctoprintApi::octoPrintGetPrinterBed(){
  String command = "/api/printer/bed?history=true&limit=2";
  String response = sendGetToOctoprint(command);
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(response);
  if(root.success()) {
    if (root.containsKey("bed")) {
      float printerBedTempActual = root["bed"]["actual"];
      float printerBedTempOffset = root["bed"]["offset"];
      float printerBedTempTarget = root["bed"]["target"];
      printerBed.printerBedTempActual = printerBedTempActual;
      printerBed.printerBedTempOffset = printerBedTempOffset;
      printerBed.printerBedTempTarget = printerBedTempTarget;      
    }
    if (root.containsKey("history")) {
      JsonArray& history = root["history"];
        long historyTime = history[0]["time"];
        float historyPrinterBedTempActual = history[0]["bed"]["actual"];        
        printerBed.printerBedTempHistoryTimestamp = historyTime;
        printerBed.printerBedTempHistoryActual = historyPrinterBedTempActual;
    }  
    return true;
  }
  return false;
}


/***** SD FUNCTIONS *****/
/*
 * http://docs.octoprint.org/en/master/api/printer.html#issue-an-sd-command
 * SD commands allow initialization, refresh and release of the printer’s SD card (if available).
 * Available commands are: init, refresh, release
*/
bool OctoprintApi::octoPrintPrinterSDInit(){
  String command = "/api/printer/sd";
  char* postData = "{\"command\": \"init\"}";
  String response = sendPostToOctoPrint(command,postData);
  if(httpStatusCode == 204) return true;
  return false;
}
bool OctoprintApi::octoPrintPrinterSDRefresh(){
  String command = "/api/printer/sd";
  char* postData = "{\"command\": \"refresh\"}";
  String response = sendPostToOctoPrint(command,postData);
  if(httpStatusCode == 204) return true;
  return false;
}
bool OctoprintApi::octoPrintPrinterSDRelease(){
  String command = "/api/printer/sd";
  char* postData = "{\"command\": \"release\"}";
  String response = sendPostToOctoPrint(command,postData);
  if(httpStatusCode == 204) return true;
  return false;
}

/*
http://docs.octoprint.org/en/master/api/printer.html#retrieve-the-current-sd-state
Retrieves the current state of the printer’s SD card.
If SD support has been disabled in OctoPrint’s settings, a 404 Not Found is returned.
Returns a 200 OK with an SD State Response in the body upon success.
*/
bool OctoprintApi::octoPrintGetPrinterSD(){
  String command = "/api/printer/sd";
  String response = sendGetToOctoprint(command);
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(response);
  if(root.success()) {
      bool printerStatesdReady = root["ready"];
      printerStats.printerStatesdReady = printerStatesdReady;
    return true;
  }
  return false;
}


/***** COMMANDS *****/
/*
http://docs.octoprint.org/en/master/api/printer.html#send-an-arbitrary-command-to-the-printer
Sends any command to the printer via the serial interface. Should be used with some care as some commands can interfere with or even stop a running print job.
If successful returns a 204 No Content and an empty body.
*/
bool OctoprintApi::octoPrintPrinterCommand(char* gcodeCommand){
  String command = "/api/printer/command";
  char postData[50];

  postData[0] = '\0';
  sprintf(postData,"{\"command\": \"%s\"}",gcodeCommand);
  
  String response = sendPostToOctoPrint(command,postData);

  if(httpStatusCode == 204) return true;
  return false;
}


/***** GENERAL FUNCTIONS *****/

/**
 * Close the client
 * */
void OctoprintApi::closeClient() {
  if(_client->connected()){
    _client->stop();
  }
}

/**
 * Extract the HTTP header response code. Used for error reporting - will print in serial monitor any non 200 response codes (i.e. if something has gone wrong!).
 * Thanks Brian for the start of this function, and the chuckle of watching you realise on a live stream that I didn't use the response code at that time! :)
 * */
int OctoprintApi::extractHttpCode(String statusCode, String body) {
  if(_debug){
    Serial.print("\nStatus code to extract: ");
    Serial.println(statusCode);
  }
  int firstSpace = statusCode.indexOf(" ");
  int lastSpace = statusCode.lastIndexOf(" ");
  if(firstSpace > -1 && lastSpace > -1 && firstSpace != lastSpace){
    String statusCodeALL = statusCode.substring(firstSpace + 1);                //"400 BAD REQUEST"
    String statusCodeExtract = statusCode.substring(firstSpace + 1, lastSpace); //May end up being e.g. "400 BAD"
    int statusCodeInt = statusCodeExtract.toInt();                              //Converts to "400" integer - i.e. strips out rest of text characters "fix"
    if(statusCodeInt != 200 
    and statusCodeInt != 201
    and statusCodeInt != 202
    and statusCodeInt != 204){
      Serial.print("\nSERVER RESPONSE CODE: " + String(statusCodeALL));
      if(body!="") Serial.println(" - " + body);
      else Serial.println();
    }
    return statusCodeInt;
  } else {
    return -1;
  }
}
