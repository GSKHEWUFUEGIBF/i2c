/* master.c - NUCLEO-F401RE I2C master
 * Blue push button B1 (PC13) raises an External Interrupt (EXTI).
 * The interrupt sets a flag; main() then sends "TURN LED ON" over I2C1.
 * I2C1_SCL = PB8 (D15), I2C1_SDA = PB9 (D14). Default 16 MHz HSI clock.
 */
#include "stm32f4xx.h"

#define SLAVE_ADDR 0x32                      /* 7-bit slave address */

volatile int button_flag = 0;                /* set by ISR, cleared by main */

static const char msg[] = "TURN LED ON";

void I2C1_init(void);
void Button_EXTI_init(void);
void I2C1_writeBytes(char saddr, const char *data, int len);
void delayMs(int n);

int main(void)
{
    I2C1_init();
    Button_EXTI_init();

    while (1)
    {
        if (button_flag)
        {
            /* send 11 characters (sizeof - 1 skips the '\0') */
            I2C1_writeBytes(SLAVE_ADDR, msg, sizeof(msg) - 1);

            delayMs(200);                    /* simple debounce delay */
            EXTI->PR = (1 << 13);            /* clear any bounce-triggered pending bit */
            button_flag = 0;                 /* ignore presses that bounced meanwhile */
        }
    }
}

/* Configure PC13 as input and enable falling-edge interrupt on EXTI line 13 */
void Button_EXTI_init(void)
{
    RCC->AHB1ENR |= (1 << 2);                /* GPIOC clock */
    RCC->APB2ENR |= (1 << 14);               /* SYSCFG clock (needed for EXTI mapping) */

    GPIOC->MODER &= ~(3 << 26);              /* PC13 = input */

    SYSCFG->EXTICR[3] &= ~(0xF << 4);        /* EXTI13 lives in EXTICR4 bits 7:4 */
    SYSCFG->EXTICR[3] |=  (0x2 << 4);        /* 0b0010 = port C -> EXTI13 = PC13 */

    EXTI->IMR  |= (1 << 13);                 /* unmask EXTI line 13 */
    EXTI->FTSR |= (1 << 13);                 /* falling edge (button is active low) */
    EXTI->RTSR &= ~(1 << 13);                /* no rising edge trigger */

    NVIC_EnableIRQ(EXTI15_10_IRQn);          /* enable the interrupt in the NVIC */
}

/* Interrupt Service Routine for EXTI lines 10-15 */
void EXTI15_10_IRQHandler(void)
{
    if (EXTI->PR & (1 << 13))                /* was it line 13? */
    {
        EXTI->PR = (1 << 13);                /* clear pending bit (write 1) */
        button_flag = 1;                     /* tell main() to send the message */
    }
}

/* I2C1 on PB8 (SCL) / PB9 (SDA), ~50 kHz, internal pull-ups */
void I2C1_init(void)
{
    RCC->AHB1ENR |= 2;                       /* GPIOB clock */
    RCC->APB1ENR |= 0x00200000;              /* I2C1 clock */

    GPIOB->AFR[1]  &= ~0x000000FF;
    GPIOB->AFR[1]  |=  0x00000044;           /* PB8, PB9 = AF4 (I2C1) */
    GPIOB->MODER   &= ~0x000F0000;
    GPIOB->MODER   |=  0x000A0000;           /* alternate function */
    GPIOB->OTYPER  |=  0x00000300;           /* open-drain */
    GPIOB->PUPDR   &= ~0x000F0000;
    GPIOB->PUPDR   |=  0x00050000;           /* internal pull-ups */

    I2C1->CR1   = 0x8000;                    /* software reset */
    I2C1->CR1  &= ~0x8000;
    I2C1->CR2   = 0x0010;                    /* peripheral clock = 16 MHz */
    I2C1->CCR   = 160;                       /* ~50 kHz */
    I2C1->TRISE = 17;
    I2C1->CR1  |= 0x0001;                    /* enable I2C1 */
}

/* Send len bytes to slave saddr in a single I2C transaction */
void I2C1_writeBytes(char saddr, const char *data, int len)
{
    volatile int tmp;

    while (I2C1->SR2 & 2);                   /* wait until bus not busy */

    I2C1->CR1 |= 0x100;                      /* START */
    while (!(I2C1->SR1 & 1));                /* wait for SB flag */

    I2C1->DR = saddr << 1;                   /* address + write bit (0) */
    while (!(I2C1->SR1 & 2));                /* wait for ADDR (hangs if no ACK) */
    tmp = I2C1->SR2;                         /* clear ADDR */

    for (int i = 0; i < len; i++)
    {
        while (!(I2C1->SR1 & 0x80));         /* wait for TXE */
        I2C1->DR = data[i];                  /* send next character */
    }

    while (!(I2C1->SR1 & 4));                /* wait for BTF (last byte done) */
    I2C1->CR1 |= 0x200;                      /* STOP */
}

/* Rough software delay (16 MHz clock) */
void delayMs(int n)
{
    for (; n > 0; n--)
        for (volatile int i = 0; i < 3200; i++);
}
