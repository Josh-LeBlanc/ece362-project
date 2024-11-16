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
#include <stdlib.h>

void initb();
void init_exti();
void spi1_init_oled();
void init_spi1();
void spi1_display1(const char*);
void spi1_display2(const char*);
void spi_cmd(unsigned int);
void spi_data(unsigned int);
void setup_tim7();
char key_to_char(unsigned char);

// void initc();
// void setn(int32_t pin_num, int32_t val);
// int32_t readpin(int32_t pin_num);
// void buttons(void);
// void keypad(void);
// void autotest(void);
extern void internal_clock(void);
void nano_wait(unsigned int n);

int bitnum = 0;
char char_log[33];
int tog = 0;
char str1[17] = {0};
char str2[17] = {0};
int pcurpos = 0;
int curpos = 0;
unsigned char key = 0;

int main(void) {
    internal_clock(); // do not comment!

    // need for 
    initb();
    init_exti();

    init_spi1();
    spi1_init_oled();

    nano_wait(100000000);
    bitnum = 0;

    setup_tim7();

    for(;;) {
        // show bitnum in str2
        itoa(bitnum, str2, 10);
        str2[strlen(str2)] = '\0';
        // read key from log
        for (int i = 1; i < 9; i++) {
            key = (key >> 1) + (char_log[i] << 7);
            // key_to_char returns the char based on the character
            str1[curpos] = key_to_char(key);
            // fill up row 1 with a's
        }
        pcurpos = curpos;
        // example switch statement. will make a function with the rest of the characters we need from https://techdocs.altium.com/display/FPGA/PS2+Keyboard+Scan+Codes

        // call the displays on each for loop
        spi1_display1(str1);
        spi1_display2(str2);
    }
    
    return 0;
}

// enables input ports for data and clock of the keyboard
void initb() {
    // enable clock
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    // pb0, 2 inputs
    GPIOB->MODER &= 0xffffffcc;
}

// turns port pb0 into exti, listening for falling edge
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

// on falling edge we log a bit and increment bitnum, resets at 33 which is one key press
void EXTI0_1_IRQHandler() {
    // acknowledge
    EXTI->PR = EXTI_PR_PR0;
    // add bit to log
    char_log[bitnum] = ((GPIOB->IDR >> 2) & 1);
    // increment bitnum
    bitnum++;
    // keypress done
    if (bitnum == 33) {
        bitnum = 0;
        curpos++;
        if (curpos > 15) { curpos = 0; }
    }
}

// copied from lab 6, powers the oled
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

// also from lab 6
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

// write to line 1
void spi1_display1(const char *string) {
    // cursor to home position
    spi_cmd(0x02);
    // for each char in string
    for (int i = 0; i < strlen(string); i++) {
        // if we are on cursor position and its toggled:
        if (i == curpos) { 
            tog ? spi_data((char)0xfe) : spi_data((char)0xff);
        }
        // else:
        else { spi_data(string[i]); }
    }
}

// write to line 2
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
    // this waits for the previous transmission to end and sends the next character
    while(!(SPI1->SR & SPI_SR_TXE)) {}
    SPI1->DR = data;
}

void spi_data(unsigned int data) {
    // i believe this sends a character and increments the cursor position
    spi_cmd(data | 0x200);
}

/**
 * @brief Setup timer 7 as described in lab handout
 *  
 *      this timer turns the cursor on and off
 * 
 */
void setup_tim7() {
    // turn it on
    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
    // update prescaler value
    TIM7->PSC = 4799;
    // reload val
    TIM7->ARR = 2999;
    //uie in dier
    TIM7->DIER |= TIM_DIER_UIE;
    // enable interrupt
    NVIC->ISER[0] |= (1 << 18);
    // enable timer
    TIM7->CR1 |= TIM_CR1_CEN;
}

void TIM7_IRQHandler() {
    // acknowledge
    TIM7->SR &= ~TIM_SR_UIF;
    // toggle cursor
    tog = !tog;
}

char key_to_char(unsigned char key) {
    switch (key) {
        case 0x16:
            return '1';
        case 0x1e:
            return '2';
        case 0x26:
            return '3';
        case 0x25:
            return '4';
        case 0x2e:
            return '5';
        case 0x36:
            return '6';
        case 0x3d:
            return '7';
        case 0x3e:
            return '8';
        case 0x46:
            return '9';
        case 0x45:
            return '0';
        case 0x15:
            return 'Q';
        case 0x1d:
            return 'W';
        case 0x24:
            return 'E';
        case 0x2d:
            return 'R';
        case 0x2c:
            return 'T';
        case 0x35:
            return 'Y';
        case 0x3c:
            return 'U';
        case 0x43:
            return 'I';
        case 0x44:
            return 'O';
        case 0x4d:
            return 'P';
        case 0x1c:
            return 'A';
        case 0x1b:
            return 'S';
        case 0x23:
            return 'D';
        case 0x2b:
            return 'F';
        case 0x34:
            return 'G';
        case 0x33:
            return 'H';
        case 0x3b:
            return 'J';
        case 0x42:
            return 'K';
        case 0x4b:
            return 'L';
        case 0x4c:
            return ';';
        case 0x52:
            return '\'';
        case 0x1a:
            return 'Z';
        case 0x22:
            return 'X';
        case 0x21:
            return 'C';
        case 0x2a:
            return 'V';
        case 0x32:
            return 'B';
        case 0x31:
            return 'N';
        case 0x3a:
            return 'M';
        case 0x41:
            return ',';
        case 0x49:
            return '.';
        case 0x29:
            return ' ';
        default:
            return ' ';
    }
}