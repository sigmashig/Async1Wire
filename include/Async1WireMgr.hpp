#pragma once
#include <map>
#include <vector>
#include <Arduino.h>
#include <OneWire.h>

#ifndef TIMER_LOOP_PERIOD_THERMOMETERS
#define TIMER_LOOP_PERIOD_THERMOMETERS 15 * 1000
#endif

#ifndef DEFAULT_RESOLUTION
#define DEFAULT_RESOLUTION 10
#endif

typedef union
{
    byte addr[8];
    int64_t packedAddress;
} Address1Wire;

typedef enum
{
    ONEWIRE_NONE,
    ONEWIRE_GENERIC,
    ONEWIRE_DSTHERMO
} OneWireDevices;

typedef struct
{
    String Name;
    Address1Wire Address;
    byte Pin;
    bool Status;
    bool IsParasitePowered;
    byte Resolution;
    double Temperature;
} Thermometer;

typedef void (*ThermometerCallback)(const Thermometer *t);

class Async1WireMgr
{
public:
    // OneWire *oneWire[];
    Async1WireMgr();
    void Init();
    bool Add1Wire(byte pin);
    bool Remove1Wire(byte pin);
    void SearchDevices();
    // bool AddThermometer(String name, Address1Wire addr);
    // bool RemoveThermometer(String name);
    void SetThermometerName(String newName, Address1Wire addr, byte pin);
    int GetNumbThermometers() { return thermometers.size(); };
    const std::map<String, Thermometer *> GetThermometers() { return thermometers; };
    void onThermometerChangesSubscribe(ThermometerCallback callback);
    void onTemperatureChangesSubscribe(ThermometerCallback callback);
    void onThermometerChangesUnSubscribe(ThermometerCallback callback);
    void onTemperatureChangesUnSubscribe(ThermometerCallback callback);
    OneWireDevices DetectFamily(Address1Wire deviceAddress);

private:
    bool isInitialized = false;
    std::map<byte, OneWire *> oneWireCollection;
    std::map<String, Thermometer *> thermometers;

    std::vector<ThermometerCallback> thermometerChangesSubscriptions;
    std::vector<ThermometerCallback> temperatureSubscriptions;
    StaticTimer_t temperatureLoopBuffer;
    TimerHandle_t temperatureLoopTimer;

    Thermometer *getThermometer(Address1Wire addr);
    void notifyThermometerChanges(const Thermometer *t);
    void notifyTemperatureChanges(const Thermometer *t);
    static void onTemperatureLoopTimer(TimerHandle_t xTimer);
};

extern Async1WireMgr OneWireMgr;