#include "RaspberryPiControl.h"

RaspberryPiControl::RaspberryPiControl(void)
{
    //Constructor
    state = WAIT_STATUS_ON_STATE;
    timeoutOn = 0;
    timeoutOff = 0;
}

void RaspberryPiControl::powerHandler(DigitalIn *raspberryPiStatus, 
                                      DigitalOut *powerEnable5V,
                                      uint32_t onThreshold,
                                      uint32_t offThreshold)
{
    switch(state)
    {
        case IDLE_STATE:
            //Check if we need to wakeup RaspberryPi
            timeoutOff++;
            if (timeoutOff >= offThreshold)
            {
                timeoutOff = 0;
                // Turn OFF RaspberryPi and set on threshold value
                //Turn ON 5V power supply
                powerEnable5V->write(1);
                state = WAIT_STATUS_ON_STATE;
            }

            break;

        case WAIT_STATUS_ON_STATE:
            //Wait when RaspberryPi pulls up STATUS pin
            //NOTE: Temporarely check reversed logic
            if (raspberryPiStatus->read())
            {
                //Add some timeout also
                state = WAKEUP_STATE;
            }

            break;

        case WAKEUP_STATE:
            //Send sensor data and when wakeup period set, shutdown
            //In order for timeout to work, this function should be executed every 1s

            //Check status pin and then turn off power supply.
            //Or after timeout, turn off power supply anyway without waiting for status.
            timeoutOn++;

            if ((!raspberryPiStatus->read()) || (timeoutOn >= onThreshold))
            {
                //Turn Off 5V power supply
                powerEnable5V->write(0);
                //Reset timeout counter
                timeoutOn = 0;
                state = IDLE_STATE;
            }

            //Still needed to handle REBOOT state of RPi

            break;

        default:
            break;
    }
}

