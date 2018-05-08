/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#ifndef __BLE_PIRA_SERVICE_H__
#define __BLE_PIRA_SERVICE_H__

#include "string.h"

class PiraService {
public:
    const static uint16_t PIRA_SERVICE_UUID                   = 0xB000;
    const static uint16_t PIRA_SET_TIME_CHARACTERISTIC_UUID   = 0xB001;
    const static uint16_t PIRA_GET_TIME_CHARACTERISTIC_UUID   = 0xB002;
    const static uint16_t PIRA_STATUS_CHARACTERISTIC_UUID     = 0xB003;
    const static uint16_t PIRA_ON_PERIOD_CHARACTERISTIC_UUID  = 0xB004;
    const static uint16_t PIRA_OFF_PERIOD_CHARACTERISTIC_UUID = 0xB005;
 
    PiraService(BLEDevice &_ble, 
                uint32_t  piraSetTimeInitValue   = 0,
                uint32_t  piraGetStatusInitValue = 0,
                char      *piraGetTimeInitValue  = NULL,
                uint32_t  piraOnPeriodInitValue  = 0,
                uint32_t  piraOffPeriodInitValue = 0) :
        ble(_ble), 
        setTime(PIRA_SET_TIME_CHARACTERISTIC_UUID, &piraSetTimeInitValue),
        getTime(PIRA_GET_TIME_CHARACTERISTIC_UUID, 
                (uint8_t *)piraGetTimeInitValue,
                (piraGetTimeInitValue != NULL) ? strlen((const char *)piraGetTimeInitValue) : 0,
                (piraGetTimeInitValue != NULL) ? strlen((const char *)piraGetTimeInitValue) : 0,
                GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ),
        getStatus(PIRA_STATUS_CHARACTERISTIC_UUID, &piraGetStatusInitValue),
        onPeriodSeconds(PIRA_ON_PERIOD_CHARACTERISTIC_UUID, &piraOnPeriodInitValue),
        offPeriodSeconds(PIRA_OFF_PERIOD_CHARACTERISTIC_UUID, &piraOffPeriodInitValue)    
    {
        GattCharacteristic *charTable[] = {&setTime, &getTime, &getStatus, &onPeriodSeconds, &offPeriodSeconds};
        GattService         piraService(PIRA_SERVICE_UUID, charTable, sizeof(charTable) / sizeof(GattCharacteristic *));
        ble.addService(piraService);
    }
 
    GattAttribute::Handle_t getSetTimeValueHandle() const 
    {
        return setTime.getValueHandle();
    }

    void updateTime(char *time)
    {
        ble.gattServer().write(getTime.getValueHandle(), (const uint8_t *)time, strlen((const char *)time));
    }

    void updateStatus(uint32_t *status)
    {
        ble.gattServer().write(getStatus.getValueHandle(), (const uint8_t *)status, sizeof(uint32_t));
    }

    GattAttribute::Handle_t getOnPeriodSecondsValueHandle() const 
    {
        return onPeriodSeconds.getValueHandle();
    }

    GattAttribute::Handle_t getOffPeriodSecondsValueHandle() const 
    {
        return offPeriodSeconds.getValueHandle();
    }

private:
    BLEDevice                             &ble;
    WriteOnlyGattCharacteristic<uint32_t> setTime;
    GattCharacteristic                    getTime;
    ReadOnlyGattCharacteristic<uint32_t>  getStatus;
    // onPeriod is amout of time RPi needs to be awake before going to sleep
    WriteOnlyGattCharacteristic<uint32_t> onPeriodSeconds;
    // offPeriod is amount of time RPi needs to sleep for before next wakeup
    WriteOnlyGattCharacteristic<uint32_t> offPeriodSeconds;
};
 
#endif /* #ifndef __BLE_PIRA_SERVICE_H__ */
