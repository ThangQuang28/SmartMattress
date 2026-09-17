/************Include Library**********************/
#include "MKL46Z4.h"
/************Marco defeniton**********************/
#define GREEN_LED_ON      GPIOD->PCOR |= (1 << 5)
#define GREEN_LED_OFF     GPIOD->PSOR |= (1 << 5)
#define RED_LED_ON    GPIOE->PCOR |= (1 << 29)
#define RED_LED_OFF   GPIOE->PSOR |= (1 << 29)
#define IS_SW1_NHAN         (0 == (GPIOC->PDIR & 0x8))
#define IS_SW2_NHAN         (0 == (GPIOC->PDIR & 0x1000))
#define MAX_BRIGHTNESS    100
#define DELAY_COUNT     1410000UL

/************************Function code************************/
void initLed();
void delay100us(unsigned int delay_count);
void mode2(int t_led);
void mode1();
void led(unsigned int doSang);

void main()
{
    initLed();
    while (1)
    {
        if(IS_SW1_NHAN)
        {
        mode1();
        }
        if(IS_SW2_NHAN)
        {
            mode2(5000);
        }
    }
}

void mode1()
{
    int i;
    for(i = 0; i < MAX_BRIGHTNESS; i += 2)
        {
            led(i);
        }
    for(i = MAX_BRIGHTNESS; i>0 ; i-=2)
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
void mode2(int t_led){
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
    /* Enable clock for PORTC, PORTD, PORTE */
    SIM->SCGC5 |= (1 << 11);    /* Set bit 11 */
    SIM->SCGC5 |= (1 << 12);    /* Set bit 12 */
    SIM->SCGC5 |= (1 << 13);    /* Set bit 13 */

    /* Configure multiplex of PTD5 and PTE29 as GPIO */
    /* Configure multiplex of PTC3 and PTC12 as GPIO */
    PORTC->PCR[12] |= PORT_PCR_MUX(1);
    PORTC->PCR[3] |= PORT_PCR_MUX(1);
    PORTD->PCR[5] |= PORT_PCR_MUX(1);
    PORTE->PCR[29] |= PORT_PCR_MUX(1);

    /* Configure PTC3 and PTC12 as input */
    GPIOC->PDDR &= ~(1 << 3);
    GPIOC->PDDR &= ~(1 << 12);

    /* Configure PTD5 and PTE29 as output */
    GPIOD->PDDR |= (1 << 5);
    GPIOE->PDDR |= (1 << 29);

    /* cau hinh tro treo cho nut bam*/
    PORTC->PCR[12] |= 3;
    PORTC->PCR[3] |= 3;

    /* Clear PTD5 and PTE29 */
    RED_LED_ON;
    GREEN_LED_ON;
}
void delay100us(unsigned int delay_count)
{
    unsigned long i;
    for (i = 0; i < (DELAY_COUNT/10000) * delay_count; i++)
    {
        __asm("nop");
    }
}
