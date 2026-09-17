#include "MKL46Z4.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define GREEN_LED_ON         GPIOD->PCOR |= (1 << 5)
#define GREEN_LED_OFF        GPIOD->PSOR |= (1 << 5)
#define RED_LED_ON           GPIOE->PCOR |= (1 << 29)
#define RED_LED_OFF          GPIOE->PSOR |= (1 << 29)
#define IS_SW1_NHAN          (0 == (GPIOC->PDIR & 0x8))
#define IS_SW2_NHAN          (0 == (GPIOC->PDIR & 0x1000))
#define DELAY_COUNT          1660000UL
#define RED_LED_TOGGLE       GPIOE->PTOR |= (1 << 29)
#define GREEN_LED_TOGGLE     GPIOD->PTOR |= (1 << 5)
#define SW1_STATUS           (GPIOC -> PDIR & 0x8)
#define SW2_STATUS           (GPIOC -> PDIR & 0x1000)
#define BUTTON_IS_PRESS      (0)
#define Load_Value           10500000

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

void initPit(uint32_t u32_Load_Hz);
void initLed();
void delay_time(unsigned long time);
void GREEN_Toggle(uint32_t u32_delay);
uint32_t u32_Load_Hz = Load_Value * 5;
unsigned long delay = DELAY_COUNT * 2;

/*******************************************************************************
 * Code
 ******************************************************************************/

void main()
{
    initLed();
    initPit(u32_Load_Hz);
    while(1)
    {
        GREEN_Toggle(delay);
    }
}
void GREEN_Toggle(uint32_t u32_delay)
{
    GREEN_LED_TOGGLE;
    delay_time(u32_delay);
}


void PIT_IRQHandler (void) /*ngat cua pit*/
{
    if(PIT->CHANNEL[0].TFLG & 1)
    {
        PIT->CHANNEL[0].TFLG |= 1;
        RED_LED_TOGGLE;
    }
}

void PORTC_PORTD_IRQHandler(void)/*ngắt Port*/
{
    if((PORTC->PCR[3] & (1<<24)) == (1<<24))
    {
    /* dao động nội 4Mhz/32Khz */
        MCG->SC &= ~(14);    /*MCG_SC_FCRDIV*/
        MCG->C2 |= 1;        /*MCG_C2_IRCS*/
        MCG->C1 &= ~(1<<6);  /* clear MCG_C1_CLKS*/
        MCG->C1 |= (1<<6);   /*MCG_C1_CLKS select internal clock*/
        delay = (DELAY_COUNT * 4 / 21) * 5;
        PIT->CHANNEL[0].TCTRL |= 0;
        PIT->CHANNEL[0].LDVAL = 4000000 - 1;
        PIT->CHANNEL[0].TCTRL |= 1;
        PORTC->PCR[3] |= 1 << 24;
    }
    if((PORTC->PCR[12] & (1<<24)) == (1<<24))
    {
        /*dao động thạch anh 8Mhz*/
        MCG->C2 |= MCG_C2_RANGE0(1);
        MCG->C2 |= MCG_C2_EREFS0(1);
        MCG->C1 &= ~(1<<6);
        MCG->C1 |= MCG_C1_CLKS(2);
        delay = (DELAY_COUNT * 8/21) * 3;
        PIT->CHANNEL[0].TCTRL |= 0;
        PIT->CHANNEL[0].LDVAL = 28000000 - 1;
        PIT->CHANNEL[0].TCTRL |= 1;
        PORTC->PCR[12] |= 1 << 24;
    }
}

void delay_time(unsigned long time)
{
    unsigned long i;
    for (i = 0; i < time ; i++)
    {
        __asm("nop");
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
    /*cau hinh ngat cho port*/
    PORTC->PCR[3] |= 10 << 16;
    PORTC->PCR[12] |= 10 << 16;
    NVIC_EnableIRQ(PORTC_PORTD_IRQn);
}
void initPit(uint32_t u32_Load_Hz)
{
    SIM->SCGC6 |= (1 << 23);          /* Set bit timer (PIT)*/
    PIT->MCR &= ~ (1<<1);             /* Set bit 2 ve 0*/
    /*timer 0*/
    PIT->CHANNEL[0].LDVAL = u32_Load_Hz - 1; /*set tan so pit, CPU 21MHz*/
    PIT->CHANNEL[0].TCTRL |= 1<<1; /*cho phep ngat tu pit*/
    NVIC_EnableIRQ(PIT_IRQn);
    PIT->CHANNEL[0].TCTRL |= 1;      /* khoi dong timer nen de cuoi sau khi setup*/
}
