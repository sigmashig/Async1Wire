# Async1Wire Library

This library is a wrapper around the [OneWire] and [DallasTemperature] libraries. It provides an asynchronous interface to the OneWire bus. 
Support DS1820 and DS18B20.

This library using an FreeRTOS functionality for ESP32. It will not work on other platforms.

2023-05-05 v0.1.0 - First Release
2023-05-06 v0.2.0 - Documenting
                    + SetTemperatureTimerInterval