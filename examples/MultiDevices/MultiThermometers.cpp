#include <Arduino.h>
#include "Async1WireMgr.hpp"
#include <DallasTemperature.h>

void newThermometer(const Thermometer *t)
{
  Serial.printf("NEW!!! Thermometer: Name=%s", t->Name.c_str());
  Serial.println();
}

void getTemp(const Thermometer *t)
{
  Serial.print("EVENT!!! Temperature: Name=");
  Serial.print(t->Name);
  Serial.print(", temp=");
  Serial.println(t->Temperature);
}

void setup(void)
{
  Serial.begin(115200);
  Serial.println("-----------------------------------------");
  bool res = false;

// Add 1Wire on pin 25
  res = OneWireMgr.Add1Wire(25);
  Serial.printf("Add 1Wire on pin 25: %d\n", res);
  //Subscribe to new thermometer found
  OneWireMgr.onThermometerChangesSubscribe(newThermometer);

  // subscribe to Temperature changes
  OneWireMgr.onTemperatureChangesSubscribe(getTemp);
  //begin work
  OneWireMgr.Init();

  
  int nTherm = OneWireMgr.GetNumbThermometers();
  Serial.printf("Number of Thermometers: %d\n", nTherm);

  for (auto &n : OneWireMgr.GetThermometers())
  {
    Serial.printf("Thermometer Name=%s, status=%d", n.first.c_str(), n.second->Status);
    Serial.println();
  }
}

void loop(void)
{
  delay(1000);
}
