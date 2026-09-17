#include "MKL46Z4.h"

#define GREEN_LED_ON      GPIOD->PCOR |= (1 << 5)
#define GREEN_LED_OFF     GPIOD->PSOR |= (1 << 5)
#define RED_LED_ON    GPIOE->PCOR |= (1 << 29)
#define RED_LED_OFF   GPIOE->PSOR |= (1 << 29)
#define MAX_BRIGHTNESS    100
#define DELAY_COUNT     1409000UL

void initLed();
void delay();
void delay100us(unsigned int delay_us);
void led(unsigned int doSang);
void toogle_LED(int t_led);
void fade_LED();

void main()
{
    initLed();
    while(1)
    {
        int count = 0;
        while(count < 5)
            {
             fade_LED();
             count++;
            }
        count = 0;
        while(count < 5)
            {
            toogle_LED(5000);
            count++;
            }
    }

}
void fade_LED()
{
    int i;
    for(i=0;i<MAX_BRIGHTNESS;i+=2)
    {
        led(i);
    }
    for(i=MAX_BRIGHTNESS;i>0;i-=2)
    {
        led(i);
    }
}
void led(unsigned int doSang)
{
        /* Set PTD5 and PTE29 */
        RED_LED_ON;
        GREEN_LED_ON;
        /* Delay */
        delay100us(doSang);
        /* Clear PTD5 and PTE29 */
        RED_LED_OFF;
        GREEN_LED_OFF;
        /* Delay */
delay100us(MAX_BRIGHTNESS - doSang);

}
void toogle_LED(int t_led){
    RED_LED_ON;
    GREEN_LED_ON;
    /* Delay */
    delay100us(t_led);
    /* Clear PTD5 and PTE29 */
    RED_LED_OFF;
    GREEN_LED_OFF;
    /* Delay */
    delay100us(t_led);
}

void initLed()
{
    /* Enable clock for PORTD, PORTE */
    SIM->SCGC5 |= (1 << 12);    /* Set bit 12 */
    SIM->SCGC5 |= (1 << 13);    /* Set bit 13 */

    /* Configure multiplex of PTD5 and PTE29 as GPIO */
    PORTD->PCR[5] |= PORT_PCR_MUX(1);
    PORTE->PCR[29] |= PORT_PCR_MUX(1);

    /* Configure PTD5 and PTE29 as output */
    GPIOD->PDDR |= (1 << 5);
    GPIOE->PDDR |= (1 << 29);

    /* Clear PTD5 and PTE29 */
    RED_LED_ON;
    GREEN_LED_ON;
}

void delayms(unsigned int delay_ms)
{
    unsigned long i;

    for (i = 0; i < (DELAY_COUNT/1000) * delay_ms; i++)
    {
        __asm("nop");
    }
}
void delay100us(unsigned int delay_us)
{
    unsigned long i;

    for (i = 0; i < (DELAY_COUNT/10000) * delay_us; i++)
    {
        __asm("nop");
    }
}
void delay()
{
    unsigned long i;

    for (i = 0; i < DELAY_COUNT; i++)
    {
        __asm("nop");
    }
}
