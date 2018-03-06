#include "mbed.h"

#define LED_GREEN   p21
#define LED_RED     p22
#define LED_BLUE    p23
#define BUTTON_PIN  p17

//#define UART_TX     p9
#define UART_TX     p3
//#define UART_RX     p11
#define UART_RX     p4

DigitalOut led1(LED1);
InterruptIn button1(BUTTON1);

//Serial pc(USBTX, USBRX);
Serial pc(UART_TX, UART_RX);

void button1_detect(void)
{
    pc.printf("Button pressed...\n");
}

int main() 
{
    //Initialize variables
    pc.baud(115200);
    pc.printf("Start...\n");

    button1.fall(button1_detect);
    led1 = 0;

    while(1) 
    {
        wait(0.5);
        led1 = 1;
        wait(0.5);
        led1 = 0;
    }
}
