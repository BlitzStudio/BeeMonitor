#include <HX711.h>
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

double TemperaturiInterioare[6];
int indexTemperaturi = 0;
int swarmDetected = 0;

void connectSimToNetwork();
// SIM AND HTTP CLIENT
#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>

SoftwareSerial sim800l(10, 11);

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

void calculateSumInWindow(int length, int smaWindow, double data[],
                          double *&sum, double (*func)(double, int))
{
  int i, j, poz = 0;
  double suma;
  sum = new double[length - smaWindow + 1];
  for (i = smaWindow - 1; i < length; i++)
  {
    suma = 0.0;
    for (j = 0; j < smaWindow; j++)
    {
      suma += data[i - smaWindow + 1 + j];
    }
    sum[poz++] = func(suma, smaWindow);
  }
}

void calculateSumOfSquaresInWindow(int length, int smaWindow, double data[],
                                   double *&sum)
{
  int i, j, poz = 0;
  double suma;
  sum = new double[length - smaWindow + 1];
  for (i = smaWindow - 1; i < length; i++)
  {
    suma = 0.0;
    for (j = 0; j < smaWindow; j++)
    {
      suma += pow(data[i - smaWindow + 1 + j], 2);
    }
    sum[poz++] = suma;
  }
}

// Modified: No longer used. `num` is the sum, the `windowSize` determines what we want to return
double calculateSum(double num, int windowSize) { return num; }

double calculateSma(double num, int windowSize) { return num / windowSize; }

double calculatePow2(double num, int windowSize) { return num * num; }

void analyzeData()
{

  double *sum = nullptr;
  double *sma = nullptr;
  double *sumOfSquares = nullptr; // Name changed for clarity
  double *stdDev = nullptr;       // Added to store the standard deviation
  int windowSize = 3;             // Explicitly define the window size
  double lastStd = 0.0;
  // Calculate sum of the data inside windows
  calculateSumInWindow(indexTemperaturi, windowSize, TemperaturiInterioare, sum, calculateSum);

  // Calculate SMA based on the data sum
  calculateSumInWindow(indexTemperaturi, windowSize, TemperaturiInterioare, sma, calculateSma);

  // Calculate the sum of squares for the numbers
  calculateSumOfSquaresInWindow(indexTemperaturi, windowSize, TemperaturiInterioare, sumOfSquares);

  // Allocate the correct memory space
  stdDev = new double[indexTemperaturi - windowSize + 1];

  for (int i = 0; i < indexTemperaturi - windowSize + 1; i++)
  {
    stdDev[i] = sumOfSquares[i] / windowSize - sma[i] * sma[i];
    // Check if it is negative due to floating point errors
    if (stdDev[i] < 0)
    {
      stdDev[i] = 0;
    }
    stdDev[i] = sqrt(stdDev[i] * windowSize / (windowSize - 1));

    if (lastStd == 0.0)
    {
      lastStd = stdDev[i];
    }
    else
    {
      if (stdDev[i] < lastStd)
      {
      }
      else if (stdDev[i] > 0.4 && !swarmDetected)
      {
        digitalWrite(38, HIGH);
        delay(300);
        digitalWrite(38, LOW);
        Serial.println("Roieste");
        swarmDetected = 1;
        modem.sendSMS("+40770672051", "Roieste, pentru mai multe detalii verifica https://beemonitor.blitzcloud.me/temps");
      }
      else if (stdDev[i] < 0.4 && swarmDetected)
      {
        swarmDetected = 0;
      }
      lastStd = stdDev[i];
    }
  }

  // Properly deallocate memory
  delete[] sma;
  delete[] sum;
  delete[] sumOfSquares;
  delete[] stdDev;
}

HX711 scale;
void setup()
{

  pinMode(38, OUTPUT);
  Serial.begin(9600);
  Serial.println("HX711 Demo");
  Serial.println("Initializing the scale");

  scale.begin(40, 3);

  // by the SCALE parameter (not set yet)

  scale.set_scale(317.718);
  scale.tare();

  sensors.begin();
  sensors.requestTemperatures();
  if (sensors.getTempCByIndex(0) == -127 || sensors.getTempCByIndex(1) == -127)
  {
    display.showCharacter("ET0", 3);
    resetBoard();
  }
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
  sensors.requestTemperatures();
  float tempInside = sensors.getTempCByIndex(0);
  float tempOutside = sensors.getTempCByIndex(1);
  float mass = scale.get_units(10);
  Serial.println(mass);
  delay(3000);
  // calibrare
  if (indexTemperaturi < 3)
  {
    Serial.println("Here");
    TemperaturiInterioare[indexTemperaturi++] = tempInside;
    delay(1000);
    return;
  }
  else if (indexTemperaturi == 6)
  {
    Serial.println("Restructurare");
    for (int i = 0; i < indexTemperaturi; i++)
    {
      Serial.println(TemperaturiInterioare[i]);
    }
    Serial.println();
    for (int i = 0; i < indexTemperaturi - 1; i++)
    {
      TemperaturiInterioare[i] = TemperaturiInterioare[i + 1];
    }
    TemperaturiInterioare[indexTemperaturi - 1] = tempInside;
    for (int i = 0; i < indexTemperaturi; i++)
    {
      Serial.println(TemperaturiInterioare[i]);
    }
  }
  else
  {
    TemperaturiInterioare[indexTemperaturi++] = tempInside;
  }
  analyzeData();

  connectSimToNetwork();
  String time = modem.getGSMDateTime(DATE_FULL);
  Serial.println("Making POST request");
  String contentType = "application/json";
  char data[128];
  if (mass < 0.0)
  {
    mass = 0.0;
  }
  sprintf(data, "{\"tempInside\":%d.%d,\"tempOutside\":%d.%d,\"time\":\"%s\",\"mass\":%d.%d,\"name\":\"Arduino/V1.0\"}", int(tempInside), (int(tempInside * 100) % 100), int(tempOutside), (int(tempOutside * 100) % 100), time.c_str(), int(mass), (int(mass * 100) % 100));
  Serial.println(data);
  httpClient.post("/", contentType, data);
  int statusCode = httpClient.responseStatusCode();
  String response = httpClient.responseBody();
  Serial.print("Status code ");
  Serial.println(statusCode);
  analyzeData();
  if (statusCode < 0)
  {
    modem.restart();
    resetBoard();
  }
  Serial.println("Wait 45s");

  delay(1000);
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