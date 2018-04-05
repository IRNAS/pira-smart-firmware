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
 
#include "mbed.h"
#include "ble/BLE.h"
#include "LEDService.h"
#include "app_config.h"
#include "ISL1208.h"
 
DigitalOut alivenessLED(LED_1, 0);
DigitalOut actuatedLED(LED_3, 1);

// Create UART object
Serial pc(UART_TX, UART_RX);
// Create I2C object
I2C i2c(I2C_SDA, I2C_SCL);
// Create ISL1208 object
ISL1208 rtc(&i2c);    
 
const static char     DEVICE_NAME[] = "LED";
static const uint16_t uuid16_list[] = {LEDService::LED_SERVICE_UUID};
 
LEDService *ledServicePtr;
 
Ticker ticker;

uint8_t send_time; 

void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *params)
{
    BLE::Instance().gap().startAdvertising();
}
 
void periodicCallback(void)
{
    alivenessLED = !alivenessLED; /* Do blinky on LED1 to indicate system aliveness. */
    
    send_time = 1; 
    //Get the current time
    //time_t seconds = rtc.time();
    //pc.printf("Test\n");
    //Print the time in various formats
    //pc.printf("Time as a basic string = %s", ctime(&seconds));
    //char buffer[32];
    //strftime(buffer, 32, "%I:%M %p\n", localtime(&seconds));
    //pc.printf("Time as a custom formatted string = %s", buffer);
}
 
/**
 * This callback allows the LEDService to receive updates to the ledState Characteristic.
 *
 * @param[in] params
 *     Information about the characterisitc being updated.
 */
void onDataWrittenCallback(const GattWriteCallbackParams *params) {
    if ((params->handle == ledServicePtr->getValueHandle()) && (params->len == 1)) {
        actuatedLED = *(params->data);
    }
}
 
/**
 * This function is called when the ble initialization process has failed
 */
void onBleInitError(BLE &ble, ble_error_t error)
{
    /* Initialization error handling should go here */
}
 
/**
 * Callback triggered when the ble initialization process has finished
 */
void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    BLE&        ble   = params->ble;
    ble_error_t error = params->error;
 
    if (error != BLE_ERROR_NONE) {
        /* In case of error, forward the error handling to onBleInitError */
        onBleInitError(ble, error);
        return;
    }
 
    /* Ensure that it is the default instance of BLE */
    if(ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
        return;
    }
 
    ble.gap().onDisconnection(disconnectionCallback);
    ble.gattServer().onDataWritten(onDataWrittenCallback);
 
    bool initialValueForLEDCharacteristic = false;
    ledServicePtr = new LEDService(ble, initialValueForLEDCharacteristic);
 
    /* setup advertising */
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.gap().setAdvertisingInterval(1000); /* 1000ms. */
    ble.gap().startAdvertising();
}

void init_uart(void)
{
    pc.baud(115200);
    pc.printf("Start...\n");
}

void init_rtc(void)
{
    i2c.frequency(400000); //400kHz
    
    //Try to open the ISL1208
    if (rtc.open()) 
    {
        pc.printf("Device detected!\n");
 
        //Configure the oscillator for a 32.768kHz crystal
        rtc.oscillatorMode(ISL1208::OSCILLATOR_CRYSTAL);
 
        //Check if we need to reset the time
        if (rtc.powerFailed()) 
        {
            //The time has been lost due to a power complete power failure
            pc.printf("Device has lost power! Resetting time...\n");

            //Set RTC time to Wed, 28 Oct 2009 11:35:37
            rtc.time(1522758120);
        }
    }
    else
    {
        pc.printf("Device NOT detected!\n");
    }
}
 
int main(void)
{
    send_time = 0;
    // UART needs to be initialized first to use it for debugging
    init_uart();

    // I2C and RTC init
    init_rtc();
 
    // periodicCallback must be attached after I2C is initialized
    ticker.attach(periodicCallback, 1); /* Blink LED every second */

    BLE &ble = BLE::Instance();
    ble.init(bleInitComplete);
 
    /* SpinWait for initialization to complete. This is necessary because the
     * BLE object is used in the main loop below. */
    while (ble.hasInitialized()  == false) { /* spin loop */ }
 
    while (true) {
        ble.waitForEvent();

        if (send_time)
        {
            send_time = 0;

            //Get the current time
            time_t seconds = rtc.time();
            //Print the time in various formats
            pc.printf("Time as a basic string = %s", ctime(&seconds));
            //char buffer[32];
            //strftime(buffer, 32, "%I:%M %p\n", localtime(&seconds));
            //pc.printf("Time as a custom formatted string = %s", buffer);
        }
    }
}
