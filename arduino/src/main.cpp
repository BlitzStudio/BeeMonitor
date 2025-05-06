#include <time.h>
#include <string.h>
#include <Stream.h>
#include <OneWire.h>
#include <Arduino.h>
// #include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <DallasTemperature.h>
#include <ArduinoHttpClient.h>
#include <BlitzCloudDisplayController.h>

void (*resetBoard)(void) = 0;
void connectSimToNetwork();
// SIM AND HTTP CLIENT
#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>

SoftwareSerial sim800l(3, 2);

// DEBUGGING
// #define DUMP_AT_COMMANDS
#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(sim800l, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(sim800l);
#endif

TinyGsmClient client(modem);
HttpClient httpClient(client, "130.61.92.168", 8030);
int *gpio = nullptr;

// Display
DisplayLed2Digits display(6, 5, 4, 1, gpio, 7, 8);

// TEMPERATURA

// Data wire is conntec to the Arduino digital pin 4
#define ONE_WIRE_BUS 12

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

void setup()
{
  Serial.begin(9600);
  sensors.begin();
  Serial.println("Wait...");
  sim800l.begin(9600);
  delay(6000);
  Serial.println("Initializing modem...");
  modem.restart();
  String modemInfo = modem.getModemInfo();
  Serial.print("Modem Info: ");
  Serial.println(modemInfo);
  if (modemInfo.length() == 0)
  {
    display.showCharacter("ES0", 3);
    resetBoard();
  }
}

void loop()
{
  unsigned int start = millis();

  connectSimToNetwork();
  String time = modem.getGSMDateTime(DATE_TIME);
  Serial.println(time);
  Serial.println("Making POST request");
  String contentType = "application/json";
  char data[128];
  sensors.requestTemperatures();
  float tempInside = sensors.getTempCByIndex(0);
  float tempOutside = 0.0;
  sprintf(data, "{\"tempInside\":%d.%d,\"tempOutside\":%d.%d,\"time\":\"%s\",\"name\":\"Arduino/V1.0\"}", int(tempInside), (int(tempInside * 100) % 100), int(tempOutside), (int(tempOutside * 100) % 100), time.c_str());
  Serial.println(data);
  String testPayload = "testdata";
  httpClient.post("/", contentType, data);
  int statusCode = httpClient.responseStatusCode();
  String response = httpClient.responseBody();

  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);
  Serial.println("Wait five seconds");
  Serial.print("Delta time:");
  Serial.println(millis() - start);

  delay(5000);
}

void connectSimToNetwork()
{

  Serial.print("Waiting for network...");
  if (!modem.waitForNetwork())
  {
    Serial.println(" fail");
    display.showCharacter("ES1", 3);
    delay(10000);
    return;
  }
  Serial.println(" success");

  if (modem.isNetworkConnected())
  {
    Serial.println("Network connected");
  }
  Serial.print(F("Connecting to "));
  Serial.print("broadband");
  if (!modem.gprsConnect("broadband", "", ""))
  {
    Serial.println(" fail");
    display.showCharacter("ES2", 3);
    delay(10000);
    return;
  }
  Serial.println(" success");

  if (modem.isGprsConnected())
  {
    Serial.println("GPRS connected");
  }
}