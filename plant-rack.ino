#define _DEBUG_

#include <time.h>
#include <ESP8266WiFi.h>
#include <ThingerWifi.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>

#include <Time.h>

#include "wifi-config.h"
#include "thinger-config.h"

#define POST_RATE_IN_SECONDS 60

#define LIGHT_PIN D3
#define LIGHT_PIN_UNUSED D2

#define FAN_PIN D1

#define SENSOR_PIN D7
#define DS18B20_PIN D6

DHT sensor(SENSOR_PIN, DHT22);

OneWire oneWire(DS18B20_PIN);
DallasTemperature dsSensor(&oneWire);

ThingerWifi thing(ThingerUsername, ThingerDeviceId, ThingerDeviceCredentials);

struct Settings
{
  int lightOnHour;
  int lightOffHour;
  int fanOnTemperature;
  int fanOffTemperature;

  Settings() : lightOnHour(8),
               lightOffHour(20),
               fanOnTemperature(28),
               fanOffTemperature(25)
  {
  }
};

Settings settings;

float temperature;
float humidity;
bool lightOn = false;
bool fanOn = false;

void setup()
{
  Serial.begin(115200);
  dsSensor.begin();
  loadSettings();

  setupTime();
  setupThing();

  pinMode(LIGHT_PIN, OUTPUT);
  digitalWrite(LIGHT_PIN, HIGH);

  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);
}

time_t lastSync = 0;

void loop()
{
  thing.handle();

  time_t now = time(nullptr);
  if (now > lastSync + POST_RATE_IN_SECONDS)
  {
    if (thing.stream(thing["state"]))
    {
      lastSync = now;
    }
  }

  if (hour(now) >= settings.lightOnHour && hour(now) < settings.lightOffHour)
    lightOn = true;
  else
    lightOn = false;
  digitalWrite(LIGHT_PIN, lightOn ? LOW : HIGH); // inverse relay

  if (!fanOn && temperature > settings.fanOnTemperature)
    fanOn = true;
  else if (fanOn && temperature < settings.fanOffTemperature)
    fanOn = false;
  digitalWrite(FAN_PIN, fanOn ? HIGH : LOW);
}

void setupTime()
{
  Serial.println();
  time_t now = time(nullptr);
  configTime(1 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  while (!now)
  {
    time(&now);
    Serial.print(".");
    delay(200);
  }

  Serial.println(ctime(&now));
}

void setupThing()
{
  thing.add_wifi(WiFiSSID, WiFiPSK);

  thing["state"] >> [](pson &out) {
    String t = String(hour(time(nullptr))) + ":" + String(minute(time(nullptr)));
    humidity = sensor.readHumidity();
    temperature = sensor.readTemperature();
    dsSensor.requestTemperatures();

    out["Device Time"] = t;
    out["Humidity"] = humidity;
    out["Temperature"] = temperature;
    out["Temperature 2"] = round(dsSensor.getTempCByIndex(0) * 10) / 10.0;
    out["fanOn"] = fanOn;
    out["lightOn"] = lightOn;
  };

  thing["lightOnHour"] << [](pson &in) {
    if (in.is_empty())
      in = settings.lightOnHour;
    else
      settings.lightOnHour = in;

    Serial.printf("lightOnHour: %2d\n", settings.lightOnHour);
    saveSettings();
  };

  thing["lightOffHour"] << [](pson &in) {
    if (in.is_empty())
      in = settings.lightOffHour;
    else
      settings.lightOffHour = in;

    Serial.printf("lightOffHour: %2d\n", settings.lightOffHour);
    saveSettings();
  };

  thing["fanOnTemperature"] << [](pson &in) {
    if (in.is_empty())
      in = settings.fanOnTemperature;
    else
      settings.fanOnTemperature = in;

    Serial.printf("fanOnTemperature: %2d\n", settings.fanOnTemperature);
    saveSettings();
  };

  thing["fanOffTemperature"] << [](pson &in) {
    if (in.is_empty())
      in = settings.fanOffTemperature;
    else
      settings.fanOffTemperature = in;

    Serial.printf("fanOffTemperature: %2d\n", settings.fanOffTemperature);
    saveSettings();
  };
}

void loadSettings()
{
  EEPROM.begin(128);
  char flag;
  EEPROM.get(0, flag);
  if (flag == 0xb0)
    EEPROM.get(1, settings);
  EEPROM.end();
}

void saveSettings()
{
  EEPROM.begin(128);
  char flag = 0xb0;
  EEPROM.put(0, flag);
  EEPROM.put(1, settings);
  EEPROM.commit();
  EEPROM.end();
}