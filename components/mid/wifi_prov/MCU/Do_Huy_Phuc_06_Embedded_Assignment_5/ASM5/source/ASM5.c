/************Include Library**********************/
#include "MKL46Z4.h"
/************Marco defeniton**********************/
#define GREEN_LED_ON         GPIOD->PCOR |= (1 << 5)
#define GREEN_LED_OFF        GPIOD->PSOR |= (1 << 5)
#define RED_LED_ON           GPIOE->PCOR |= (1 << 29)
#define RED_LED_OFF          GPIOE->PSOR |= (1 << 29)
#define IS_SW1_NHAN          (0 == (GPIOC->PDIR & 0x8))
#define IS_SW2_NHAN          (0 == (GPIOC->PDIR & 0x1000))
#define DELAY_COUNT          1410000UL

/************************Function code************************/
void initLed();
void delay100us(uint8_t delay_count);
void Toggle_Leds(uint8_t Brightness_Red, uint8_t Brightness_Green);
uint8_t Brightness_Green = 0;
uint8_t Brightness_Red = 0;
void main()
{
    initLed();
    PORTC->PCR[3] |= 10 << 16;
    PORTC->PCR[12] |= 10 << 16;
    NVIC_EnableIRQ(PORTC_PORTD_IRQn);

    while (1)
    {
        Toggle_Leds(Brightness_Red, Brightness_Green);
    }
}
void PORTC_PORTD_IRQHandler(void)
{
    if((PORTC->PCR[3] & (1<<24)) == (1<<24))
    {
        if(Brightness_Red == 100)
        {
            Brightness_Red = 0;
            RED_LED_OFF;

        }
        else
        {
            Brightness_Red+=10;
        }
        PORTC->PCR[3] |= 1 << 24;/*down flag*/
    }
    if((PORTC->PCR[12] & (1<<24)) == (1<<24))
    {
        if(Brightness_Green == 100)
        {
            Brightness_Green = 0;
            GREEN_LED_OFF;
        }
        else
        {
            Brightness_Green+=10;
        }
        PORTC->PCR[12] |= 1 << 24;/*down flag*/
    }
}

void Toggle_Leds(uint8_t Brightness_Red, uint8_t Brightness_Green) {
     if (Brightness_Green >= Brightness_Red) {
         if (Brightness_Red != 0) {
             RED_LED_ON;
         }
         if (Brightness_Green != 0) {
             GREEN_LED_ON;
         }
        delay100us(Brightness_Red);
        if(Brightness_Red != 100) {
            RED_LED_OFF;
        }
        delay100us(Brightness_Green - Brightness_Red);
        if (Brightness_Green != 100) {
            GREEN_LED_OFF;
        }
        delay100us(100 - Brightness_Green);
    } else {
        if (Brightness_Red != 0) {
            RED_LED_ON;
        }
        if (Brightness_Green != 0) {
            GREEN_LED_ON;
        }
        delay100us(Brightness_Green);
        if(Brightness_Green != 100) {
            GREEN_LED_OFF;
        }
        delay100us(Brightness_Red - Brightness_Green);
        if (Brightness_Red != 100) {
            RED_LED_OFF;
        }
        delay100us(100 - Brightness_Red);
    }
}

void initLed()
{
    /* Enable clock for PORTC, PORTD, PORTE cho phep thanh ghi truy cap*/
    SIM->SCGC5 |= (1 << 11);    /* Set bit 11 */
    SIM->SCGC5 |= (1 << 12);    /* Set bit 12 */
    SIM->SCGC5 |= (1 << 13);    /* Set bit 13 */
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

void delay100us(uint8_t delay_count)
{
    unsigned long i;
    for (i = 0; i < (DELAY_COUNT/10000) * delay_count; i++)
    {
        __asm("nop");
    }
}
