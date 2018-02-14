/* ___       _        ____       _       _      _    ____ ___
  / _ \  ___| |_ ___ |  _ \ _ __(_)_ __ | |_   / \  |  _ \_ _|
 | | | |/ __| __/ _ \| |_) | '__| | '_ \| __| / _ \ | |_) | |
 | |_| | (__| || (_) |  __/| |  | | | | | |_ / ___ \|  __/| |
  \___/ \___|\__\___/|_|   |_|  |_|_| |_|\__/_/   \_\_|  |___|
.......By Stephen Ludgate https://www.chunkymedia.co.uk.......

*/

#include "Arduino.h"
#include "OctoprintApi.h"

OctoprintApi::OctoprintApi(Client& client, IPAddress octoPrintIp, int octoPrintPort, String apiKey)  {
  _client = &client;
  _apiKey = apiKey;
  _octoPrintIp = octoPrintIp;
  _octoPrintPort = octoPrintPort;
  _usingIpAddress = true;
}

OctoprintApi::OctoprintApi(Client& client, char* octoPrintUrl, int octoPrintPort, String apiKey)  {
  _client = &client;
  _apiKey = apiKey;
  _octoPrintUrl = octoPrintUrl;
  _octoPrintPort = octoPrintPort;
  _usingIpAddress = false;
}

String OctoprintApi::sendGetToOctoprint(String command) {
//  Serial.println("OctoprintApi::sendGetToOctoprint() CALLED");
  String statusCode="";
  String headers="";
  String body="";
  bool finishedStatusCode = false;
  bool finishedHeaders = false;
  bool currentLineIsBlank = true;
  int ch_count = 0;
  unsigned long now;
  bool avail;

  bool connected;

  if(_usingIpAddress) {
    connected = _client->connect(_octoPrintIp, _octoPrintPort);
  } else {
    connected = _client->connect(_octoPrintUrl, _octoPrintPort);
  }

  if (connected) {
    if (_debug) Serial.println(".... connected to server");

    _client->println("GET " + command + " HTTP/1.1");
    _client->print("Host: ");
    if(_usingIpAddress) {
      _client->println(_octoPrintIp);
    } else {
      _client->println(_octoPrintUrl);
    }
    _client->print("X-Api-Key: "); _client->println(_apiKey);
    _client->println("User-Agent: arduino/1.0");
    _client->println();

    now = millis();
    while (millis() - now < 3000) {
      while (_client->available()) {
        char c = _client->read();

        if (_debug) Serial.print(c);

        if(!finishedStatusCode){
          if(c == '\n'){
            finishedStatusCode = true;
          } else {
            statusCode = statusCode + c;
          }
        }

        if(!finishedHeaders){
					if (currentLineIsBlank && c == '\n') {
						finishedHeaders = true;
					}
					else{
						headers = headers + c;

					}
				} else {
					if (ch_count < maxMessageLength)  {
						body=body+c;
						ch_count++;
					}
				}

				if (c == '\n') {
					currentLineIsBlank = true;
				}else if (c != '\r') {
					currentLineIsBlank = false;
				}
      }
    }
  }

  closeClient();
  int httpCode = extractHttpCode(statusCode);
  return body;
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
  if (_debug) Serial.println("OctoprintApi::getOctoprintEndpointResults() CALLED");

  return sendGetToOctoprint("/api/" + command);
}

void OctoprintApi::closeClient() {
  if(_client->connected()){
    _client->stop();
  }
}

int OctoprintApi::extractHttpCode(String statusCode) {
  if(_debug){
    Serial.print("Status Code: ");
    Serial.println(statusCode);
  }
  int firstSpace = statusCode.indexOf(" ");
  int lastSpace = statusCode.lastIndexOf(" ");
  if(firstSpace > -1 && lastSpace > -1 && firstSpace != lastSpace){
    statusCode.substring(firstSpace + 1, lastSpace);
  } else {
    return -1;
  }
}
