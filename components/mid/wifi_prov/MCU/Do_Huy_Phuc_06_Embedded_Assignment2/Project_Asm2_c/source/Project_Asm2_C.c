/************Include Library**********************/
#include "MKL46Z4.h"
/************Marco defeniton**********************/
#define GREEN_LED_ON      GPIOD->PCOR |= (1 << 5)
#define GREEN_LED_OFF     GPIOD->PSOR |= (1 << 5)
#define RED_LED_ON    GPIOE->PCOR |= (1 << 29)
#define RED_LED_OFF   GPIOE->PSOR |= (1 << 29)
#define IS_SW1_NHAN         (0 == (GPIOC->PDIR & 0x8))
#define IS_SW2_NHAN         (0 == (GPIOC->PDIR & 0x1000))

/************************Function code************************/
void initLed();
void main()
{
    initLed();
    int currentSw1;
    int currentSw2;
    int prevSw2 = 0;
    int prevSw1 = 0;
    int ledSang = 1;
    int ledSang1 = 1;
    while (1)
    {
        currentSw2 = IS_SW2_NHAN;
        if (currentSw2 != prevSw2 && currentSw2 == 1) {
            if (ledSang1 == 0) {
                RED_LED_ON;
                ledSang1 = 1;
            } else 
            {
                RED_LED_OFF;
                ledSang1 = 0;
            }
        }
        prevSw2 = currentSw2;

        currentSw1 = IS_SW1_NHAN;
        if (currentSw1 != prevSw1 && currentSw1 == 1) {
            if (ledSang == 0) {
                GREEN_LED_ON;
                ledSang = 1;
            } else
            {
                GREEN_LED_OFF;
                ledSang = 0;
            }
        }
        prevSw1 = currentSw1;
    }
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
