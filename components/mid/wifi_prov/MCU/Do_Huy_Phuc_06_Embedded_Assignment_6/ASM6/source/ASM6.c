#include <myMKL64Z.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define GREEN_LED_ON      GPIOD->PCOR |= (1 << 5)
#define GREEN_LED_OFF     GPIOD->PSOR |= (1 << 5)
#define GREEN_LED_TOGGLE  ((GPIO_Type *)0x400FF0C0u)->PTOR |= (1 << 5)
#define GREEN_LED_TOGGLE_BY_ADDRESS   ((*(unsigned int *)0x400FF0CC) |= (1<<5))

#define RED_LED_ON    GPIOE->PCOR |= (1 << 29)
#define RED_LED_OFF   GPIOE->PSOR |= (1 << 29)
#define RED_LED_TOGGLE   GPIOE->PTOR |= (1 << 29)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
void initLed();

/*******************************************************************************
 * Code
 ******************************************************************************/
void main()
{
    initLed();
    SIM->SCGC6 |= 1 << 23; /* bat clock cho PIT */
    PIT->MCR &= ~(1 << 1);
    PIT->CHANNEL[0].LDVAL = 10500000;
    PIT->CHANNEL[0].TCTRL |= 1;/* cho phep timer chay */
    PIT->CHANNEL[0].TCTRL |= 1 << 1;

    PIT->CHANNEL[1].LDVAL = 9;

    PIT->CHANNEL[1].TCTRL |= 1 ;/* cho phep timer chay */
    PIT->CHANNEL[1].TCTRL |= 1 << 1;
    PIT->CHANNEL[1].TCTRL |= 1 << 2;

/* nên cho timer chạy ở dau cùng sau khi đa cấu hình xong xuôi */

    NVIC_EnableIRQ(PIT_IRQn); /* cho phep ngat tu NVIC vao CPU */


    while (1)
    {


    }
}

void PIT_IRQHandler(void)
{
    if(PIT->CHANNEL[0].TFLG & 1)
    {
        RED_LED_TOGGLE;
        PIT->CHANNEL[0].TFLG |= 1;
    }
    if(PIT->CHANNEL[1].TFLG & 1)
    {
        GREEN_LED_TOGGLE;
        PIT->CHANNEL[1].TFLG |= 1;
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
