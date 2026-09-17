#include "MKL46Z4.h"
#include "stdbool.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define GREEN_LED_ON         GPIOD->PCOR |= (1 << 5)
#define GREEN_LED_OFF        GPIOD->PSOR |= (1 << 5)
#define RED_LED_ON           GPIOE->PCOR |= (1 << 29)
#define RED_LED_OFF          GPIOE->PSOR |= (1 << 29)
#define IS_SW1_NHAN          (0 == (GPIOC->PDIR & 0x8))
#define IS_SW2_NHAN          (0 == (GPIOC->PDIR & 0x1000))
#define RED_LED_TOGGLE       GPIOE->PTOR |= (1 << 29)
#define GREEN_LED_TOGGLE     GPIOD->PTOR |= (1 << 5)
#define SW1_STATUS           (GPIOC -> PDIR & 0x8)
#define SW2_STATUS           (GPIOC -> PDIR & 0x1000)
#define BUTTON_IS_PRESS      (0)
#define DELAY_COUNT          1410000UL

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

void initLed();
void mode1(uint32_t state_led, unsigned char doSang);
void process_button1(unsigned char *doSang);
void process_button2(unsigned char *doSang);
void delay100us(unsigned int delay_count);

/*******************************************************************************
 * Variables
 ******************************************************************************/

uint32_t state_led = 0;

/*******************************************************************************
 * Code
 ******************************************************************************/

void main()
{
    static unsigned char doSang;
    initLed();
    while(1)
    {
        /*dinh thoi co dinh vong lap 10ms*/
        process_button1(&doSang);
        process_button2(&doSang);
        mode1(state_led, doSang);/*10ms*/
    }
}

void mode1(uint32_t state_led, unsigned char doSang)
{
    if((state_led == 1)&&(doSang != 0))
    {
        RED_LED_ON;
    }
       else if((state_led == 2)&&(doSang != 0))
    {
        GREEN_LED_ON;
    }
    else if((state_led == 3)&&(doSang != 0))
    {
        RED_LED_ON;
        GREEN_LED_ON;
    }

    delay100us(doSang);
    if (state_led == 1 && doSang != 0) {
        RED_LED_OFF;
    } else if (state_led == 2 && doSang != 0) {
        GREEN_LED_OFF;
    } else if (state_led == 3 && doSang != 0) {
        RED_LED_OFF;
        GREEN_LED_OFF;
    }
    if (doSang != 100)
    {
        RED_LED_OFF;
        GREEN_LED_OFF;
    }
    delay100us(100 - doSang);

}

void process_button1(unsigned char *doSang)
{
    static bool check_led = false;
    uint8_t CurrentSw1 = SW1_STATUS;
    static uint8_t PrevSw1;
    static uint32_t time = 0;
    static bool time_long = false;
    static uint8_t state_led_current = 0;
    static bool demBrightness = false;
    /*nhan nhanh*/
    if((CurrentSw1 != BUTTON_IS_PRESS) && (PrevSw1 == BUTTON_IS_PRESS) && (time < 100))
    {
        if(check_led == true)
        {
            state_led++;
            if(state_led > 3)
            {
                state_led = 1;
            }
        }
        time_long = false;
        time = 0;

    }

    if(time == 100)
    {
        if(check_led == true)
        {
            state_led_current = state_led;
            state_led = 0;
            check_led = false;
        }
        else
        {
            state_led = state_led_current;
            check_led = true;
        }
        demBrightness = true;
    }

    /*nhan giu*/
    if((CurrentSw1 != BUTTON_IS_PRESS) && (PrevSw1 == BUTTON_IS_PRESS) && (time > 100))
    {
        time_long = false;
        time = 0;
    }
    /*bat dau nhan*/
    if((CurrentSw1 == BUTTON_IS_PRESS) && (PrevSw1 != BUTTON_IS_PRESS))
    {
        time_long = true;
    }
    PrevSw1 = CurrentSw1;
    /*kiem tra bo dem thoi gian*/
    if(time_long == true)
    {
        time++;
    }
    if (demBrightness == true)
    {
        *doSang = 50;
        demBrightness = false;
    }
}



void process_button2(unsigned char *doSang)
{
    static uint32_t time = 0;
    static bool Check_time = false;
    uint32_t CurrentSw2 = SW2_STATUS;
    static uint32_t PrevSw2;

    static bool Max_doSang = false;
    static bool flag = false;
    static uint32_t dem = 0;
    /*nhan nhanh*/
    if((CurrentSw2 != BUTTON_IS_PRESS) && (PrevSw2 == BUTTON_IS_PRESS) && (time < 100))
    {
        if (Max_doSang == false)
        {
            *doSang += 25;
            if (*doSang >= 100) {
                *doSang = 100;
                Max_doSang = true;
            }
        }
        else
        {
            *doSang -= 25;
            if(*doSang <= 0)
            {
                *doSang = 0;
                Max_doSang = false;
            }
        }
        Check_time = false;
        time = 0;
    }
    /*bat dau nhan giu*/
    if(time == 100)
    {
        flag = true;
    }
    /*nhan giu*/
    if((CurrentSw2 != BUTTON_IS_PRESS) && (PrevSw2 == BUTTON_IS_PRESS) && (time > 100))
    {
        Check_time = false;
        time = 0;
        flag = false;
        dem = 0;
    }
    /*bat dau nhan*/
    if((CurrentSw2 == BUTTON_IS_PRESS) && (PrevSw2 != BUTTON_IS_PRESS))
    {
        Check_time = true;
    }
    PrevSw2 = CurrentSw2;
    if(flag == true)
    {
        dem++;
        if(dem == 4)
        {
            if (Max_doSang == false)
            {
                *doSang += 1;
                if(*doSang >= 100)
                {
                    *doSang = 100;
                    Check_time = true;
                    flag = false;
                }
            }
            else
            {
                *doSang -= 1;
                if(*doSang <= 0)
                {
                    *doSang = 0;
                    Check_time = false;
                    flag = false;
                }
            }
            dem = 0;
        }
    }
    else
    {
        dem = 0;
    }
    /*kiem tra bo dem thoi gian*/
    if(Check_time == true)
    {
        time++;
    }
    else
    {
        time = 0;
    }

}

void initLed()
{
    /* Enable clock for PORTC, PORTD, PORTE */
    SIM->SCGC5 |= (1 << 11);    /* Set bit 11 */
    SIM->SCGC5 |= (1 << 12);    /* Set bit 12 */
    SIM->SCGC5 |= (1 << 13);    /* Set bit 13 */
    /* Configure multiplex of PTC3 and PTC12 as GPIO */
    PORTC->PCR[12] |= PORT_PCR_MUX(1);
    PORTC->PCR[3] |= PORT_PCR_MUX(1);
    /* Configure multiplex of PTD5 and PTE29 as GPIO */
    PORTD->PCR[5] |= PORT_PCR_MUX(1);
    PORTE->PCR[29] |= PORT_PCR_MUX(1);
    /* Configure PTC3 and PTC12 as input */
    GPIOC->PDDR &= ~(1 << 3);
    GPIOC->PDDR &= ~(1 << 12);
    /* Configure PTD5 and PTE29 as output */
    GPIOD->PDDR |= (1 << 5);
    GPIOE->PDDR |= (1 << 29);
    /*cau hinh tro treo cho nut bam*/
    PORTC->PCR[12] |= 3;
    PORTC->PCR[3] |= 3;
    /* Clear PTD5 and PTE29 */
    RED_LED_OFF;
    GREEN_LED_OFF;
}

void delay100us(unsigned int delay_count)
{
    unsigned long i;

    for (i = 0; i < (DELAY_COUNT/10000) * delay_count; i++)
    {
        __asm("nop");
    }
}
