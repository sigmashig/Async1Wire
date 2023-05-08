#include "Async1WireMgr.hpp"
#include <DallasTemperature.h>

char Async1WireMgr::addrPrinted[SIZE_OF_ADDRESS_PRINTED + 1];

Async1WireMgr::Async1WireMgr()
{
    temperatureLoopTimer = xTimerCreateStatic("TemperatureLoopTimer", pdMS_TO_TICKS(temperatureTimerInterval),
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
    if (isInitialized)
    {
        Serial.println("Point 10");
        std::map<String, Thermometer *> inActiveThermometers;

        for (auto &thermometer : thermometers)
        {
            if (!thermometer.second->Status)
            {
                inActiveThermometers[thermometer.first] = thermometer.second;
            }
            else
            {
                thermometer.second->Status = false;
            }
        }

        for (auto &oneWireUnit : oneWireCollection)
        {
            Serial.println("Point 11");
            oneWireUnit.second->reset_search();
            Address1Wire deviceAddress;
            // Step1: find all devices
            std::vector<Address1Wire> foundDevices;
            while (oneWireUnit.second->search(deviceAddress.addr))
            {
                Serial.println("Point 12");

                if (oneWireUnit.second->crc8(deviceAddress.addr, 7) != deviceAddress.addr[7])
                {
                    Serial.println("Point 13");

                    Thermometer *t = getThermometer(deviceAddress);
                    ThermometerChanges tc;
                    tc.Address = deviceAddress;
                    tc.Event = UNIT_ERROR;
                    strncpy(tc.Name, t->Name.c_str(), sizeof(tc.Name));
                    tc.OldName[0] = 0;
                    tc.Pin = t->Pin;
                    strncpy(tc.Message,"CRC Error", sizeof(tc.Message));
                    notifyThermometerChanges(&tc);
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
                    Serial.println("Point 14");

                    Thermometer *t = getThermometer(addr);
                    bool isNew = (t == nullptr);
                    if (isNew)
                    {
                        t = new Thermometer();
                        t->Name = "T" + String(addr.packedAddress, HEX);
                        t->Address = addr;
                        thermometers[t->Name] = t;
                    }
                    t->Pin = oneWireUnit.first;
                    t->Status = true;
                    t->IsParasitePowered = dt.isParasitePowerMode();
                    t->Resolution = dt.getResolution(addr.addr);
                    t->Temperature = 0;
                    dt.setResolution(addr.addr, DEFAULT_RESOLUTION);

                    if (inActiveThermometers.count(t->Name) > 0 || isNew)
                    {
                        Serial.println("Point 15");

                        ThermometerChanges changes;
                        strncpy(changes.Name, t->Name.c_str(), sizeof(changes.Name));
                        changes.Address = t->Address;
                        changes.Pin = t->Pin;
                        changes.OldName[0] = 0;

                        if (isNew)
                        {
                            changes.Event = UNIT_ADDED;
                        }
                        else
                        {
                            changes.Event = UNIT_CONNECTION_RESTORED;
                        }
                        notifyThermometerChanges(&changes);
                    }
                }
                break;
                }
            }
        }
        for (auto &thermometer : thermometers)
        {
            if (!thermometer.second->Status)
            {
                if (inActiveThermometers.count(thermometer.first) == 0)
                {
                    Serial.println("Point 16");

                    ThermometerChanges changes;
                    changes.Event = UNIT_CONNECTION_LOST;
                    strncpy(changes.Name, thermometer.second->Name.c_str(), sizeof(changes.Name));
                    changes.Address = thermometer.second->Address;
                    changes.Pin = thermometer.second->Pin;
                    changes.OldName[0] = 0;
                    notifyThermometerChanges(&changes);
                }
            }
        }
    }
}

const char *Async1WireMgr::PrintAddress(Address1Wire addr)
{
    sprintf(addrPrinted, "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
            addr.addr[0], addr.addr[1], addr.addr[2], addr.addr[3],
            addr.addr[4], addr.addr[5], addr.addr[6], addr.addr[7]);
    return addrPrinted;
}

void Async1WireMgr::SetThermometerName(String newName, Address1Wire addr)
{
    Thermometer *thermometer = getThermometer(addr);
    ThermometerChanges changes;
    strncpy(changes.Name, newName.c_str(), sizeof(changes.Name));
    changes.Address = addr;
    if (thermometer != nullptr)
    {
        thermometer->Status = false;
        changes.Event = UNIT_RENAMED;
        changes.Pin = thermometer->Pin;
        strncpy(changes.OldName, thermometer->Name.c_str(), sizeof(changes.OldName));
        notifyThermometerChanges(&changes);

        thermometers.erase(thermometer->Name);
        thermometer->Name = newName;
        thermometers[newName] = thermometer;
    }
    else
    {
        thermometer = new Thermometer();
        thermometer->Name = newName;
        thermometer->Pin = 0;
        thermometer->Address = addr;
        thermometer->Status = false;
        thermometer->IsParasitePowered = false;
        thermometer->Resolution = DEFAULT_RESOLUTION;
        thermometer->Temperature = 0;
        thermometers[newName] = thermometer;

        changes.Event = UNIT_ADDED;
        changes.Pin = 0;
        changes.OldName[0] = 0;
        notifyThermometerChanges(&changes);

        SearchDevices();
    }
}

void Async1WireMgr::SetTemperatureTimerInterval(ulong interval)
{
    temperatureTimerInterval = interval;
    xTimerChangePeriod(temperatureLoopTimer, pdMS_TO_TICKS(temperatureTimerInterval), 0);
}

void Async1WireMgr::onThermometerChangesSubscribe(ThermometerCallback callback)
{
    thermometerChangesSubscriptions.push_back(callback);
}

void Async1WireMgr::onTemperatureChangesSubscribe(TemperatureCallback callback)
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

void Async1WireMgr::onTemperatureChangesUnSubscribe(TemperatureCallback callback)
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

void Async1WireMgr::notifyThermometerChanges(const ThermometerChanges *t)
{
    for (auto &callback : thermometerChangesSubscriptions)
    {
 
        ThermChangesParams params = ThermChangesParams();
        Serial.printf("notifyThermometerChanges: size(p)=%u size(t)=%lu size(c)=%lu\n", sizeof(params.t), sizeof(*t), sizeof(callback));
        //memcpy(&(params.t), t, sizeof(*t));

        params.t = *t;
        params.callback = callback;

        xTaskCreate(runThermometerChanges, "ThermometerChanges", taskStackSize, &params, 1, NULL);
    }
}

void Async1WireMgr::notifyTemperatureChanges(const Thermometer *t)
{
    for (auto &callback : temperatureSubscriptions)
    {
        TemperatureChangesParams* params = new TemperatureChangesParams();
        params->t = (Thermometer*)t;
        params->callback = callback;
        xTaskCreate(runTemperatureChanges, "TemperatureChanges", taskStackSize, params, 1, NULL);
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
                bool isConnected = dt.isConnected(t->Address.addr);
                if (isConnected)
                {
                    if (!t->Status)
                    {
                        t->Status = true;
                        ThermometerChanges changes;
                        changes.Event = UNIT_CONNECTION_RESTORED;
                        strncpy(changes.Name, t->Name.c_str(), sizeof(changes.Name));
                        changes.Address = t->Address;
                        changes.Pin = t->Pin;
                        changes.OldName[0] = 0;
                        OneWireMgr.notifyThermometerChanges(&changes);
                    }

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
                else
                {
                    if (t->Status)
                    {
                        t->Status = false;
                        ThermometerChanges changes;
                        changes.Event = UNIT_CONNECTION_LOST;
                        strncpy(changes.Name, t->Name.c_str(), sizeof(changes.Name));
                        changes.Address = t->Address;
                        changes.Pin = t->Pin;
                        changes.OldName[0] = 0;
                        OneWireMgr.notifyThermometerChanges(&changes);
                    }
                }
            }
        }
        xTimerReset(OneWireMgr.temperatureLoopTimer, 0);
    }
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

void Async1WireMgr::runThermometerChanges(void *params){
    Serial.println("Point1");
    ThermChangesParams *p = (ThermChangesParams*)params;
    Serial.println("Point2");
    p->callback(NULL /*p->t*/);
    Serial.println("Point3");
    // delete p;
    vTaskDelete(NULL);
}

void Async1WireMgr::runTemperatureChanges(void *params)
{
    //TemperatureChangesParams *p = (TemperatureChangesParams *)params;
    //p->callback(p->t);
    //delete p;
    vTaskDelete(NULL);
}
//--------------------------------------------------------------------------
Async1WireMgr OneWireMgr;