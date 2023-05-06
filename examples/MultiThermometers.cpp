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

  // Add new thermometer manually (there are search devices before begin work)
  OneWireMgr.SetThermometerName("T_first_thermometer", Address1Wire({0x28, 0xff, 0xe6, 0xe4, 0x90, 0x16, 0x04, 0x46}));

  // Set timer interval for temperature refresh in ms
  OneWireMgr.SetTemperatureTimerInterval(5 * 1000); // 5 sec
  // begin work
  OneWireMgr.Init();
  // rename thermometer
  OneWireMgr.SetThermometerName("T_Second_thermometer", Address1Wire({0x28, 0xff, 0x3e, 0x9a, 0x87, 0x16, 0x03, 0x28}));

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
