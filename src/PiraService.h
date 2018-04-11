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
    const static uint16_t PIRA_SERVICE_UUID                 = 0xB000;
    const static uint16_t PIRA_SET_TIME_CHARACTERISTIC_UUID = 0xB001;
    //const static uint16_t PIRA_GET_TIME_CHARACTERISTIC_UUID = 0xB002;
    //const static uint16_t PIRA_STATUS_CHARACTERISTIC_UUID   = 0xB003;
 
    PiraService(BLEDevice &_ble, 
                char *piraSetTimeInitValue = NULL) :
        ble(_ble), 
        setTime(PIRA_SET_TIME_CHARACTERISTIC_UUID, 
                (uint8_t *)piraSetTimeInitValue,
                (piraSetTimeInitValue != NULL) ? strlen((const char *)piraSetTimeInitValue) : 0,
                (piraSetTimeInitValue != NULL) ? strlen((const char *)piraSetTimeInitValue) : 0,
                GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE)
//        getTime(PIRA_GET_TIME_CHARACTERISTIC_UUID, &piraGetTimeInitValue),
//        getStatus(PIRA_STATUS_CHARACTERISTIC_UUID, &piraGetStatusInitValue)
    {
        GattCharacteristic *charTable[] = {&setTime};//, &getTime, &getStatus};
        GattService         piraService(PIRA_SERVICE_UUID, charTable, sizeof(charTable) / sizeof(GattCharacteristic *));
        ble.addService(piraService);
    }
 
    GattAttribute::Handle_t getSetTimeValueHandle() const {
        return setTime.getValueHandle();

//    GattAttribute::Handle_t getValueHandle() const {
//        return ledState.getValueHandle();
//
//    GattAttribute::Handle_t getValueHandle() const {
//        return ledState.getValueHandle();
    }
 
private:
    BLEDevice                           &ble;
    GattCharacteristic                  setTime;
    //WriteOnlyGattCharacteristic<bool>   setTime;
    //ReadOnlyGattCharacteristic<uint8_t> getTime;
    //ReadOnlyGattCharacteristic<uint8_t> getStatus;
};
 
#endif /* #ifndef __BLE_PIRA_SERVICE_H__ */
