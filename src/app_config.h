#ifndef _APP_CONFIG_H
#define _APP_CONFIG_H

#include "mbed.h"

#define TARGET_DEFAULT          0    /*< For any board which uses standard MBED connection */
#define TARGET_RBLABBLENANO     1    /*< RedBearLab BLE Nano board */
#define TARGET_SEEEDTINYBLE     2    /*< Seeed Tiny BLE board */
#define TARGET_PIRA_SMART       3    /*< Pira Smart board */

//#define TARGET                  TARGET_RBLABBLENANO
#define TARGET                  TARGET_PIRA_SMART

#if (TARGET == TARGET_DEFAULT)
   
    #define LED_ON      0
    #define LED_OFF     1

    #define LED_1       LED1 
    #define LED_2       LED2
    #define LED_3       LED3
    #define LED_4       LED4

    #define UART_TX     USBTX
    #define UART_RX     USBRX

#elif (TARGET == TARGET_RBLABBLENANO)
    
    #define LED_ON      0
    #define LED_OFF     1

    #define LED_1       p23
    #define LED_2       p22
    #define LED_3       p21

    #define BUTTON_1    p16
    #define BUTTON_2    p17
    #define BUTTON_3    p18

    #define UART_TX     p9
    #define UART_RX     p11

    #define I2C_SDA     p0
    #define I2C_SCL     p1

    #define ENABLE_3V3_PIN  p21
    #define ENABLE_5V_PIN   p19

#elif (TARGET == TARGET_PIRA_SMART)
    
    #define DEBUG

    #define LED_ON      1
    #define LED_OFF     0

    #define LED_1       p26
    #define LED_2       p27

    #define UART_TX     p19
    #define UART_RX     p18

    #define I2C_SDA     p3
    #define I2C_SCL     p2

    #define ENABLE_3V3_PIN  p1
    #define ENABLE_5V_PIN   p25

    #define RASPBERRY_PI_STATUS     p5  //GPIO_BCM17 from RaspberryPi

#elif (TARGET == TARGET_SEEEDTINYBLE) 

    #define LED_ON      0
    #define LED_OFF     1

    #define LED_1       LED1 
    #define LED_2       LED2
    #define LED_3       LED3
    #define LED_4       LED4

    #define BUTTON_1    p17

    #define UART_TX     p3
    #define UART_RX     p4

#else
    #error Target not defined correctly.
#endif


#endif /* _APP_CONFIG_H */
