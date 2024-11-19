#include "stm32f0xx.h"

#define SER_PIN    (1 << 12)  // PB12
#define SRCLK_PIN  (1 << 13)  // PB13
#define RCLK_PIN   (1 << 15)  // PB15
uint16_t msg[8] = { 0x0000,0x0100,0x0200,0x0300,0x0400,0x0500,0x0600,0x0700 };
extern const char font[];
volatile uint32_t time = 0;
volatile uint32_t timer_overflow = 0;
volatile uint32_t WPM = 0;

// Segment map for common cathode 8-segment display (0–9)
const uint8_t map[] = {
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F  // 9
};

void small_delay(void) {
    nano_wait(5);
}

void setup_bb(void) {
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB->MODER &= ~(GPIO_MODER_MODER12 | GPIO_MODER_MODER13 | GPIO_MODER_MODER15);
    GPIOB->MODER |= (GPIO_MODER_MODER12_0 | GPIO_MODER_MODER13_0 | GPIO_MODER_MODER15_0);
    GPIOB->ODR |= GPIO_ODR_12;
    GPIOB->ODR &= ~GPIO_ODR_13;
}

void enable_ports(void) {
    // Only enable port C for the keypad
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    GPIOC->MODER &= ~0xffff;
    GPIOC->MODER |= 0x55 << (4*2);
    GPIOC->OTYPER &= ~0xff;
    GPIOC->OTYPER |= 0xf0;
    GPIOC->PUPDR &= ~0xff;
    GPIOC->PUPDR |= 0x55;
}

void bb_write_bit(int val) {
    // CS (PB12)
    // SCK (PB13)
    // SDI (PB15)
    if(val)
    {
        GPIOB->ODR |= GPIO_ODR_15;
    }
    else
    {
        GPIOB->ODR &= ~GPIO_ODR_15;
    }
    small_delay();
    GPIOB->ODR |= GPIO_ODR_13;
    small_delay();
    GPIOB->ODR &= ~GPIO_ODR_13;
}
void bb_write_halfword(int halfword) {
    GPIOB->ODR &= ~GPIO_ODR_12;
    for(int i = 15; i >= 0; i--)
    {
        int bit = (halfword >> i) & 1;
        bb_write_bit(bit);
    }
    GPIOB->ODR |= GPIO_ODR_12;
}
void display_WPM(int val) {
    int hundreds = val/ 100;
    int tens = (val /10) % 10;
    int ones = val % 10;
    msg[0] |= font[' '];
    msg[1] |= font[' '];
    msg[2] |= font[' '];
    msg[3] |= font[' '];
    msg[4] |= font[' '];
    msg[5] |= map[hundreds];
    msg[6] |= map[tens];
    msg[7] |= map[ones];
    for(;;)
        for(int d=0; d<8; d++) {
            bb_write_halfword(msg[d]);
            nano_wait(1); // wait 1 ms between digits
        }
}

uint32_t calculate_elapsed_time(void) {
    static uint32_t start_time = 0;
    static int first_call = 1;

    if (first_call) {
        start_time = TIM2->CNT; // Save the initial counter value
        first_call = 0;         // Mark first call as done
        return 0;               // Return 0 since no time has passed
    } else {
        uint32_t current_time = TIM2->CNT; // Read the current counter value
        if (current_time >= start_time) {
            // Calculate elapsed time in seconds
            time =((current_time - start_time)/100)+1;
            //time = 420;
        } else {
            // Handle counter overflow
            time = (((TIM2->ARR - start_time) + current_time)/100) + 1;
            //time = 69;
        }
        if(timer_overflow > 1)
        {
            time += (timer_overflow-1)*10;
            if(time %10 == 0)
            {
                time -= 10;
            }
            //time = 69;
        }
        return time;
    }
}

void TIM2_IRQHandler(void) {
    if (TIM2->SR & TIM_SR_UIF) { // Check for update interrupt flag
        TIM2->SR &= ~TIM_SR_UIF; // Clear the interrupt flag
        timer_overflow++;       // Increment the overflow counter
    }
}

void init_timer(void) {
    // Enable TIM2 clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // Configure TIM2: count up, no prescaler, continuous mode
    TIM2->PSC = 47999;       // Prescaler (assuming 8 MHz clock, this makes 1ms ticks)
    TIM2->ARR = 999;        // Auto-reload value for 1-second overflow
    TIM2->DIER |= TIM_DIER_UIE; // Enable update interrupt
    NVIC_EnableIRQ(TIM2_IRQn);
    TIM2->CR1 |= TIM_CR1_CEN; // Enable the timer
}

void calculate_WPM(int n) //n in number of words
{
    display_WPM(60 * n / time);
}

int main() {
    enable_ports();  // Initialize GPIO
    setup_bb();
    init_timer();
    time = calculate_elapsed_time();
    nano_wait(19000000000);
    time = calculate_elapsed_time();
    calculate_WPM(5);
    return 0;
}