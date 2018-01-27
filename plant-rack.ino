#define _DEBUG_

#include <time.h>
#include <ESP8266WiFi.h>
#include <ThingerWifi.h>
#include <DHT.h>

#include <Time.h>

#include "wifi-config.h"
#include "thinger-config.h"

#define POST_RATE_IN_SECONDS 120
#define LIGHT_PIN D3
#define FAN_PIN D1
#define SENSOR_PIN D7

DHT sensor(SENSOR_PIN, DHT22);

ThingerWifi thing(ThingerUsername, ThingerDeviceId, ThingerDeviceCredentials);
int lightOnHour = 8;
int lightOffHour = 20;

void setup()
{
  pinMode(LIGHT_PIN, OUTPUT);
  digitalWrite(LIGHT_PIN, HIGH);

  Serial.begin(115200);

  setupTime();
  setupThing();

  pinMode(LIGHT_PIN, OUTPUT);
  digitalWrite(LIGHT_PIN, HIGH);

  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, HIGH);
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

  if (hour(now) >= lightOnHour && hour(now) < lightOffHour)
    digitalWrite(LIGHT_PIN, LOW);
  else
    digitalWrite(LIGHT_PIN, HIGH);
}

void setupTime()
{
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

    out["Device Time"] = t;
    out["Humidity"] = sensor.readHumidity();
    out["Temperature"] = sensor.readTemperature();
  };

  thing["lightOnHour"] << [](pson &in) {
    if (in.is_empty())
      in = lightOnHour;
    else
      lightOnHour = in;

    Serial.printf("lightOnHour: %2d\n", lightOnHour);

  };

  thing["lightOffHour"] << [](pson &in) {
    if (in.is_empty())
      in = lightOffHour;
    else
      lightOffHour = in;

    Serial.printf("lightOffHour: %2d\n", lightOffHour);
  };
}
