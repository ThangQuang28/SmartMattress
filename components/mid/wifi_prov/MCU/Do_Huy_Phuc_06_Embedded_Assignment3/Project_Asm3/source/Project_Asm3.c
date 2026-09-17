/************Marco defeniton**********************/
#define GPIOD_PCOR (*((unsigned int *) 0x400FF0C8))
#define GPIOD_PSOR (*((unsigned int *) 0x400FF0C4))
#define GPIOE_PSOR (*((unsigned int *) 0x400FF104))
#define GPIOE_PCOR (*((unsigned int *) 0x400FF108))
#define GPIOC_PDIR (*((unsigned int *) 0x400FF090))
#define SIM_SCGC5  (*((unsigned int *) 0x40048038))
#define GPIOC_PDDR (*((unsigned int *) 0x400FF094))
#define GPIOD_PDDR (*((unsigned int *) 0x400FF0D4))
#define GPIOE_PDDR (*((unsigned int *) 0x400FF114))
#define PORTC_PCR12 (*((unsigned int *) 0x4004B030))
#define PORTC_PCR3 (*((unsigned int *) 0x4004B00C))
#define PORTD_PCR5 (*((unsigned int *) 0x4004C014))
#define PORTE_PCR29 (*((unsigned int *) 0x4004D074))

#define GREEN_LED_ON    GPIOD_PCOR |= (1 << 5)
#define GREEN_LED_OFF    GPIOD_PSOR |= (1 << 5)
#define RED_LED_ON    GPIOE_PCOR |= (1 << 29)
#define RED_LED_OFF    GPIOE_PSOR |= (1 << 29)

#define IS_SW1_NHAN       (0 == (GPIOC_PDIR & 0x8))
#define IS_SW2_NHAN        (0 == (GPIOC_PDIR & 0x1000))
#define MAX_BRIGHTNESS    100
#define DELAY_COUNT     1410000UL

/************************Function code************************/

void initLed();
void delay100us(unsigned int delay_count);
void led(unsigned int doSang);


void main()
{
    initLed();
    int redBrightness = 0;
    int isButtonPressed = 0;

    while (1) {
        if (IS_SW1_NHAN) {
            if (!isButtonPressed) {
                isButtonPressed = 1;
                if (redBrightness == 100) {
                        redBrightness = 0;
                } else {
                        redBrightness += 10;
                }
            }
        } else {
            isButtonPressed = 0;
        }
        led(redBrightness);
    }
}

void led(unsigned int doSang)
{
        /* Set PTD5 and PTE29 */
        RED_LED_ON;
        /* Delay */
        delay100us(doSang);
        /* Clear PTD5 and PTE29 */
        RED_LED_OFF;
        /* Delay */
        delay100us(MAX_BRIGHTNESS - doSang);

}

void initLed()
{
    /* Enable clock for PORTC, PORTD, PORTE */
    SIM_SCGC5 |= (1 << 11);    /* Set bit 11 */
    SIM_SCGC5 |= (1 << 12);    /* Set bit 12 */
    SIM_SCGC5 |= (1 << 13);    /* Set bit 13 */

    /* Configure multiplex of PTD5 and PTE29 as GPIO */
    /* Configure multiplex of PTC3 and PTC12 as GPIO */
    PORTC_PCR12 |= (1 << 8);
    PORTC_PCR3 |= (1 << 8);
    PORTD_PCR5 |= (1 << 8);
    PORTE_PCR29 |= (1 << 8);

    /* Configure PTC3 and PTC12 as input */
    GPIOC_PDDR &= ~(1 << 3);
    GPIOC_PDDR &= ~(1 << 12);

    /* Configure PTD5 and PTE29 as output */
    GPIOD_PDDR |= (1 << 5);
    GPIOE_PDDR |= (1 << 29);

    /* cau hinh tro treo cho nut bam*/
    PORTC_PCR12 |= 3;
    PORTC_PCR3 |= 3;

    /* Clear PTD5 and PTE29 */
    RED_LED_ON;
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
