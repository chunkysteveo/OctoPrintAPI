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

#define OPAPI_TIMEOUT     3000  // 3s timetout
#define OPAPI_RUN_TIMEOUT (millis() - start_waiting < OPAPI_TIMEOUT) && (start_waiting <= millis())

#define POSTDATA_SIZE     128
#define TEMPDATA_SIZE     24
#define JSONDOCUMENT_SIZE 1024
#define USER_AGENT        "OctoPrintAPI/1.1.4 (Arduino)"
#define USER_AGENT_SIZE   64

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
  OctoprintApi(Client &client, IPAddress octoPrintIp, uint16_t octoPrintPort, String apiKey);
  OctoprintApi(Client &client, char *octoPrintUrl, uint16_t octoPrintPort, String apiKey);
  String sendGetToOctoprint(String command);
  String getOctoprintEndpointResults(String command);
  bool getPrinterStatistics();
  bool getOctoprintVersion();
  printerStatistics printerStats;
  octoprintVersion octoprintVer;
  bool getPrintJob();
  printJobCall printJob;
  bool _debug             = false;
  uint16_t httpStatusCode = 0;

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
  bool octoPrintSetTemperatures(uint16_t tool0 = 0, uint16_t tool1 = 0, uint16_t bed = 0);
  bool octoPrintCoolDown() { return octoPrintSetTemperatures(); };

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

  bool octoPrintPrinterCommand(const char *gcodeCommand);

 private:
  Client *_client;
  String _apiKey;
  IPAddress _octoPrintIp;
  bool _usingIpAddress;
  char *_octoPrintUrl;
  uint16_t _octoPrintPort;
  char _useragent[USER_AGENT_SIZE];

  void closeClient();
  void sendHeader(const String type, const String command, const char *data);
  int extractHttpCode(const String statusCode);
  String sendRequestToOctoprint(const String type, const String command, const char *data);
};

#endif
