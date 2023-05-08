# Async1Wire Library

This library is a wrapper around the [OneWire] and [DallasTemperature] libraries. It provides an asynchronous interface to the OneWire bus. 
Support DS1820 and DS18B20.

IMPORTANT NOTE!
This library using an FreeRTOS functionality for ESP32. It will not work on other platforms.
This library is in beta version and can be changed. Use it on own risk!

2023-05-05 v0.1.0 - First Release

2023-05-06 v0.2.0 - Documenting
                    + SetTemperatureTimerInterval

2023-05-07 v0.2.2 - Improve connection lost detection
                    ! Change onThermometerChange parameter type
                    ! Change onThermometerChange will fired for connection lost/restored, added, renamed
                    PLEASE NOTE! This change is not compatible with previous version
                    PLEASE NOTE! Callback functions are blocked. Please use them only for short operations.
                    
This Branch is over due to the fact that callback functions are blocked functionality and can't be widely used. The new version will be released soon.
