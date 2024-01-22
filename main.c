#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <system_stm32f10x.h>
#include <stm32f10x.h>

// define states
#define Pgo 0
#define Wgo 1
#define Sgo 2
#define Wwait 3
#define Swait 4
#define Warn1 5
#define Off1 6
#define Warn2 7
#define Off2 8
#define Warn3 9
#define AllStop 10

//define colors
#define GREEN 1
#define YELLOW 2
#define WARN 3
#define ALLRED 4

const uint32_t greenDelay = 10000;
const uint32_t yellowDelay = 5000;
const uint32_t warnDelay = 1000;
const uint32_t redDelay = 180403;

static void TimerDelayMs(uint32_t time);
static void sendRemaningTime(uint8_t color, uint32_t time);
static void SystemClock_Config(void);
static void TIM_Init(void);
static void GPIO_Init(void);
static void SysTick_Init(uint32_t ticks);
volatile static uint32_t TimeDelay;
void SysTick_Handler(void);				//Delay 1ms
void Delay(uint32_t nTime);

static void PLLInit(void);
static void Interupt_Config(void);


struct State {
	uint32_t out;
	unsigned long wait;
	uint32_t next[8]; //Index of the state
};
typedef const struct State Stype;

Stype fsm[11] = {
		// Pgo
		{0x127, greenDelay, {Warn1,	Warn1,	Warn1,	Warn1,	Pgo,	Warn1,	Warn1,	Warn1}},
		// Wgo
		{0x61, greenDelay, {Wwait,	Wgo,	Wwait,	Wwait,	Wwait,	Wwait,	Wwait,	Wwait}},
		// Sgo
		{0x109, greenDelay, {Swait,	Swait,	Sgo,	Swait,	Swait,	Swait,	Swait,	Swait}},
		// Wwait
		{0xA1, yellowDelay, {AllStop,	Wgo,	Sgo,	Sgo,	Pgo,	Pgo,	Pgo,	Pgo}},
		// Swait
		{0x111, yellowDelay, {AllStop,	Wgo,	Sgo,	Wgo,	Pgo,	Wgo,	Pgo,	Wgo}},
		// Warn1
		{0x127, warnDelay, {Off1,	Off1,	Off1,	Off1,	Off1,	Off1,	Off1,	Off1}},
		// Off1
		{0x121, warnDelay, {Warn2,	Warn2,	Warn2,	Warn2,	Warn2,	Warn2,	Warn2,	Warn2}},
		// Warn2
		{0x120, warnDelay, {Off2,	Off2,	Off2,	Off2,	Off2,	Off2,	Off2,	Off2}},
		// Off2
		{0x121, warnDelay, {Warn3,	Warn3,	Warn3,	Warn3,	Warn3,	Warn3,	Warn3,	Warn3}},
		// Warn3
		{0x120, warnDelay, {AllStop,	Wgo,	Sgo,	Sgo,	Pgo,	Wgo,	Sgo,	Sgo}},
		// AllStop
		{0x121, redDelay, {AllStop,	Wgo,	Sgo,	Sgo,	Pgo,	Wgo,	Sgo,	Sgo}}
};

uint16_t S;
uint32_t Input = 0xFF;
bool checkWalk = 0;
bool checkSouth = 0;
bool checkWest = 0;
bool checkGPIO = 0;
uint8_t inputValue = 0;
uint32_t greenEnd = 0;
uint32_t yellowEnd = 0;
uint32_t warnEnd = 0;
uint32_t greenEnds = 0;
uint32_t yellowEnds = 0;
uint32_t warnEnds = 0;
uint32_t count1 = 0;
uint32_t count2 = 0;
uint16_t greenCNT = 0;
uint16_t yellowCNT = 0;
uint16_t warnCNT = 0;
uint32_t warnTime = 0;
char lcdCNT[50];


int main(void) {
	PLLInit();
	SysTick_Init(48000);
	RCC->APB2ENR |= 0x0C;
	while(0 == ((RCC->APB2ENR) & 0x0C)){}


	
}

static void sendRemaningTime(uint8_t color, uint32_t time) {
	count1 = time;
	greenCNT = TIM2->CNT;
	yellowCNT = TIM3->CNT;
	warnCNT = TIM4->CNT;
	time /= 1000;
	count2 = time;

	switch(color)
	{
	case 1: // green
		HD44780_Clear();
		HD44780_SetCursor(14,0);
		sprintf(lcdCNT,"%02ld", time+greenEnd*10);
		HD44780_PrintStr(lcdCNT);
		break;
	case 2: // yellow
		HD44780_Clear();
		HD44780_SetCursor(7,0);
		sprintf(lcdCNT,"%02ld", time);
		HD44780_PrintStr(lcdCNT);
//		HD44780_PrintStr("Hello World");
		break;
	case 3: // warn
		HD44780_Clear();
		HD44780_SetCursor(7,0);
		if(warnEnds == 1) {
			sprintf(lcdCNT,"%lu", warnTime);
			HD44780_PrintStr(lcdCNT);
			warnTime = 1;
			break;
		} else if (warnEnds == 2) {
			sprintf(lcdCNT,"%02ld", warnTime);
			HD44780_PrintStr(lcdCNT);
			warnTime = 2;
			break;
		} else if (warnEnds == 3) {
			sprintf(lcdCNT,"%02ld", warnTime);
			HD44780_PrintStr(lcdCNT);
			warnTime = 3;
			break;
		} else if (warnEnds == 4) {
			sprintf(lcdCNT,"%02ld", warnTime);
			HD44780_PrintStr(lcdCNT);
			warnTime = 4;
			break;
		} else if (warnEnds == 5) {
			sprintf(lcdCNT,"%02ld", warnTime);
			HD44780_PrintStr(lcdCNT);
			warnTime = 0;
			break;
		} else {
			sprintf(lcdCNT,"%02ld", warnTime);
			HD44780_PrintStr("00");
			warnTime = 0;
			break;
		}
		break;
	case 4: // allRed
		HD44780_Clear();
		HD44780_SetCursor(0,0);
		HD44780_PrintStr("00");
	}
}


static void TimerDelayMs(uint32_t time) {
	bool jumpToMain = 0;
	switch (time)
	{
	case 10000: // green
		warnEnds = 0;
		if(greenEnd == 0) {
			TIM2->CR1 |= TIM_CR1_CEN;;
			if(greenEnds == 0) {
				greenEnds ++;
				greenEnd = 0;
			}
			while(1) {
				sendRemaningTime(GREEN, TIM2->CNT);
				if(greenEnd == 1) {
					greenCNT = TIM2->CNT;
					jumpToMain = 1;
					TIM2->CR1 |= ~TIM_CR1_CEN;;
					break;
				}
			}
		}
		if((greenEnd >= 1 ) && (jumpToMain == 0)) {
//			count2++;
			TIM2->CR1 |= TIM_CR1_CEN;;
			uint32_t nextGreenEnd = greenEnd + 1;
			uint8_t inputCompare = inputValue;
			while(1) {
				sendRemaningTime(GREEN, TIM2->CNT);
				if((greenEnd == nextGreenEnd) || (inputCompare != inputValue)) {
					TIM2->CR1 |= ~TIM_CR1_CEN;;
					break;
				}
			}
		}
		break;
	case 180403: // all red
		greenEnds = 0;
		yellowEnds = 0;
		warnEnds = 0;
		greenEnd = 0;
		yellowEnd = 0;
		warnEnd = 0;
		while(1) {
			sendRemaningTime(ALLRED, 180403);
			if(checkGPIO == 1) {
				break;
			}
		}
	break;
	case 5000: // yellow
		yellowEnd = 0;
		greenEnd = 0;
		greenEnds = 0;
		warnEnd = 0;
		warnEnds = 0;
		TIM3->CR1 |= TIM_CR1_CEN;;
		yellowEnd = 0;
		while(1) {
			sendRemaningTime(YELLOW, TIM3->CNT);
			if(yellowEnd == 1) {
				TIM3->CR1 |= ~TIM_CR1_CEN;;
				break;
			}
		}
	case 1000: // warn
		warnEnd = 0;
		greenEnd = 0;
		greenEnds = 0;
		TIM4->CR1 |= TIM_CR1_CEN;;
		while(1) {
			sendRemaningTime(WARN, TIM4->CNT);
			warnCNT = TIM4->CNT;
			if(warnEnd == 1) {
				TIM4->CR1 |= ~TIM_CR1_CEN;;
				break;
			}
		}
		break;
	default:
		break;
	}
}


void SysTick_Init(uint32_t ticks)
{
	SysTick->CTRL = 0x00000000;	// SysTick_CTRL_DISABLE;
	SysTick->LOAD = ticks - 1;
	// set interupt piority of SysTick to least urgency
	NVIC_SetPriority (SysTick_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL);
	SysTick->VAL = 0;	// reset the value
	// select processor clock
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |
                   SysTick_CTRL_TICKINT_Msk   |
                   SysTick_CTRL_ENABLE_Msk;  /* Enable SysTick IRQ and SysTick Timer */
	
}

static void TIM_Init(void) {
	//Enable timer 2,3,4 clock
	RCC->APB1ENR |= 0x07; 
	TIM2->PSC = 16000-1;
	TIM2->ARR = 10000-100;
	TIM3->PSC = 16000-1;
	TIM3->ARR = 5000-100;
	TIM4->PSC = 16000-1;
	TIM4->ARR = 1000-100;
	
	//Send an update event to reset the timer and apply settings.
	TIM2->EGR |= TIM_EGR_UG;
	TIM3->EGR |= TIM_EGR_UG;
	TIM4->EGR |= TIM_EGR_UG;
//	TIM2->CR1 |= (1<<0); //Enable the timer and start counting
//	while (!(TIM2->SR & (1<<0))); //Wait for timer 2 to set
}

static void PLLInit(void) {
	FLASH->ACR |= FLASH_ACR_LATENCY_2;
	
	// setting APB,AHB & HSE divisor
	// read stm32f10x.h and rm0008 103/1136 for more detail
	RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;
	RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;
	RCC->CFGR &= 0xFFFDFFFF;	// (PLLXTPRE=0)
	// turn on HSE
	RCC->CR 	|= RCC_CR_HSEON;
	while (!(RCC->CR & RCC_CR_HSERDY)) 
		;		// hse is ready
	
	// config PLL
	RCC->CFGR |= RCC_CFGR_PLLMULL6;
	RCC->CFGR |= RCC_CFGR_PLLSRC_HSE;
	RCC->CFGR |= RCC_CR_PLLON;
	while (!(RCC->CR & RCC_CR_PLLRDY))
		;		// pll is ready
	
	// sellect the pll for coreclock
	RCC->CFGR  |= RCC_CFGR_SW_PLL;	// 0X02
	while (!(RCC->CFGR & RCC_CFGR_SWS_PLL))
		;		// clock is ready 48Mhz
}


static void GPIO_Init(void) {
	RCC->APB2ENR |= 0x0C; //Enable port A + port B 's clock
	GPIOA->CRL |= 0x33333333;
	GPIOA->CRH |= 0x33;
	GPIOB->CRL |= 0x888000;
	GPIOA->BSRR |= 0x380000;
}

static void Interupt_Config(void) {
	//GPIO Interrupts Config
	RCC->APB2ENR |= (1<<0);  // Enable AFIO CLOCK
	AFIO->EXTICR[0] &= (0x01<<12);  // Setup GPIO interrupt for PB3,4,5
	AFIO->EXTICR[1] &= (0x11<<0);
	EXTI->IMR |= (0x07<<3);  // Bit[1] = 1  --> Disable the Mask on EXTI 3.4.5
	EXTI->RTSR |= (0x07<<3);  // Bit[1] = 1  --> Enable Rising Edge Trigger for PB 3,4,5
	EXTI->FTSR |= (0x07<<3);  // Bit[1] = 1  --> Enable Falling Edge Trigger for PB 3,4,5
	NVIC_SetPriority(EXTI3_IRQn, 0);
	NVIC_SetPriority(EXTI4_IRQn, 0);
	NVIC_SetPriority(EXTI9_5_IRQn, 0);
	NVIC_EnableIRQ (EXTI3_IRQn);  // Enable Interrupt for PB3
	NVIC_EnableIRQ (EXTI4_IRQn);  // Enable Interrupt for PB4
	NVIC_EnableIRQ (EXTI9_5_IRQn);  // Enable Interrupt for PB5
	
	//Timers Interrupts Config
	TIM2->DIER = (1<<0);
	TIM3->DIER = (1<<0);
	TIM4->DIER = (1<<0);
	NVIC_SetPriority(TIM2_IRQn, 0);
	NVIC_SetPriority(TIM3_IRQn, 0);
	NVIC_SetPriority(TIM4_IRQn, 0);
	NVIC_EnableIRQ (TIM2_IRQn);  // Enable Interrupt for PB3
	NVIC_EnableIRQ (TIM3_IRQn);  // Enable Interrupt for PB4
	NVIC_EnableIRQ (TIM4_IRQn);  // Enable Interrupt for PB5
}

void TIM2_IRQHandler(void) {
	if ((TIM2->SR & TIM_SR_UIF) != 0) // if UIF flag is set
    {
      greenEnd += 1;
			TIM2->SR &= ~TIM_SR_UIF; // clear UIF flag
				
    }
}

void TIM3_IRQHandler(void) {
	if ((TIM3->SR & TIM_SR_UIF) != 0) // if UIF flag is set
    {
      yellowEnd += 1;
			TIM3->SR &= ~TIM_SR_UIF; // clear UIF flag
				
    }
}

void TIM4_IRQHandler(void) {
	if ((TIM2->SR & TIM_SR_UIF) != 0) // if UIF flag is set
    {
      warnEnd += 1;
    	warnEnds += 1;
    	if(warnEnds >= 6) {
    		warnEnds = 0;
    	}
			TIM2->SR &= ~TIM_SR_UIF; // clear UIF flag
				
    }
}

void EXTI3_IRQHandler(void) {
	if (EXTI->PR & (1<<3))    // If the PB3 triggered the interrupt
	{
		if((((GPIOB->IDR)&0x08)>>3)==1) {
			// Rising edge (button released)
			checkWest = 1;
		} else {
		  // Falling edge (button pressed)
			checkWest = 0;
		}
		EXTI->PR |= (1<<3);  // Clear the interrupt flag by writing a 1 
	}
	inputValue = (checkWalk << 2) | (checkSouth << 1) | (checkWest << 0);
	checkGPIO = checkWalk | checkSouth |  checkWest;
}

void EXTI4_IRQHandler(void) {
	if (EXTI->PR & (1<<4))    // If the PB3 triggered the interrupt
	{
		if((((GPIOB->IDR)&0x10)>>4)==1) {
			// Rising edge (button released)
			checkSouth = 1;
		} else {
		  // Falling edge (button pressed)
			checkSouth = 0;
		}
		EXTI->PR |= (1<<4);  // Clear the interrupt flag by writing a 1 
	}
	inputValue = (checkWalk << 2) | (checkSouth << 1) | (checkWest << 0);
	checkGPIO = checkWalk | checkSouth |  checkWest;
}

void EXTI9_5_IRQHandler(void) {
	if (EXTI->PR & (1<<5))    // If the PB5 triggered the interrupt
	{
		if((((GPIOB->IDR)&0x20)>>5)==1) {
			// Rising edge (button released)
			checkWalk = 1;
		} else {
		  // Falling edge (button pressed)
			checkWalk = 0;
		}
		EXTI->PR |= (1<<3);  // Clear the interrupt flag by writing a 1 
	}
	inputValue = (checkWalk << 2) | (checkSouth << 1) | (checkWest << 0);
	checkGPIO = checkWalk | checkSouth |  checkWest;
}