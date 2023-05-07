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

#define SIZE_OF_ADDRESS_PRINTED (sizeof("00:00:00:00:00:00:00:00") - 1)

typedef enum
{
    UNIT_RENAMED,
    UNIT_ADDED,
    UNIT_CONNECTION_LOST,
    UNIT_CONNECTION_RESTORED,
    UNIT_ERROR
} ChangesEvent;

typedef union
{
    byte addr[8];
    int64_t packedAddress;
} Address1Wire;

typedef struct
{
    String Name;
    String OldName;
    String Message;
    Address1Wire Address;
    byte Pin;
    ChangesEvent Event;
} ThermometerChanges;

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

typedef void (*ThermometerCallback)(const ThermometerChanges *t);
typedef void (*TemperatureCallback)(const Thermometer *t);

class Async1WireMgr
{
public:
    /// @brief Default constructor.
    /// @details No activities here. Just initialize variables.
    Async1WireMgr();

    /// @brief Begin work.
    /// @details This method must be called before any other method.
    ///       - It initializes all internal variables and starts timer.
    ///       - Initialize OneWire buses
    ///       - Search for devices
    void Init();

    /// @brief Add OneWire bus to collection.
    /// @details You should add OneWireBus to collection any time. If you add bus after initialization, the bus will be initialized.
    /// @param pin - pin number to which the bus is connected.
    ///
    /// @return true, if bus was added successfully. Bus will be initialized when this method is called after Asnc1Wire mgr initialization.
    ///          false, if bus already exists in comllection.
    bool Add1Wire(byte pin);
    /// @brief Remove OneWire bus from collection.
    /// @param pin
    /// @return true if bus was removed successfully. false if bus was not found.
    bool Remove1Wire(byte pin);

    /// @brief Search for devices on all buses.
    /// @details This method will search for devices on all buses and update internal collection of devices.
    ///          If new device was found, it will be added to collection.
    ///          This method doesn't remove any devices event if it is not found on the bus.
    ///         The devices not found are marked as Sttus=false
    void SearchDevices();

    /// @brief Set name/Add thermometer to collection
    /// @details If thermometer with the same address already exists, it's name will be updated.
    ///          If thermometer with the same address doesn't exists, it will be added to collection.
    /// @param newName
    /// @param addr
    void SetThermometerName(String newName, Address1Wire addr);

    /// @brief Get number of thermometers in collection.
    /// @return number of thermometers in collection.
    int GetNumbThermometers() { return thermometers.size(); };

    /// @brief Get the collection of thermometers (ReadOnly)
    /// @return Collection of thermometers
    const std::map<String, Thermometer *> GetThermometers() { return thermometers; };

    /// @brief Add callback on thermometer changes (Add new thermometer).
    /// @details This method adds callback to collection of callbacks.
    ///          The callback will be called when new thermometer is added to collection in both cases: Manually or by SearchDevices method.
    void onThermometerChangesSubscribe(ThermometerCallback callback);
    /// @brief Remove callback on thermometer changes (Add new thermometer).
    /// @param callback
    void onThermometerChangesUnSubscribe(ThermometerCallback callback);

    /// @brief Add callback on temperature changes.
    /// @details This method adds callback to collection of callbacks.
    ///          The callback will be called when temperature of any thermometer is changed.
    ///          The precision of temperature is 0.1 degree.
    /// @param callback
    void onTemperatureChangesSubscribe(TemperatureCallback callback);
    /// @brief Remove callback on temperature changes.
    /// @param callback
    void onTemperatureChangesUnSubscribe(TemperatureCallback callback);

    /// @brief Detect family of device by address.
    /// @details This method detects family of device by address.
    /// @param deviceAddress
    /// @return Type of device
    OneWireDevices DetectFamily(Address1Wire deviceAddress);

    /// @brief Set interval for temperature loop.
    /// @details This method sets interval for temperature loop.
    ///          The temperature refreshed every interval. No refresh between intervals.
    ///          The default value is 15 seconds.(TIMER_LOOP_PERIOD_THERMOMETERS)
    void SetTemperatureTimerInterval(ulong interval);
    /// @brief Print OneWire address to string.
    /// @param addr
    /// @return buffer with printed address. Please, note that the buffer is static and just one for all calls.
    static const char *PrintAddress(Address1Wire addr);

private:
    bool isInitialized = false;
    static char addrPrinted[SIZE_OF_ADDRESS_PRINTED + 1];
    ulong temperatureTimerInterval = TIMER_LOOP_PERIOD_THERMOMETERS;
    std::map<byte, OneWire *> oneWireCollection;
    std::map<String, Thermometer *> thermometers;

    std::vector<ThermometerCallback> thermometerChangesSubscriptions;
    std::vector<TemperatureCallback> temperatureSubscriptions;
    StaticTimer_t temperatureLoopBuffer;
    TimerHandle_t temperatureLoopTimer;

    Thermometer *getThermometer(Address1Wire addr);
    void notifyThermometerChanges(const ThermometerChanges *t);
    void notifyTemperatureChanges(const Thermometer *t);
    static void onTemperatureLoopTimer(TimerHandle_t xTimer);
    void requestTemperature();
};

extern Async1WireMgr OneWireMgr;