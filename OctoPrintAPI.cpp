/* ___       _        ____       _       _      _    ____ ___
  / _ \  ___| |_ ___ |  _ \ _ __(_)_ __ | |_   / \  |  _ \_ _|
 | | | |/ __| __/ _ \| |_) | '__| | '_ \| __| / _ \ | |_) | |
 | |_| | (__| || (_) |  __/| |  | | | | | |_ / ___ \|  __/| |
  \___/ \___|\__\___/|_|   |_|  |_|_| |_|\__/_/   \_\_|  |___|
.......By Stephen Ludgate https://www.chunkymedia.co.uk.......
*/

#include "OctoPrintAPI.h"

#include "Arduino.h"

/** OctoprintApi()
 * IP address version of the client connect function
 **/
OctoprintApi::OctoprintApi(Client &client, IPAddress octoPrintIp, uint16_t octoPrintPort, String apiKey) {
  _client         = &client;
  _apiKey         = apiKey;
  _octoPrintIp    = octoPrintIp;
  _octoPrintPort  = octoPrintPort;
  _usingIpAddress = true;
  snprintf(_useragent, USER_AGENT_SIZE, "User-Agent: %s", USER_AGENT);
}

/** OctoprintApi()
 * Hostname version of the client connect function
 **/
OctoprintApi::OctoprintApi(Client &client, char *octoPrintUrl, uint16_t octoPrintPort, String apiKey) {
  _client         = &client;
  _apiKey         = apiKey;
  _octoPrintUrl   = octoPrintUrl;
  _octoPrintPort  = octoPrintPort;
  _usingIpAddress = false;
  snprintf(_useragent, USER_AGENT_SIZE, "User-Agent: %s", USER_AGENT);
}

/* GET YOUR ASS TO OCTOPRINT...
 */
String OctoprintApi::sendRequestToOctoprint(String type, String command, const char *data) {
  if (_debug) Serial.println("OctoprintApi::sendRequestToOctoprint() CALLED");

  if ((type != "GET") && (type != "POST")) {
    if (_debug) Serial.println("OctoprintApi::sendRequestToOctoprint() Only GET & POST are supported... exiting.");
    return "";
  }

  String buffer               = "";
  char c                      = 0;
  uint32_t bodySize           = 0;
  unsigned long start_waiting = 0;

  if (_usingIpAddress)
    _client->connect(_octoPrintIp, _octoPrintPort);
  else
    _client->connect(_octoPrintUrl, _octoPrintPort);

  if (!_client->connected()) {
    if (_debug) Serial.println("connection failed.");
    closeClient();
    return "";
  }
  if (_debug) Serial.println("...connected to server.");

  sendHeader(type, command, data);

  if (_debug) Serial.println("Request sent. Waiting for the answer.");
  start_waiting = millis();
  while (_client->connected() && !_client->available() && OPAPI_RUN_TIMEOUT)  // wait for a reply
    delay(100);
  if (!_client->connected() || !_client->available()) {
    if (_debug) Serial.println("Timeout during waiting for a reply");
    closeClient();
    return "";
  }

  if (_debug) Serial.println("Reading status code");
  while (_client->connected() && _client->available() && (c = _client->read()) != '\n' && OPAPI_RUN_TIMEOUT)
    buffer = buffer + c;

  httpStatusCode = extractHttpCode(buffer);
  if (_debug) {
    Serial.print("HTTP code:");
    Serial.println(httpStatusCode);
  }
  if (!(200 <= httpStatusCode && httpStatusCode < 204) && httpStatusCode != 409) {  // Code 204 NO CONTENT is ok, we close and return
    closeClient();
    return "";
  }

  if (_debug) Serial.println("Reading headers");
  for (buffer = ""; _client->connected() && _client->available() && OPAPI_RUN_TIMEOUT;) {
    c      = _client->read();
    buffer = buffer + c;
    if (_debug) Serial.print(c);
    if (c == '\n') {
      if (buffer.startsWith("Content-Length: "))
        bodySize = buffer.substring(16).toInt();
      else if (buffer == "\n" || buffer == "\r\n")
        break;
      buffer = "";
    }
  }

  if (_debug) Serial.println("Reading body");
  for (buffer = ""; _client->connected() && _client->available() && bodySize-- && OPAPI_RUN_TIMEOUT;)
    buffer = buffer + (char)_client->read();
  if (_debug) Serial.println(buffer);

  closeClient();
  return buffer;
}

String OctoprintApi::sendGetToOctoprint(String command) {
  if (_debug) Serial.println("OctoprintApi::sendGetToOctoprint() CALLED");
  return sendRequestToOctoprint("GET", command, NULL);
}

/** getOctoprintVersion()
 * http://docs.octoprint.org/en/master/api/version.html#version-information
 * Retrieve information regarding server and API version. Returns a JSON object with two keys, api containing the API version, server containing the server version.
 * Status Codes:
 * 200 OK – No error
 * */
bool OctoprintApi::getOctoprintVersion() {
  String response = sendGetToOctoprint("/api/version");
  StaticJsonDocument<JSONDOCUMENT_SIZE> root;

  if (!deserializeJson(root, response) && root.containsKey("api")) {
    octoprintVer.octoprintApi    = (const char *)root["api"];
    octoprintVer.octoprintServer = (const char *)root["server"];
    return true;
  }
  return false;
}

/** getPrinterStatistics()
 * http://docs.octoprint.org/en/master/api/printer.html#retrieve-the-current-printer-state
 * Retrieves the current state of the printer.
 * Returns a 200 OK with a Full State Response in the body upon success.
 * */
bool OctoprintApi::getPrinterStatistics() {
  String response = sendGetToOctoprint("/api/printer");  //recieve reply from OctoPrint

  StaticJsonDocument<JSONDOCUMENT_SIZE> root;
  if (!deserializeJson(root, response)) {
    if (root.containsKey("state")) {
      printerStats.printerState              = (const char *)root["state"]["text"];
      printerStats.printerStateclosedOrError = root["state"]["flags"]["closedOrError"];
      printerStats.printerStateerror         = root["state"]["flags"]["error"];
      printerStats.printerStatefinishing     = root["state"]["flags"]["finishing"];
      printerStats.printerStateoperational   = root["state"]["flags"]["operational"];
      printerStats.printerStatepaused        = root["state"]["flags"]["paused"];
      printerStats.printerStatepausing       = root["state"]["flags"]["pausing"];
      printerStats.printerStatePrinting      = root["state"]["flags"]["printing"];
      printerStats.printerStateready         = root["state"]["flags"]["ready"];
      printerStats.printerStateresuming      = root["state"]["flags"]["resuming"];
      printerStats.printerStatesdReady       = root["state"]["flags"]["sdReady"];
    }

    printerStats.printerBedAvailable   = false;
    printerStats.printerTool0Available = false;
    printerStats.printerTool1Available = false;
    if (root.containsKey("temperature")) {
      if (root["temperature"].containsKey("bed")) {
        printerStats.printerBedTempActual = root["temperature"]["bed"]["actual"];
        printerStats.printerBedTempTarget = root["temperature"]["bed"]["target"];
        printerStats.printerBedTempOffset = root["temperature"]["bed"]["offset"];
        printerStats.printerBedAvailable  = true;
      }

      if (root["temperature"].containsKey("tool0")) {
        printerStats.printerTool0TempActual = root["temperature"]["tool0"]["actual"];
        printerStats.printerTool0TempTarget = root["temperature"]["tool0"]["target"];
        printerStats.printerTool0TempOffset = root["temperature"]["tool0"]["offset"];
        printerStats.printerTool0Available  = true;
      }

      if (root["temperature"].containsKey("tool1")) {
        printerStats.printerTool1TempActual = root["temperature"]["tool1"]["actual"];
        printerStats.printerTool1TempTarget = root["temperature"]["tool1"]["target"];
        printerStats.printerTool1TempOffset = root["temperature"]["tool1"]["offset"];
        printerStats.printerTool1Available  = true;
      }
    }
    return true;
  } else {
    printerStats.printerStateoperational = false;
    if (response == "Printer is not operational") {
      printerStats.printerState = response;
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
bool OctoprintApi::octoPrintJobStart() {
  sendPostToOctoPrint("/api/job", "{\"command\": \"start\"}");
  return (httpStatusCode == 204);
}

bool OctoprintApi::octoPrintJobCancel() {
  sendPostToOctoPrint("/api/job", "{\"command\": \"cancel\"}");
  return (httpStatusCode == 204);
}

bool OctoprintApi::octoPrintJobRestart() {
  sendPostToOctoPrint("/api/job", "{\"command\": \"restart\"}");
  return (httpStatusCode == 204);
}

bool OctoprintApi::octoPrintJobPauseResume() {
  sendPostToOctoPrint("/api/job", "{\"command\": \"pause\"}");
  return (httpStatusCode == 204);
}

bool OctoprintApi::octoPrintJobPause() {
  sendPostToOctoPrint("/api/job", "{\"command\": \"pause\", \"action\": \"pause\"}");
  return (httpStatusCode == 204);
}

bool OctoprintApi::octoPrintJobResume() {
  sendPostToOctoPrint("/api/job", "{\"command\": \"pause\", \"action\": \"resume\"}");
  return (httpStatusCode == 204);
}

bool OctoprintApi::octoPrintFileSelect(String &path) {
  sendPostToOctoPrint("/api/files/local" + path, "{\"command\": \"select\", \"print\": false }");
  return (httpStatusCode == 204);
}

//bool OctoprintApi::octoPrintJobPause(String actionCommand){}

/** getPrintJob
 * http://docs.octoprint.org/en/master/api/job.html#retrieve-information-about-the-current-job
 * Retrieve information about the current job (if there is one).
 * Returns a 200 OK with a Job information response in the body.
 * */
bool OctoprintApi::getPrintJob() {
  String response = sendGetToOctoprint("/api/job");

  StaticJsonDocument<JSONDOCUMENT_SIZE> root;
  if (!deserializeJson(root, response)) {
    printJob.printerState = (const char *)root["state"];

    if (root.containsKey("job")) {
      printJob.estimatedPrintTime = root["job"]["estimatedPrintTime"];

      printJob.jobFileDate   = root["job"]["file"]["date"];
      printJob.jobFileName   = (const char *)(root["job"]["file"]["name"] | "");
      printJob.jobFileOrigin = (const char *)(root["job"]["file"]["origin"] | "");
      printJob.jobFileSize   = root["job"]["file"]["size"] | 0;

      printJob.jobFilamentTool0Length = root["job"]["filament"]["tool0"]["length"] | 0;
      printJob.jobFilamentTool0Volume = root["job"]["filament"]["tool0"]["volume"] | 0.0;
      printJob.jobFilamentTool1Length = root["job"]["filament"]["tool1"]["length"] | 0;
      printJob.jobFilamentTool1Volume = root["job"]["filament"]["tool1"]["volume"] | 0.0;
    }
    if (root.containsKey("progress")) {
      printJob.progressCompletion          = root["progress"]["completion"] | 0.0;
      printJob.progressFilepos             = root["progress"]["filepos"] | 0;
      printJob.progressPrintTime           = root["progress"]["printTime"] | 0;
      printJob.progressPrintTimeLeft       = root["progress"]["printTimeLeft"] | 0;
      printJob.progressprintTimeLeftOrigin = (const char *)root["progress"]["printTimeLeftOrigin"];
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
 * **/
String OctoprintApi::sendPostToOctoPrint(String command, const char *postData) {
  if (_debug) Serial.println("OctoprintApi::sendPostToOctoPrint() CALLED");
  return sendRequestToOctoprint("POST", command, postData);
}

/***** CONNECTION HANDLING *****/
/**
 * http://docs.octoprint.org/en/master/api/connection.html#issue-a-connection-command
 * Issue a connection command. Currently available command are: connect, disconnect, fake_ack
 * Status Codes:
 * 204 No Content – No error
 * 400 Bad Request – If the selected port or baudrate for a connect command are not part of the available options.
 * */
bool OctoprintApi::octoPrintConnectionAutoConnect() {
  sendPostToOctoPrint("/api/connection", "{\"command\": \"connect\"}");
  return (httpStatusCode == 204);
}
bool OctoprintApi::octoPrintConnectionDisconnect() {
  sendPostToOctoPrint("/api/connection", "{\"command\": \"disconnect\"}");
  return (httpStatusCode == 204);
}
bool OctoprintApi::octoPrintConnectionFakeAck() {
  sendPostToOctoPrint("/api/connection", "{\"command\": \"fake_ack\"}");
  return (httpStatusCode == 204);
}

/***** PRINT HEAD *****/
/**
 * http://docs.octoprint.org/en/master/api/printer.html#issue-a-print-head-command
 * Print head commands allow jogging and homing the print head in all three axes.
 * Available commands are: jog, home, feedrate
 * All of these commands except feedrate may only be sent if the printer is currently operational and not printing. Otherwise a 409 Conflict is returned.
 * Upon success, a status code of 204 No Content and an empty body is returned.
 * */
bool OctoprintApi::octoPrintPrintHeadHome() {
  // {
  // "command": "home",
  // "axes": ["x", "y", "z"]
  // }
  sendPostToOctoPrint("/api/printer/printhead", "{\"command\": \"home\",\"axes\": [\"x\", \"y\"]}");
  return (httpStatusCode == 204);
}

bool OctoprintApi::octoPrintPrintHeadRelativeJog(double x, double y, double z, double f) {
  // {
  // "command": "jog",
  // "x": 10,
  // "y": -5,
  // "z": 0.02,
  // "absolute": false,
  // "speed": 30
  // }
  char postData[POSTDATA_SIZE];
  char tmp[TEMPDATA_SIZE];

  strncpy(postData, "{\"command\": \"jog\"", POSTDATA_SIZE);
  if (x != 0) {
    snprintf(tmp, TEMPDATA_SIZE, ", \"x\": %.2f", x);
    strncat(postData, tmp, TEMPDATA_SIZE);
  }
  if (y != 0) {
    snprintf(tmp, TEMPDATA_SIZE, ", \"y\": %.2f", y);
    strncat(postData, tmp, TEMPDATA_SIZE);
  }
  if (z != 0) {
    snprintf(tmp, TEMPDATA_SIZE, ", \"z\": %.2f", z);
    strncat(postData, tmp, TEMPDATA_SIZE);
  }
  if (f != 0) {
    snprintf(tmp, TEMPDATA_SIZE, ", \"speed\": %.2f", f);
    strncat(postData, tmp, TEMPDATA_SIZE);
  }
  strncat(postData, ", \"absolute\": false", TEMPDATA_SIZE);
  strncat(postData, " }", TEMPDATA_SIZE);

  sendPostToOctoPrint("/api/printer/printhead", postData);
  return (httpStatusCode == 204);
}

bool OctoprintApi::octoPrintExtrude(double amount) {
  char postData[POSTDATA_SIZE];
  snprintf(postData, POSTDATA_SIZE, "{ \"command\": \"extrude\", \"amount\": %.2f }", amount);

  sendPostToOctoPrint("/api/printer/tool", postData);
  return (httpStatusCode == 204);
}

bool OctoprintApi::octoPrintSetBedTemperature(uint16_t t) {
  char postData[POSTDATA_SIZE];
  snprintf(postData, POSTDATA_SIZE, "{ \"command\": \"target\", \"target\": %d }", t);

  sendPostToOctoPrint("/api/printer/bed", postData);
  return (httpStatusCode == 204);
}

bool OctoprintApi::octoPrintSetTool0Temperature(uint16_t t) {
  char postData[POSTDATA_SIZE];
  snprintf(postData, POSTDATA_SIZE, "{ \"command\": \"target\", \"targets\": { \"tool0\": %d } }", t);

  sendPostToOctoPrint("/api/printer/tool", postData);
  return (httpStatusCode == 204);
}

bool OctoprintApi::octoPrintSetTool1Temperature(uint16_t t) {
  char postData[POSTDATA_SIZE];
  snprintf(postData, POSTDATA_SIZE, "{ \"command\": \"target\", \"targets\": { \"tool1\": %d } }", t);

  sendPostToOctoPrint("/api/printer/tool", postData);
  return (httpStatusCode == 204);
}

bool OctoprintApi::octoPrintSetTemperatures(uint16_t tool0, uint16_t tool1, uint16_t bed) {
  char postData[POSTDATA_SIZE];
  snprintf(postData, POSTDATA_SIZE,
           "{ \"command\": \"target\", \"targets\": { \"tool0\": %d, \"tool1\": %d, \"bed\": %d } }",
           tool0, tool1, bed);

  sendPostToOctoPrint("/api/printer/tool", postData);
  return (httpStatusCode == 204);
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
bool OctoprintApi::octoPrintGetPrinterBed() {
  String response = sendGetToOctoprint("/api/printer/bed?history=true&limit=2");

  StaticJsonDocument<JSONDOCUMENT_SIZE> root;
  if (!deserializeJson(root, response)) {
    if (root.containsKey("bed")) {
      printerBed.printerBedTempActual = root["bed"]["actual"];
      printerBed.printerBedTempOffset = root["bed"]["offset"];
      printerBed.printerBedTempTarget = root["bed"]["target"];
    }
    if (root.containsKey("history")) {
      printerBed.printerBedTempHistoryTimestamp = root["history"][0]["time"];
      printerBed.printerBedTempHistoryActual    = root["history"][0]["bed"]["actual"];
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
bool OctoprintApi::octoPrintPrinterSDInit() {
  sendPostToOctoPrint("/api/printer/sd", "{\"command\": \"init\"}");
  return (httpStatusCode == 204);
}
bool OctoprintApi::octoPrintPrinterSDRefresh() {
  sendPostToOctoPrint("/api/printer/sd", "{\"command\": \"refresh\"}");
  return (httpStatusCode == 204);
}
bool OctoprintApi::octoPrintPrinterSDRelease() {
  sendPostToOctoPrint("/api/printer/sd", "{\"command\": \"release\"}");
  return (httpStatusCode == 204);
}

/*
http://docs.octoprint.org/en/master/api/printer.html#retrieve-the-current-sd-state
Retrieves the current state of the printer’s SD card.
If SD support has been disabled in OctoPrint’s settings, a 404 Not Found is returned.
Returns a 200 OK with an SD State Response in the body upon success.
*/
bool OctoprintApi::octoPrintGetPrinterSD() {
  String response = sendGetToOctoprint("/api/printer/sd");

  StaticJsonDocument<JSONDOCUMENT_SIZE> root;
  if (!deserializeJson(root, response)) {
    printerStats.printerStatesdReady = root["ready"];
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
bool OctoprintApi::octoPrintPrinterCommand(const char *gcodeCommand) {
  char postData[POSTDATA_SIZE];

  snprintf(postData, POSTDATA_SIZE, "{\"command\": \"%s\"}", gcodeCommand);
  sendPostToOctoPrint("/api/printer/command", postData);

  return (httpStatusCode == 204);
}

/***** GENERAL FUNCTIONS *****/
/**
 * Close the client
 * */
void OctoprintApi::closeClient() { _client->stop(); }

/**
 * Extract the HTTP header response code. Used for error reporting - will print in serial monitor any non 200 response codes (i.e. if something has gone wrong!).
 * Thanks Brian for the start of this function, and the chuckle of watching you realise on a live stream that I didn't use the response code at that time! :)
 * */
int OctoprintApi::extractHttpCode(const String statusCode) {  // HTTP/1.1 200 OK  || HTTP/1.1 400 BAD REQUEST
  if (_debug) {
    Serial.print("Status code to extract: ");
    Serial.println(statusCode);
  }

  String statusExtract = statusCode.substring(statusCode.indexOf(" ") + 1);               // 200 OK || 400 BAD REQUEST
  int statusCodeInt    = statusExtract.substring(0, statusExtract.indexOf(" ")).toInt();  // 200 || 400

  return statusCodeInt ? statusCodeInt : -1;
}

/** 
 * Send HTTP Headers
 * */
void OctoprintApi::sendHeader(const String type, const String command, const char *data) {
  _client->println(type + " " + command + " HTTP/1.1");
  _client->print("Host: ");
  if (_usingIpAddress)
    _client->println(_octoPrintIp);
  else
    _client->println(_octoPrintUrl);
  _client->print("X-Api-Key: ");
  _client->println(_apiKey);
  _client->println(_useragent);
  if (data != NULL) {
    _client->println("Content-Type: application/json");
    _client->print("Content-Length: ");
    _client->println(strlen(data));  // number of bytes in the payload
    _client->println();              // important need an empty line here
    _client->println(data);          // the payload
  } else
    _client->println();
}