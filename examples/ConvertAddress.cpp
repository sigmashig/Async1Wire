#include <Arduino.h>
#include "Async1WireMgr.hpp"

void setup()
{
    Serial.begin(115200);
    Serial.println("Start");
    char* addrStr = "28-ff:4c_7c+0b:16,04a0"; // strange but it is possible
    Address1Wire addr;
    addr = Async1WireMgr::ParseAddress(addrStr);
    Serial.printf("Convert string: %s to address: %s \n", addrStr, Async1WireMgr::PrintAddress(addr));
}


void loop()
{
    vTaskDelete(NULL);
}