/* ___       _        ____       _       _      _    ____ ___ 
  / _ \  ___| |_ ___ |  _ \ _ __(_)_ __ | |_   / \  |  _ \_ _|
 | | | |/ __| __/ _ \| |_) | '__| | '_ \| __| / _ \ | |_) | | 
 | |_| | (__| || (_) |  __/| |  | | | | | |_ / ___ \|  __/| | 
  \___/ \___|\__\___/|_|   |_|  |_|_| |_|\__/_/   \_\_|  |___|
.......By Stephen Ludgate https://www.chunkymedia.co.uk.......

*/

#ifndef OctoprintApi_h
#define OctoprintApi_h

#include <Arduino.h>

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

#define OPAPI_TIMEOUT 1500
#define USER_AGENT "OctoPrintAPI/1.0 (Arduino)"

struct printerStatistics{
  String printerState;
  bool printerStateclosedOrError;
  bool printerStateerror;
  bool printerStateoperational;
  bool printerStatepaused;
  bool printerStatePrinting;
  bool printerStateready;
  bool printerStatesdReady;
};

struct octoprintVersion{
  String octoprintApi;
  String octoprintServer;
};

struct printJobCall{

  String printerState;
  long estimatedPrintTime;

  long jobFileDate;
  String jobFileName;
  String jobFileOrigin;
  long jobFileSize;
  
  float progressCompletion;
  long progressFilepos;
  long progressPrintTime;
  long progressPrintTimeLeft;
};

class OctoprintApi
{
  public:
    OctoprintApi (String octoPrintHost, int octoPrintPort, String apiKey);
    String sendGetToOctoprint(String command);
    String getOctoprintEndpointResults(String command);
    bool getPrinterStatistics();
    bool getOctoprintVersion();
    printerStatistics printerStats;
    octoprintVersion octoprintVer;
    bool getPrintJob();
    printJobCall printJob;

  private:
    String _apiKey;
    String _octoPrintHost;
    int _octoPrintPort;
    const int maxMessageLength = 1000;
};

#endif