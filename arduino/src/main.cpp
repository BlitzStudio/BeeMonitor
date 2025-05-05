#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ArduinoHttpClient.h>
#include <BlitzCloudDisplayController.h>

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
DisplayLed2Digits display(6, 5, 4, 1, gpio, 7, 8);

void setup()
{

  // display.showNumber(1234, 2000);
  char content[5];
  strcpy(content, "c1");
  display.showCharacter(content, 3000);

  Serial.begin(9600);
  Serial.println("Wait...");

  // TinyGsmAutoBaud(sim800l, 9600, 115200);

  sim800l.begin(9600);
  delay(6000);

  Serial.println("Initializing modem...");
  modem.restart();
  // modem.init();

  // String modemInfo = modem.getModemInfo();
  // Serial.print("Modem Info: ");
  // Serial.println(modemInfo);
}

void loop()
{
  Serial.print("Waiting for network...");
  if (!modem.waitForNetwork())
  {
    Serial.println(" fail");
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
    delay(10000);
    return;
  }
  Serial.println(" success");

  if (modem.isGprsConnected())
  {
    Serial.println("GPRS connected");
  }
  Serial.print(F("Performing HTTP GET request... "));
  int err = httpClient.get("/");
  if (err != 0)
  {
    Serial.println(F("failed to connect"));
    delay(10000);
    return;
  }

  int status = httpClient.responseStatusCode();
  Serial.print(F("Response status code: "));
  Serial.println(status);
  if (!status)
  {
    delay(10000);
    return;
  }

  Serial.println(F("Response Headers:"));
  while (httpClient.headerAvailable())
  {
    String headerName = httpClient.readHeaderName();
    String headerValue = httpClient.readHeaderValue();
    Serial.println("    " + headerName + " : " + headerValue);
  }

  int length = httpClient.contentLength();
  if (length >= 0)
  {
    Serial.print(F("Content length is: "));
    Serial.println(length);
  }
  if (httpClient.isResponseChunked())
  {
    Serial.println(F("The response is chunked"));
  }

  String body = httpClient.responseBody();
  Serial.println(F("Response:"));
  Serial.println(body);

  Serial.print(F("Body length is: "));
  Serial.println(body.length());

  // Shutdown

  httpClient.stop();
  Serial.println(F("Server disconnected"));
}

// void setUpSim()
// {

//   sim800l.println("AT"); // Once the handshake test is successful, it will back to OK
//   updateSerial();
//   sim800l.println("AT+CSQ"); // Signal quality test, value range is 0-31 , 31 is the best
//   updateSerial();
//   sim800l.println("AT+CCID"); // Read SIM information to confirm whether the SIM is plugged
//   updateSerial();
//   sim800l.println("AT+CREG?"); // Check whether it has registered in the network
//   updateSerial();
//   sim800l.println("AT+CFUN=1");
//   updateSerial();
//   sim800l.println("AT+CPIN?");
//   updateSerial();
//   sim800l.println("AT+CGATT=1");
//   updateSerial();

//   sim800l.println("AT+CSTT=\"broadband\",\"\",\"\"");
//   updateSerial();
//   sim800l.println("AT+CIICR");
//   updateSerial();
//   sim800l.println("AT+CIFSR");
//   updateSerial();
// }

// void updateSerial()
// {
//   delay(500);
//   while (Serial.available())
//   {
//     sim800l.write(Serial.read()); // Forward what Serial received to Software Serial Port
//   }
//   while (sim800l.available())
//   {
//     Serial.write(sim800l.read()); // Forward what Software Serial received to Serial Port
//   }
// }