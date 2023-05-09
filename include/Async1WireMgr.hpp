#pragma once
#include <map>
#include <vector>
#include <Arduino.h>
#include <esp_event.h>
#include <OneWire.h>

#ifndef TIMER_LOOP_PERIOD_THERMOMETERS
#define TIMER_LOOP_PERIOD_THERMOMETERS 15 * 1000
#endif

#ifndef DEFAULT_RESOLUTION
#define DEFAULT_RESOLUTION 10
#endif

#define SIZE_OF_ADDRESS_PRINTED (sizeof("00:00:00:00:00:00:00:00") - 1)
#define LENGTH_OF_NAME 32

ESP_EVENT_DECLARE_BASE(ONEWIRE_EVENT);
typedef enum
{
    ONEWIRE_EVENT_THERMOMETER,
    ONEWIRE_EVENT_TEMPERATURE
} OneWireEvent;

typedef enum
{
    UNIT_OK,
    UNIT_CRC_ERROR
} UnitError;

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
    char Name[LENGTH_OF_NAME];
    char OldName[LENGTH_OF_NAME];
    UnitError ErrorCode;
    Address1Wire Address;
    byte Pin;
    ChangesEvent Event;
} ThermometerEvent;

typedef struct
{
    char Name[LENGTH_OF_NAME];
    double Temperature;
} TemperatureEvent;

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


class Async1WireMgr
{
public:
    /// @brief Default constructor.
    /// @details No activities here. Just initialize variables.
    /// @param loop - event loop handle. NULL - default loop is used
    Async1WireMgr(esp_event_loop_handle_t loop = NULL);

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

    /// @brief Parse string to OneWire address.
    /// @details This method parses string to OneWire address. String can be in any format, like
    ///          28-3c-01-4b-06-00-00-7f
    ///          283c014b0600007f
    ///          and even 28:3c-014b+06/00-00:7f
    /// @param addr
    /// @return
    static Address1Wire ParseAddress(const char *addrStr);

private:
    bool isInitialized = false;
    static char addrPrinted[SIZE_OF_ADDRESS_PRINTED + 1];
    ulong temperatureTimerInterval = TIMER_LOOP_PERIOD_THERMOMETERS;
    std::map<byte, OneWire *> oneWireCollection;
    std::map<String, Thermometer *> thermometers;
    esp_event_loop_handle_t eventLoop;

    StaticTimer_t temperatureLoopBuffer;
    TimerHandle_t temperatureLoopTimer;

    Thermometer *getThermometer(Address1Wire addr);
    void notifyThermometerChanges(ThermometerEvent *t);
    void notifyTemperatureChanges(TemperatureEvent *t);
    static void onTemperatureLoopTimer(TimerHandle_t xTimer);
    void requestTemperature();
};

extern Async1WireMgr OneWireMgr;