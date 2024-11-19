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

void init_gpio();
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
int curpos = 0;
unsigned char key = 0;
int paused = 0;
int scroll = 0;
int pchar_num = 0;
int char_num = 0;
int game_over = 0;
int chars_correct = 0;
char curkey = 0;

char* start_string1 = " press start";
char* start_string2 = "   to begin";
char* paused1 = "    paused      ";
char* clear = "                ";
char* target_string = 0;
char* attempt_string = 0;
char* corp1 = "within the capitalist system all methods for raising the social productiveness of labour are brought about at the cost of the individual labourer; all means for the development of production transform themselves into means of domination over, and exploitation of, the producers; they mutilate the labourer into a fragment of a man, degrade him to the level of an appendage of a machine, destroy every remnant of charm in his work and turn it into a hated toil; they estrange from him the intellectual potentialities of the labour process in the same proportion as science is incorporated in it as an independent power; they distort the conditions under which he works, subject him during the labour process to a despotism the more hateful for its meanness; they transform his life-time into working-time, and drag his wife and child beneath the wheels of the juggernaut of capital.";
char* corp2 = "ok this is going to be short";

int main(void) {
    internal_clock(); // do not comment!

    // need for 
    init_gpio();
    init_exti();

    init_spi1();
    spi1_init_oled();
    spi1_display1(start_string1);
    spi1_display2(start_string2);

    nano_wait(100000000);
    game_over = 1;
    bitnum = 0;
    pchar_num = 0;
    paused = 0;

    setup_tim7();

    for(;;) {
        for (int i = 1; i < 9; i++) {
            // read for key changes
            key = (key >> 1) + (char_log[i] << 7);
            // add key to str1
        }
        // str1[curpos] = key_to_char(key);

        // if scroll
        if (scroll) {
            // if there's more than 16 chars left
            scroll = 0;
            if (strlen(target_string + pchar_num) >= 16) {
                for (int i = 0; i < 16; i++) {
                    str1[i] = str2[i];
                    str2[i] = target_string[pchar_num++];
                }
            } else if (strlen(target_string + pchar_num)) {
                for (int i = 0; i < 16; i++) {
                    str1[i] = str2[i];
                    str2[i] = ' ';
                }
                for (int i = 0; i < strlen(target_string + pchar_num); i++) {
                    str2[i] = target_string[pchar_num++];
                }
            } else {
                for (int i = 0; i < strlen(target_string) - char_num; i++) {
                    str1[i] = str2[i];
                    str2[i] = ' ';
                }
                for (int i = strlen(target_string) - char_num; i < 16; i++) {
                    str1[i] = ' ';
                }
                // for (int i = 0; i < 2; i++) {
                //     itoa(strlen(target_string), str2, 10);
                //     itoa(chars_correct, str2 + 4, 10);
                // }
            } 
        }
        // call the displays on each for loop
        // show accuracy or attstring
        if(char_num == strlen(target_string)) {
            // game end
            game_over = 1;
            float accuracy = (float)chars_correct / (float)strlen(target_string);
            // show accuracy
            itoa(strlen(target_string), str2, 10);
            itoa(chars_correct, str1, 10);
            spi1_display1(str1);
            spi1_display2(str2);
            // // attempt string
            // for(int i = 0; i < 16; i++) {
            //     str2[i] = attempt_string[i];
            // }

        }
        if (!game_over) {
            if (GPIOC->IDR & 1) {
                paused = ~paused;
                nano_wait(200000000);
            }
            if (paused) {
                spi1_display1(paused1);
                spi1_display2(clear);
                continue;
            }
            spi1_display1(str1);
            spi1_display2(str2);
        }
    }
    
    return 0;
}

// enables input ports for data and clock of the keyboard
void init_gpio() {
    // enable clocks
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    // pb0, 2, pa0, pc0 inputs
    GPIOA->MODER &= 0xfffffffc;
    GPIOB->MODER &= 0xffffffcc;
    GPIOC->MODER &= 0xfffffffc;
    // pull down on pc0
    GPIOC->PUPDR |= 2;
}

// turns port pb0 into exti, listening for falling edge
void init_exti() {
    // clock on
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    // setting pb0 as interrupt source and pa0 for 2 and pc0 for 4
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PB | SYSCFG_EXTICR1_EXTI2_PA;
    SYSCFG->EXTICR[1] |= SYSCFG_EXTICR2_EXTI4_PC;
    // falling edge
    EXTI->FTSR |= 1 | (1 << 4);
    EXTI->RTSR |= (1 << 2);
    // unmask
    EXTI->IMR |= 0b10101;
    // enable in NVIC
    NVIC_EnableIRQ(EXTI0_1_IRQn);
    NVIC_EnableIRQ(EXTI2_3_IRQn);
    // NVIC_EnableIRQ(EXTI4_15_IRQn);
}

// on falling edge we log a bit and increment bitnum, resets at 33 which is one key press
void EXTI0_1_IRQHandler() {
    // acknowledge
    EXTI->PR = EXTI_PR_PR0;
    if (game_over) { return; }
    // add bit to log
    char_log[bitnum] = ((GPIOB->IDR >> 2) & 1);
    // increment bitnum
    bitnum++;
    // keypress done
    if (bitnum == 33) {
        bitnum = 0;
        curkey = key_to_char(key);
        attempt_string[char_num] = curkey;
        char_num++;
        curpos++;
        if (key == 0x66) { 
            curpos--;
            char_num--;
        }
        if (curkey == target_string[char_num - 1]) { chars_correct++; }
        if (curpos > 15) {
            curpos = 0;
            scroll = 1;
        }
    }
}

void EXTI2_3_IRQHandler() {
    // ack
    EXTI->PR = EXTI_PR_PR2;
    if (paused) {
        // restart
        bitnum = 0;
        char_num = 0;
        pchar_num = 0;
        curpos = 0;
        scroll = 0;
        paused = 0;
    }
    else {
        // start
        target_string = corp2;
        game_over = 0;
    }
    attempt_string = (char*)malloc(sizeof(char) * strlen(target_string));
    for (int i = 0; i < 16; i++) {
        str1[i] = target_string[i];
        str2[i] = target_string[16 + i];
    }
    pchar_num = strlen(target_string);
}

// void EXTI4_15_IRQHandler() {
//     // ack
//     EXTI->PR = EXTI_PR_PR4;
//     if (GPIOC->IDR & 1) {
//         if (str1[0] == 'x') {
//             str1[0] = 'o';
//         } else {
//             str1[0] = 'x';
//         }
//         nano_wait(10000000);
//     }
//     // paused = ~paused;
//     // set everything to zero and call start
// }

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
            if (paused) {
                tog ? spi_data(paused1[i]) : spi_data((char)0xff);
            } else {
                tog ? spi_data(str1[i]) : spi_data((char)0xff);
            }
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
            return 'q';
        case 0x1d:
            return 'w';
        case 0x24:
            return 'e';
        case 0x2d:
            return 'r';
        case 0x2c:
            return 't';
        case 0x35:
            return 'y';
        case 0x3c:
            return 'u';
        case 0x43:
            return 'i';
        case 0x44:
            return 'o';
        case 0x4d:
            return 'p';
        case 0x1c:
            return 'a';
        case 0x1b:
            return 's';
        case 0x23:
            return 'd';
        case 0x2b:
            return 'f';
        case 0x34:
            return 'g';
        case 0x33:
            return 'h';
        case 0x3b:
            return 'j';
        case 0x42:
            return 'k';
        case 0x4b:
            return 'l';
        case 0x4c:
            return ';';
        case 0x52:
            return '\'';
        case 0x1a:
            return 'z';
        case 0x22:
            return 'x';
        case 0x21:
            return 'c';
        case 0x2a:
            return 'v';
        case 0x32:
            return 'b';
        case 0x31:
            return 'n';
        case 0x3a:
            return 'm';
        case 0x41:
            return ',';
        case 0x49:
            return '.';
        case 0x66:
            // backspace
            if (curpos != 0) {
                curpos = curpos - 1;
                char_num = char_num - 1;
            }
            if (target_string[char_num] == attempt_string[char_num]) { chars_correct--; }

            return target_string[char_num];
        case 0x29:
            return ' ';
        default:
            return ' ';
    }
}