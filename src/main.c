/**
  ******************************************************************************
  * @file    main.c
  * @author  Weili An, Niraj Menon
  * @date    Jan 5 2024
  * @brief   ECE 362 Lab 1 template
  ******************************************************************************
*/


/**
******************************************************************************/

// Fill out your username, otherwise your completion code will have the 
// wrong username!
const char* username = "jleblan";

/******************************************************************************
*/ 

#include "stm32f0xx.h"
#include <stdint.h>
#include <string.h>

void initb();
void init_exti();
void spi1_init_oled();
void init_spi1();
void spi1_display1(const char*);
void spi1_display2(const char*);
void spi_cmd(unsigned int);
void spi_data(unsigned int);

// void initc();
// void setn(int32_t pin_num, int32_t val);
// int32_t readpin(int32_t pin_num);
// void buttons(void);
// void keypad(void);
// void autotest(void);
extern void internal_clock(void);
void nano_wait(unsigned int n);

int dignum = 0;
int code[8] = {0};

int main(void) {
    internal_clock(); // do not comment!

    initb();
    init_exti();

    dignum = 0;
    for (int i = 0; i < 8; i++) { code[i] = 0; }

    init_spi1();
    spi1_init_oled();
    spi1_display1("hello");
    spi1_display2("bitch");

    for(;;) {}
    
    return 0;
}

void initb() {
    // enable clock
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    // pb0, 2 inputs
    GPIOB->MODER &= 0xffffffcc;
}

void init_exti() {
    // clock on
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    // setting pb0 as interrupt source
    SYSCFG->EXTICR[0] |= 0x1;
    // falling edge
    EXTI->FTSR |= 1;
    // unmask
    EXTI->IMR |= 0b00001;
    // enable in NVIC
    NVIC->ISER[0] |= 0b100000;
}

void EXTI0_1_IRQHandler() {
    EXTI->PR = EXTI_PR_PR0;
    int bit = (GPIOB->IDR >> 2) & 1;
    if (dignum > 0 && dignum < 9) {
        code[dignum - 1] = bit;
    }
    dignum++;
    if (dignum == 12) { dignum = 0; }
}

void init_spi1() {
    // nss is pa15, sck is pa5, mosi is pa7
    // configure for 10 bits
    // enable it

    // clocks on
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    // set pins 15, 5, 7 to alternate function mode
    GPIOA->MODER &= 0x3ffff3ff;
    GPIOA->MODER |= 0x8000a800;
    // all in af0
    GPIOA->AFR[1] &= 0x0ffffffff;
    GPIOA->AFR[0] &= 0x0f0ffffff;
    // disable spi
    SPI1->CR1 &= ~SPI_CR1_SPE;
    // min baud rate
    SPI1->CR1 |= SPI_CR1_MSTR | SPI_CR1_BR;
    // 10 bit word size
    SPI1->CR2 |= SPI_CR2_DS_0 | SPI_CR2_DS_3;
    SPI1->CR2 &= ~(SPI_CR2_DS_1 | SPI_CR2_DS_2);
    // enable nssp
    SPI1->CR2 |= SPI_CR2_NSSP;
    // ss output enable
    SPI1->CR2 |= SPI_CR2_SSOE;
    // txdmaen
    SPI1->CR2 |= SPI_CR2_TXDMAEN;
    // enable spi channel
    SPI1->CR1 |= SPI_CR1_SPE;
}

void spi1_init_oled() {
    // wait 1 ms
    nano_wait(1000000);
    // function set
    spi_cmd(0x38);
    // display off
    spi_cmd(0x08);
    // clear display
    spi_cmd(0x01);
    // wait 2 ms
    nano_wait(2000000);
    // entry mode
    spi_cmd(0x06);
    // cursor to home position
    spi_cmd(0x02);
    // turn display on
    spi_cmd(0x0c);
}

void spi1_display1(const char *string) {
    // cursor to home position
    spi_cmd(0x02);
    // for each char in string
    for (int i = 0; i < strlen(string); i++) {
        // call spi data on char
        spi_data(string[i]);
    }
}

void spi1_display2(const char *string) {
    // move cursor to second row
    // for each char in string
    spi_cmd(0xc0);
    for (int i = 0; i < strlen(string); i++) {
        // call spi data on char
        spi_data(string[i]);
    }
}

void nano_wait(unsigned int n) {
    asm(    "        mov r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}

void spi_cmd(unsigned int data) {
    while(!(SPI1->SR & SPI_SR_TXE)) {}
    SPI1->DR = data;
}

void spi_data(unsigned int data) {
    spi_cmd(data | 0x200);
}

/**
 * @brief Init GPIO port B
 *        Pin 0: input
 *        Pin 4: input
 *        Pin 8-11: output
 *
 */
// void initb() {
//     // turn on clock
//     RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
//     // // reseting the register:
//     GPIOB->MODER &= 0x00000000;
//     // forces 0s in the last 2 bits
//     GPIOB->MODER &= 0xfffffffc;
//     // forces 0s in bits 8 and 9 (0-indexed like in manual)
//     GPIOB->MODER &= 0xfffffcff;
//     // 1s in bits 16, 18, 20, 22 (0-indexed like in manual)
//     GPIOB->MODER |= 0x550000;
//     // 0s in bits 17, 19, 21, 23 (0-indexed like in manual)
//     GPIOB->MODER &= 0xff55ffff;
// }

// /**
//  * @brief Init GPIO port C
//  *        Pin 0-3: inputs with internal pull down resistors
//  *        Pin 4-7: outputs
//  *
//  */
// void initc() {
//     // turn on clock
//     RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
//     // 0 moder register
//     // GPIOC->MODER &= 0x000000;
//     // settings 01s for outputs 4, 5, 6, 7
//     // GPIOC->MODER |= 0x00005500;
//     // GPIOC->MODER |= ~0x0f;
//     // 0 the pupdr register

//     GPIOC->MODER &= ~((85 << 4*2));
//     GPIOC->MODER |= 0x00005500;

//     GPIOC->PUPDR &= 0x00000000;
//     // set 10 on pins 0-3
//     GPIOC->PUPDR |= 0xaa;
// }

// /**
//  * @brief Set GPIO port B pin to some value
//  *
//  * @param pin_num: Pin number in GPIO B
//  * @param val    : Pin value, if 0 then the
//  *                 pin is set low, else set high
//  */
// void setn(int32_t pin_num, int32_t val) {
//     // GPIOC->BSRR ^= GPIOC->BSRR;
//     if (val == 0) {
//         GPIOB->BSRR |= 1 << (16 + pin_num);
//     }
//     else {
//         GPIOB->BSRR |= 1 << pin_num;
//     }
// }

// /**
//  * @brief Read GPIO port B pin values
//  *
//  * @param pin_num   : Pin number in GPIO B to be read
//  * @return int32_t  : 1: the pin is high; 0: the pin is low
//  */
// int32_t readpin(int32_t pin_num) {
//     return (GPIOB->IDR & (1 << pin_num)) >> pin_num;
// }

// /**
//  * @brief Control LEDs with buttons
//  *        Use PB0 value for PB8
//  *        Use PB4 value for PB9
//  *
//  */
// void buttons(void) {
//     int32_t button0 = readpin(0);
//     setn(8, button0);
//     int32_t button4 = readpin(4);
//     setn(9, button4);
// }

// /**
//  * @brief Control LEDs with keypad
//  * 
//  */
// void keypad(void) {
//     for(int i=0; i < 4; i++) {
//         GPIOC->ODR = 1 << (i+4);
//         nano_wait(1000000);
//         int32_t inputs = GPIOC->IDR & 0xf;
//         int32_t input = (inputs & (1 << i)) >> i;
//         setn(8 + i, input);
//     }
// }