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
void init_exti();
void init_tim15();
void init_tim7();
void init_spi2();
void spi2_setup_dma();
void spi2_enable_dma();
void init_spi1();
void spi_cmd(unsigned int);
void spi_data(unsigned int);
void spi1_init_oled();
void spi1_display1(const char*);
void spi1_display2(const char*);
void spi1_setup_dma();
void spi1_enable_dma();
void update_history(int, int);
int read_rows();
void drive_column(int);
void push_queue(int);
char pop_queue();

// void initc();
// void setn(int32_t pin_num, int32_t val);
// int32_t readpin(int32_t pin_num);
// void buttons(void);
// void keypad(void);
// void autotest(void);
extern void internal_clock(void);
void nano_wait(unsigned int n);
typedef struct PS2Packet {
    
} PS2Packet;

int dignum = 0;
int code[8] = {0};
uint8_t col;
int mesg_index = 0;
uint16_t msg[8] = { 0x0000,0x0100,0x0200,0x0300,0x0400,0x0500,0x0600,0x0700 };
extern const char font[];
uint8_t hist[16];
char queue[2];  // A two-entry queue of button press/release events.
int qin;        // Which queue entry is next for input
int qout;       // Which queue entry is next for output

const char keymap[] = "DCBA#9630852*741";

int main(void) {
    internal_clock(); // do not comment!

    initb();
    init_exti();

    dignum = 0;
    for (int i = 0; i < 8; i++) { code[i] = 0; }

    //setup keyboard
    init_tim7();
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

//===========================================================================
// Copy the Timer 7 ISR from lab 5
//===========================================================================
// TODO To be copied
void init_tim15(void) {
    RCC->APB2ENR |= RCC_APB2ENR_TIM15EN;
    TIM15->PSC = 4799;
    TIM15->ARR = 9;
    TIM15->DIER |= TIM_DIER_UDE;
    TIM15->DIER &= ~TIM_DIER_UIE;
    TIM15->CR1 |= TIM_CR1_CEN;
}
void init_tim7(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
    TIM7->PSC = 4799;
    TIM7->ARR = 9;
    TIM7->DIER |= TIM_DIER_UIE;
    TIM7->CR1 |= TIM_CR1_CEN;
    NVIC_EnableIRQ(TIM7_IRQn);
}
void TIM7_IRQHandler()
{
    TIM7->SR &= ~TIM_SR_UIF;
    int rows = read_rows();
    update_history(col, rows);
    col = (col +1) & 3;
    drive_column(col);
}


//===========================================================================
// Initialize the SPI2 peripheral.
//===========================================================================
void init_spi2(void) {
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    GPIOB->MODER &= ~(GPIO_MODER_MODER12 | GPIO_MODER_MODER13 | GPIO_MODER_MODER15);
    GPIOB->MODER |= (GPIO_MODER_MODER12_1 | GPIO_MODER_MODER13_1 | GPIO_MODER_MODER15_1);
    GPIOB->AFR[1] &= ~(GPIO_AFRH_AFRH4 | GPIO_AFRH_AFRH5 | GPIO_AFRH_AFRH7);
    SPI2->CR1 &= ~SPI_CR1_SPE;
    SPI2->CR1 |= SPI_CR1_BR;
    SPI2->CR2 |= 0xF << SPI_CR2_DS_Pos;
    SPI2->CR1 |= SPI_CR1_MSTR;
    SPI2->CR2 |= SPI_CR2_SSOE | SPI_CR2_NSSP;
    SPI2->CR2 |= SPI_CR2_TXDMAEN;
    SPI2->CR1 |= SPI_CR1_SPE;
}

//===========================================================================
// Configure the SPI2 peripheral to trigger the DMA channel when the
// transmitter is empty.  Use the code from setup_dma from lab 5.
//===========================================================================
void spi2_setup_dma(void) {
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel5->CCR &= ~DMA_CCR_EN;
    DMA1_Channel5->CMAR = (uint32_t)&msg;
    DMA1_Channel5->CPAR = (uint32_t)&(SPI2->DR);
    DMA1_Channel5->CNDTR = 8;
    DMA1_Channel5->CCR &= ~DMA_CCR_DIR;
    DMA1_Channel5->CCR |= DMA_CCR_DIR;
    DMA1_Channel5->CCR |= DMA_CCR_MINC;
    DMA1_Channel5->CCR &= ~DMA_CCR_MSIZE;
    DMA1_Channel5->CCR |= DMA_CCR_MSIZE_0;
    DMA1_Channel5->CCR &= ~DMA_CCR_PSIZE;
    DMA1_Channel5->CCR |= DMA_CCR_PSIZE_0;
    DMA1_Channel5->CCR |= DMA_CCR_CIRC;
    SPI2->CR2 |= SPI_CR2_TXDMAEN;
}

//===========================================================================
// Enable the DMA channel.
//===========================================================================
void spi2_enable_dma(void) {
    DMA1_Channel5->CCR = DMA_CCR_EN;
}

//===========================================================================
// 4.4 SPI OLED Display
//===========================================================================
void init_spi1() {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    GPIOA->MODER &= ~(GPIO_MODER_MODER15 | GPIO_MODER_MODER5 | GPIO_MODER_MODER7);
    GPIOA->MODER |= (GPIO_MODER_MODER15_1 | GPIO_MODER_MODER5_1 | GPIO_MODER_MODER7_1);
    GPIOA->AFR[0] &= ~((GPIO_AFRL_AFRL5) | (GPIO_AFRL_AFRL7));
    GPIOA->AFR[1] &= ~(GPIO_AFRH_AFRH7);
    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1 |= SPI_CR1_MSTR | SPI_CR1_BR;
    //SPI1->CR2 &= ~SPI_CR2_DS;
    SPI1->CR2 |= (0x9 << SPI_CR2_DS_Pos);
    SPI1->CR2 &= ~(SPI_CR2_DS_1 | SPI_CR2_DS_2);
    SPI1->CR2 |= SPI_CR2_SSOE | SPI_CR2_NSSP;
    SPI1->CR2 |= SPI_CR2_TXDMAEN;
    SPI1->CR1 |= SPI_CR1_SPE;
}
void spi_cmd(unsigned int data) {
    while(!(SPI1->SR & SPI_SR_TXE))
    {
    }
    SPI1->DR = data;
}
void spi_data(unsigned int data) {
    spi_cmd(data | 0x200);
}
void spi1_init_oled() {
    nano_wait(1000);
    spi_cmd(0x38);
    spi_cmd(0x08);
    spi_cmd(0x01);
    nano_wait(2000);
    spi_cmd(0x06);
    spi_cmd(0x02);
    spi_cmd(0x0C);
}
void spi1_display1(const char *string) {
    spi_cmd(0x0);
    while(*string != '\0')
    {
        spi_data((unsigned int)(*string));
        string++;
    }
}
void spi1_display2(const char *string) {
    spi_cmd(0xc0);
    while(*string != '\0')
    {
        spi_data((unsigned int)(*string));
        string++;
    }
}

//===========================================================================
// This is the 34-entry buffer to be copied into SPI1.
// Each element is a 16-bit value that is either character data or a command.
// Element 0 is the command to set the cursor to the first position of line 1.
// The next 16 elements are 16 characters.
// Element 17 is the command to set the cursor to the first position of line 2.
//===========================================================================
uint16_t display[34] = {
        0x002, // Command to set the cursor at the first position line 1
        0x200+'E', 0x200+'C', 0x200+'E', 0x200+'3', 0x200+'6', + 0x200+'2', 0x200+' ', 0x200+'i',
        0x200+'s', 0x200+' ', 0x200+'t', 0x200+'h', + 0x200+'e', 0x200+' ', 0x200+' ', 0x200+' ',
        0x0c0, // Command to set the cursor at the first position line 2
        0x200+'c', 0x200+'l', 0x200+'a', 0x200+'s', 0x200+'s', + 0x200+' ', 0x200+'f', 0x200+'o',
        0x200+'r', 0x200+' ', 0x200+'y', 0x200+'o', + 0x200+'u', 0x200+'!', 0x200+' ', 0x200+' ',
};

//===========================================================================
// Configure the proper DMA channel to be triggered by SPI1_TX.
// Set the SPI1 peripheral to trigger a DMA when the transmitter is empty.
//===========================================================================
void spi1_setup_dma(void) {
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel3->CCR &= ~DMA_CCR_EN;
    DMA1_Channel3->CPAR = (uint32_t)&SPI1->DR;
    DMA1_Channel3->CMAR = (uint32_t)&display;
    DMA1_Channel3->CNDTR = 34;
    DMA1_Channel3->CCR &= ~DMA_CCR_DIR;
    DMA1_Channel3->CCR |= DMA_CCR_DIR;
    DMA1_Channel3->CCR |= DMA_CCR_MINC;
    DMA1_Channel3->CCR &= ~DMA_CCR_MSIZE;
    DMA1_Channel3->CCR |= DMA_CCR_MSIZE_0;
    DMA1_Channel3->CCR &= ~DMA_CCR_PSIZE;
    DMA1_Channel3->CCR |= DMA_CCR_PSIZE_0;
    DMA1_Channel3->CCR |= DMA_CCR_CIRC;
    SPI2->CR2 |= SPI_CR2_TXDMAEN;
}

//===========================================================================
// Enable the DMA channel triggered by SPI1_TX.
//===========================================================================
void spi1_enable_dma(void) {
    DMA1_Channel3->CCR |= DMA_CCR_EN;
}

void update_history(int c, int rows)
{
    // We used to make students do this in assembly language.
    for(int i = 0; i < 4; i++) {
        hist[4*c+i] = (hist[4*c+i]<<1) + ((rows>>i)&1);
        if (hist[4*c+i] == 0x01)
            push_queue(0x80 | keymap[4*c+i]);
        if (hist[4*c+i] == 0xfe)
            push_queue(keymap[4*c+i]);
    }
}

void drive_column(int c)
{
    GPIOC->BSRR = 0xf00000 | ~(1 << (c + 4));
}

int read_rows()
{
    return (~GPIOC->IDR) & 0xf;
}

void push_queue(int n) {
    queue[qin] = n;
    qin ^= 1;
}

char pop_queue() {
    char tmp = queue[qout];
    queue[qout] = 0;
    qout ^= 1;
    return tmp;
}

void nano_wait(unsigned int n) {
    asm(    "        mov r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
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