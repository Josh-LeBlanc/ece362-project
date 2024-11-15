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

void initb();
void initc();
void setn(int32_t pin_num, int32_t val);
int32_t readpin(int32_t pin_num);
void buttons(void);
void keypad(void);
void autotest(void);
extern void internal_clock(void);
extern void nano_wait(unsigned int n);

int main(void) {
    internal_clock(); // do not comment!
    // Comment until most things have been implemented
    // autotest();
    initb();
    initc();

    // uncomment one of the loops, below, when ready
    // while(1) {
    //   buttons();
    // }

    // while(1) {
    //   keypad();
    // }

    for(;;);
    
    return 0;
}

/**
 * @brief Init GPIO port B
 *        Pin 0: input
 *        Pin 4: input
 *        Pin 8-11: output
 *
 */
void initb() {
    // turn on clock
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    // // reseting the register:
    GPIOB->MODER &= 0x00000000;
    // forces 0s in the last 2 bits
    GPIOB->MODER &= 0xfffffffc;
    // forces 0s in bits 8 and 9 (0-indexed like in manual)
    GPIOB->MODER &= 0xfffffcff;
    // 1s in bits 16, 18, 20, 22 (0-indexed like in manual)
    GPIOB->MODER |= 0x550000;
    // 0s in bits 17, 19, 21, 23 (0-indexed like in manual)
    GPIOB->MODER &= 0xff55ffff;
}

/**
 * @brief Init GPIO port C
 *        Pin 0-3: inputs with internal pull down resistors
 *        Pin 4-7: outputs
 *
 */
void initc() {
    // turn on clock
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    // 0 moder register
    // GPIOC->MODER &= 0x000000;
    // settings 01s for outputs 4, 5, 6, 7
    // GPIOC->MODER |= 0x00005500;
    // GPIOC->MODER |= ~0x0f;
    // 0 the pupdr register

    GPIOC->MODER &= ~((85 << 4*2));
    GPIOC->MODER |= 0x00005500;

    GPIOC->PUPDR &= 0x00000000;
    // set 10 on pins 0-3
    GPIOC->PUPDR |= 0xaa;
}

/**
 * @brief Set GPIO port B pin to some value
 *
 * @param pin_num: Pin number in GPIO B
 * @param val    : Pin value, if 0 then the
 *                 pin is set low, else set high
 */
void setn(int32_t pin_num, int32_t val) {
    // GPIOC->BSRR ^= GPIOC->BSRR;
    if (val == 0) {
        GPIOB->BSRR |= 1 << (16 + pin_num);
    }
    else {
        GPIOB->BSRR |= 1 << pin_num;
    }
}

/**
 * @brief Read GPIO port B pin values
 *
 * @param pin_num   : Pin number in GPIO B to be read
 * @return int32_t  : 1: the pin is high; 0: the pin is low
 */
int32_t readpin(int32_t pin_num) {
    return (GPIOB->IDR & (1 << pin_num)) >> pin_num;
}

/**
 * @brief Control LEDs with buttons
 *        Use PB0 value for PB8
 *        Use PB4 value for PB9
 *
 */
void buttons(void) {
    int32_t button0 = readpin(0);
    setn(8, button0);
    int32_t button4 = readpin(4);
    setn(9, button4);
}

/**
 * @brief Control LEDs with keypad
 * 
 */
void keypad(void) {
    for(int i=0; i < 4; i++) {
        GPIOC->ODR = 1 << (i+4);
        nano_wait(1000000);
        int32_t inputs = GPIOC->IDR & 0xf;
        int32_t input = (inputs & (1 << i)) >> i;
        setn(8 + i, input);
    }
}
