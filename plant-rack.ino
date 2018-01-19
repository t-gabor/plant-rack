#define _DEBUG_

#include <time.h>
#include <ESP8266WiFi.h>
#include <ThingerWifi.h>
#include <DHT.h>

#include <Time.h>

#include "wifi-config.h"
#include "thinger-config.h"

const unsigned long postRateInSeconds = 120;

DHT dht(D7, DHT22);

ThingerWifi thing(ThingerUsername, ThingerDeviceId, ThingerDeviceCredentials);

void setup()
{
  Serial.begin(115200);

  setupTime();

  thing.add_wifi(WiFiSSID, WiFiPSK);

  thing["state"] >> [](pson &out) {
    String t = String(hour(time(nullptr))) + ":" + String(minute(time(nullptr)));

    out["Device Time"] = t;
    out["Humidity"] = dht.readHumidity();
    out["Temperature"] = dht.readTemperature();
  };
}

time_t lastSync = 0;

void loop()
{
  thing.handle();

  time_t now = time(nullptr);
  if (now > lastSync + postRateInSeconds)
  {
    if (thing.stream(thing["state"]))
    {
      lastSync = now;
    }
  }
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
