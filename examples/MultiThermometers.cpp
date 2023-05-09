#include <Arduino.h>
#include "Async1WireMgr.hpp"
#include <DallasTemperature.h>


void thermometerHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  ThermometerEvent *t = (ThermometerEvent *)event_data;
  switch (t->Event)
  {
  case UNIT_RENAMED:
    Serial.printf("[%lu]RENAMED!!! Thermometer: OldName=%s -> Name=%s\n", millis(), t->OldName, t->Name);
    break;
  case UNIT_ADDED:
    Serial.printf("[%lu]ADDED!!! Thermometer: Name=%s\n", millis(), t->Name);
    break;
  case UNIT_CONNECTION_LOST:
    Serial.printf("[%lu]LOST!!! Thermometer: Name=%s\n", millis(), t->Name);
    break;
  case UNIT_CONNECTION_RESTORED:
    Serial.printf("[%lu]RESTORED!!! Thermometer: Name=%s\n", millis(), t->Name);
    break;
  case UNIT_ERROR:
    Serial.printf("[%lu]ERROR!!! Thermometer: Name=%s Error=%u\n ", millis(), t->Name, t->ErrorCode);
    break;
  }
}

void temperatureHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  TemperatureEvent *t = (TemperatureEvent *)event_data;

  Serial.print("EVENT!!! Temperature: Name=");
  Serial.print(t->Name);
  Serial.print(", temp=");
  Serial.println(t->Temperature);
}


void setup(void)
{
  Serial.begin(115200);
  Serial.println("-----------------------------------------");
  esp_err_t espRes;
 
  // Subscribe for event of thermometer's changes
  espRes = esp_event_handler_instance_register(ONEWIRE_EVENT,
                                               ONEWIRE_EVENT_THERMOMETER,
                                               thermometerHandler,
                                               NULL,
                                               NULL);
  if (espRes != ESP_OK)
  {
    Serial.printf("Failed to register thermometer event handler: %d", espRes);
  }
  // Subscribe for event of temperature's changes
  espRes = esp_event_handler_instance_register(ONEWIRE_EVENT,
                                               ONEWIRE_EVENT_TEMPERATURE,
                                               temperatureHandler,
                                               NULL,
                                               NULL);
  if (espRes != ESP_OK)
  {
    Serial.printf("Failed to register temperature event handler: %d", espRes);
  }
  
 
  bool res = false;

  // Add 1Wire on pin 25
  res = OneWireMgr.Add1Wire(25);
  Serial.printf("Add 1Wire on pin 25: %d\n", res);
  
  // Add new thermometer manually (there are search devices before begin work)
  Serial.println("Add new thermometer manually");
  OneWireMgr.SetThermometerName("T_first_thermometer", Address1Wire({0x28, 0xff, 0xe6, 0xe4, 0x90, 0x16, 0x04, 0x46}));

  // Set timer interval for temperature refresh in ms
  Serial.println("Set timer interval for temperature refresh");
  OneWireMgr.SetTemperatureTimerInterval(5 * 1000); // 5 sec
  // begin work
  Serial.println("begin work");
  OneWireMgr.Init();
  // rename thermometer
  Serial.println("rename thermometer");
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
  vTaskDelete(NULL);
}
