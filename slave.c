/* slave.c - NUCLEO-F401RE I2C slave
 * Receives a message over I2C1 (address 0x32). If it equals
 * "TURN LED ON", the on-board LED LD2 (PA5) changes state (toggle).
 * I2C1_SCL = PB8 (D15), I2C1_SDA = PB9 (D14). Default 16 MHz HSI clock.
 */
#include "stm32f4xx.h"
#include <string.h>

#define OWN_ADDR 0x32                        /* must match master's SLAVE_ADDR */

void LED_init(void);
void I2C1_slave_init(void);
int  I2C1_slaveReceive(char *buf, int max);

int main(void)
{
    char buf[16];

    LED_init();
    I2C1_slave_init();

    while (1)
    {
        int n = I2C1_slaveReceive(buf, sizeof(buf) - 1);
        buf[n] = '\0';                       /* make it a C string */

        if (strcmp(buf, "TURN LED ON") == 0)
            GPIOA->ODR ^= (1 << 5);          /* flip LED state */
    }
}

/* PA5 (LD2) as output */
void LED_init(void)
{
    RCC->AHB1ENR |= 1;                       /* GPIOA clock */
    GPIOA->MODER &= ~(3 << 10);
    GPIOA->MODER |=  (1 << 10);              /* PA5 = output */
}

/* I2C1 slave setup with own address OWN_ADDR */
void I2C1_slave_init(void)
{
    RCC->AHB1ENR |= 2;                       /* GPIOB clock */
    RCC->APB1ENR |= 0x00200000;              /* I2C1 clock */

    GPIOB->AFR[1]  &= ~0x000000FF;
    GPIOB->AFR[1]  |=  0x00000044;           /* PB8, PB9 = AF4 */
    GPIOB->MODER   &= ~0x000F0000;
    GPIOB->MODER   |=  0x000A0000;           /* alternate function */
    GPIOB->OTYPER  |=  0x00000300;           /* open-drain */
    GPIOB->PUPDR   &= ~0x000F0000;
    GPIOB->PUPDR   |=  0x00050000;           /* internal pull-ups */

    I2C1->CR1   = 0x8000;                    /* software reset */
    I2C1->CR1  &= ~0x8000;
    I2C1->CR2   = 0x0010;                    /* 16 MHz */
    I2C1->CCR   = 160;                       /* ~50 kHz, same as master */
    I2C1->TRISE = 17;
    I2C1->OAR1  = (OWN_ADDR << 1) | (1 << 14);   /* own address; bit 14 must be 1 */
    I2C1->CR1  |= I2C_CR1_ACK | I2C_CR1_PE;      /* ACK + enable */
}

/* Receive bytes until STOP; returns number of bytes stored in buf */
int I2C1_slaveReceive(char *buf, int max)
{
    int n = 0;
    volatile uint32_t tmp;

    while (!(I2C1->SR1 & I2C_SR1_ADDR));     /* wait for address match */
    tmp = I2C1->SR1;
    tmp = I2C1->SR2;                         /* clear ADDR */

    while (1)
    {
        if (I2C1->SR1 & I2C_SR1_RXNE)        /* a byte has arrived */
        {
            char b = I2C1->DR;
            if (n < max) buf[n++] = b;
        }
        if (I2C1->SR1 & I2C_SR1_STOPF)       /* master sent STOP */
        {
            tmp = I2C1->SR1;                 /* clear STOPF: read SR1... */
            I2C1->CR1 |= I2C_CR1_PE;         /* ...then write CR1 */
            break;
        }
    }
    return n;
}
