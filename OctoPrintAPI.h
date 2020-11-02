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
#include <Client.h>

#define OPAPI_TIMEOUT       3000
#define POSTDATA_SIZE       256
#define POSTDATA_GCODE_SIZE 50
#define JSONDOCUMENT_SIZE   1024
#define USER_AGENT          "OctoPrintAPI/1.1.5 (Arduino)"

struct printerStatistics {
  String printerState;
  bool printerStateclosedOrError;
  bool printerStateerror;
  bool printerStatefinishing;
  bool printerStateoperational;
  bool printerStatepaused;
  bool printerStatepausing;
  bool printerStatePrinting;
  bool printerStateready;
  bool printerStateresuming;
  bool printerStatesdReady;

  float printerBedTempActual;
  float printerBedTempTarget;
  float printerBedTempOffset;
  bool printerBedAvailable;

  float printerTool0TempActual;
  float printerTool0TempTarget;
  float printerTool0TempOffset;
  bool printerTool0Available;

  float printerTool1TempTarget;
  float printerTool1TempActual;
  float printerTool1TempOffset;
  bool printerTool1Available;
};

struct octoprintVersion {
  String octoprintApi;
  String octoprintServer;
};

struct printJobCall {
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
  String progressprintTimeLeftOrigin;

  long jobFilamentTool0Length;
  float jobFilamentTool0Volume;
  long jobFilamentTool1Length;
  float jobFilamentTool1Volume;
};

struct printerBedCall {
  float printerBedTempActual;
  float printerBedTempOffset;
  float printerBedTempTarget;
  long printerBedTempHistoryTimestamp;
  float printerBedTempHistoryActual;
};

class OctoprintApi {
 public:
  OctoprintApi(Client &client, IPAddress octoPrintIp, int octoPrintPort, String apiKey);
  OctoprintApi(Client &client, char *octoPrintUrl, int octoPrintPort, String apiKey);
  String sendGetToOctoprint(String command);
  String getOctoprintEndpointResults(String command);
  bool getPrinterStatistics();
  bool getOctoprintVersion();
  printerStatistics printerStats;
  octoprintVersion octoprintVer;
  bool getPrintJob();
  printJobCall printJob;
  bool _debug          = false;
  int httpStatusCode   = 0;
  String httpErrorBody = "";
  String sendPostToOctoPrint(String command, const char *postData);
  bool octoPrintConnectionDisconnect();
  bool octoPrintConnectionAutoConnect();
  bool octoPrintConnectionFakeAck();
  bool octoPrintPrintHeadHome();
  bool octoPrintPrintHeadRelativeJog(double x, double y, double z, double f);
  bool octoPrintExtrude(double amount);
  bool octoPrintSetBedTemperature(uint16_t t);
  bool octoPrintSetTool0Temperature(uint16_t t);
  bool octoPrintSetTool1Temperature(uint16_t t);

  bool octoPrintGetPrinterSD();
  bool octoPrintPrinterSDInit();
  bool octoPrintPrinterSDRefresh();
  bool octoPrintPrinterSDRelease();

  bool octoPrintGetPrinterBed();
  printerBedCall printerBed;

  bool octoPrintJobStart();
  bool octoPrintJobCancel();
  bool octoPrintJobRestart();
  bool octoPrintJobPauseResume();
  bool octoPrintJobPause();
  bool octoPrintJobResume();
  bool octoPrintFileSelect(String &path);

  bool octoPrintCoreShutdown();
  bool octoPrintCoreReboot();
  bool octoPrintCoreRestart();

  bool octoPrintPrinterCommand(char *gcodeCommand);

 private:
  Client *_client;
  String _apiKey;
  IPAddress _octoPrintIp;
  bool _usingIpAddress;
  char *_octoPrintUrl;
  int _octoPrintPort;
  const int maxMessageLength = 1000;
  void closeClient();
  int extractHttpCode(String statusCode, String body);
  String sendRequestToOctoprint(String type, String command, const char *data);
};

#endif
