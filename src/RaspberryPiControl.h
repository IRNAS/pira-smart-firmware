#ifndef RASPBERRY_PI_CONTROL_H
#define RASPBERRY_PI_CONTROL_H

#include "mbed.h"

class RaspberryPiControl 
{
public:

    enum ControlState
    {
        IDLE_STATE             = 0,
        WAIT_STATUS_ON_STATE   = 1,
        WAKEUP_STATE           = 2,
        WAIT_STATUS_OFF_STATE  = 3,
    };
    
    RaspberryPiControl(void);

    void powerHandler(void);
    void powerHandler(DigitalIn *raspberryPiStatus, 
                      DigitalOut *powerEnable5V,
                      uint32_t onThreshold,
                      uint32_t offThreshold);
private:
    uint8_t  state;
    uint32_t timeout;
};

#endif /* RASPBERRY_PI_CONTROL_H */
