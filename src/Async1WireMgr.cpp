#include "Async1WireMgr.hpp"
#include <DallasTemperature.h>

Async1WireMgr::Async1WireMgr()
{
    temperatureLoopTimer = xTimerCreateStatic("TemperatureLoopTimer", pdMS_TO_TICKS(TIMER_LOOP_PERIOD_THERMOMETERS),
                                              pdTRUE, NULL, onTemperatureLoopTimer, &temperatureLoopBuffer);
}

void Async1WireMgr::Init()
{
    if (!isInitialized)
    {

        for (auto &oneWire : oneWireCollection)
        {
            oneWire.second->begin(oneWire.first);
        }
        isInitialized = true;
    }
    SearchDevices();
    requestTemperature();
    xTimerStart(temperatureLoopTimer, 0);
    
}

bool Async1WireMgr::Add1Wire(byte pin)
{
    if (oneWireCollection.find(pin) != oneWireCollection.end())
    {
        return false;
    }
    oneWireCollection[pin] = new OneWire();
    if (isInitialized)
    {
        oneWireCollection[pin]->begin(pin);
    }
    return true;
}

bool Async1WireMgr::Remove1Wire(byte pin)
{
    if (oneWireCollection.find(pin) != oneWireCollection.end())
    {
        oneWireCollection.erase(pin);
        return true;
    }
    return false;
}

void Async1WireMgr::SearchDevices()
{
    for (auto &oneWireUnit : oneWireCollection)
    {
        oneWireUnit.second->reset_search();
        Address1Wire deviceAddress;
        for (auto &thermometer : thermometers)
        {
            thermometer.second->Status = false;
        }
        // Step1: find all devices
        std::vector<Address1Wire> foundDevices;
        while (oneWireUnit.second->search(deviceAddress.addr))
        {
            if (oneWireUnit.second->crc8(deviceAddress.addr, 7) != deviceAddress.addr[7])
            {
                Serial.println("Async1WireMgr::SearchDevices() - CRC Error");
                continue;
            }

            foundDevices.push_back(deviceAddress);
        }

        // Step2: detect device found
        DallasTemperature dt = DallasTemperature(oneWireUnit.second);
        for (auto &addr : foundDevices)
        {
            switch (DetectFamily(addr))
            {
            case ONEWIRE_DSTHERMO:
            {
                Thermometer *t = getThermometer(addr);
                if (t == nullptr)
                {
                    t = new Thermometer();
                    t->Name = "T" + String(addr.packedAddress, HEX);
                    t->Address = addr;
                    t->Pin = oneWireUnit.first;
                    t->Status = true;
                    t->IsParasitePowered = dt.isParasitePowerMode();
                    t->Resolution = dt.getResolution(addr.addr);
                    t->Temperature = 0;
                    dt.setResolution(addr.addr, DEFAULT_RESOLUTION);
                    notifyThermometerChanges(t);
                }
                else
                {
                    t->Status = true;
                }
                thermometers[t->Name] = t;
            }
            break;
            }
        }
    }
}

void Async1WireMgr::SetThermometerName(String newName, Address1Wire addr, byte pin)
{
    Thermometer *thermometer = getThermometer(addr);
    if (thermometer != nullptr)
    {
        thermometers.erase(thermometer->Name);
        thermometer->Name = newName;
        thermometers[newName] = thermometer;
    }
    else
    {
        thermometer = new Thermometer();
        thermometer->Name = newName;
        thermometer->Pin = pin;
        thermometer->Address = addr;
        thermometer->Status = false;
        thermometer->IsParasitePowered = false;
        thermometer->Resolution = DEFAULT_RESOLUTION;
        thermometer->Temperature = 0;
        thermometers[newName] = thermometer;
        SearchDevices();
    }
}

void Async1WireMgr::onThermometerChangesSubscribe(ThermometerCallback callback)
{
    thermometerChangesSubscriptions.push_back(callback);
}

void Async1WireMgr::onTemperatureChangesSubscribe(ThermometerCallback callback)
{
    temperatureSubscriptions.push_back(callback);
}

void Async1WireMgr::onThermometerChangesUnSubscribe(ThermometerCallback callback)
{
    for (auto it = thermometerChangesSubscriptions.begin(); it != thermometerChangesSubscriptions.end(); ++it)
    {
        if (*it == callback)
        {
            thermometerChangesSubscriptions.erase(it);
            break;
        }
    }
}

void Async1WireMgr::onTemperatureChangesUnSubscribe(ThermometerCallback callback)
{
    for (auto it = temperatureSubscriptions.begin(); it != temperatureSubscriptions.end(); ++it)
    {
        if (*it == callback)
        {
            temperatureSubscriptions.erase(it);
            break;
        }
    }
}
Thermometer *Async1WireMgr::getThermometer(Address1Wire addr)
{
    for (auto &thermometer : thermometers)
    {
        if (thermometer.second->Address.packedAddress == addr.packedAddress)
        {
            return thermometer.second;
        }
    }
    return nullptr;
}

void Async1WireMgr::notifyThermometerChanges(const Thermometer *t)
{
    for (auto &callback : thermometerChangesSubscriptions)
    {
        callback(t);
    }
}

void Async1WireMgr::notifyTemperatureChanges(const Thermometer *t)
{
    for (auto &callback : temperatureSubscriptions)
    {
        callback(t);
    }
}

OneWireDevices Async1WireMgr::DetectFamily(Address1Wire deviceAddress)
{
    switch (deviceAddress.addr[0])
    {
    case DS18S20MODEL:
    case DS18B20MODEL:
    case DS1822MODEL:
    case DS1825MODEL:
    case DS28EA00MODEL:
        return ONEWIRE_DSTHERMO;
    default:
        return ONEWIRE_GENERIC;
    }
}

void Async1WireMgr::onTemperatureLoopTimer(TimerHandle_t xTimer)
{
    for (auto &ow : OneWireMgr.oneWireCollection)
    {
        OneWire *oneWire = ow.second;
        DallasTemperature dt = DallasTemperature(oneWire);
        for (auto &th : OneWireMgr.thermometers)
        {
            Thermometer *t = th.second;
            if (t->Pin == ow.first)
            {
                if (t->Status)
                {
                    double temp = dt.getTempC(t->Address.addr);
                    temp = round(temp * 10) / 10;
                    if (t->Temperature != temp)
                    {
                        t->Temperature = temp;
                        OneWireMgr.notifyTemperatureChanges(t);
                    }

                    dt.requestTemperaturesByAddress(t->Address.addr);
                }
            }
        }
    }
    xTimerReset(OneWireMgr.temperatureLoopTimer, 0);
}

void Async1WireMgr::requestTemperature()
{
    for (auto &ow : OneWireMgr.oneWireCollection)
    {
        OneWire *oneWire = ow.second;
        DallasTemperature dt = DallasTemperature(oneWire);
        for (auto &th : OneWireMgr.thermometers)
        {
            Thermometer *t = th.second;
            if (t->Pin == ow.first)
            {
                if (t->Status)
                {
                    dt.requestTemperaturesByAddress(t->Address.addr);
                }
            }
        }
    }
}

//--------------------------------------------------------------------------
Async1WireMgr OneWireMgr;