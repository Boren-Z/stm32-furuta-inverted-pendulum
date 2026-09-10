#include "stm32f10x.h"

/**
  * @brief  Configure TIM3 as a quadrature encoder interface on PA6/PA7 (pull-up inputs)
  * @param  None
  * @retval None
  */
void Encoder_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
		
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision 	= TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode 		= TIM_CounterMode_Up;
/* 	
	In encoder mode CNT is clocked by the A/B edges, not by the 72MHz clock,
	so there is no periodic "update". ARR is the full 16-bit range.
*/
	TIM_TimeBaseInitStructure.TIM_Period 			= 65536 - 1;	// int16_t
	TIM_TimeBaseInitStructure.TIM_Prescaler 		= 1 - 1;
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);
	
	TIM_ICInitTypeDef TIM_ICInitStructure;
	
	/* Fill the rest of the struct with library defaults */
	TIM_ICStructInit(&TIM_ICInitStructure);

	/* Channel 1 (TI1 Channel 1): max digital filter (0xF) to reject encoder glitches */
	TIM_ICInitStructure.TIM_Channel 	= TIM_Channel_1;
	TIM_ICInitStructure.TIM_ICFilter 	= 0xF;
	TIM_ICInit(TIM3, &TIM_ICInitStructure);

	/* Channel 2 (TI2 Channel 2) */
	TIM_ICInitStructure.TIM_Channel 	= TIM_Channel_2;
	TIM_ICInitStructure.TIM_ICFilter 	= 0xF;
	TIM_ICInit(TIM3, &TIM_ICInitStructure);
	

	TIM_EncoderInterfaceConfig( TIM3,
								TIM_EncoderMode_TI12,		//x4 resolution
								TIM_ICPolarity_Rising,
								TIM_ICPolarity_Falling);

	TIM_Cmd(TIM3, ENABLE);
}

/**
  * @brief  Read the pulse count accumulated since the last call, then clear it.
  *         Called at a fixed period (TIM1 1ms ISR), so the return value is speed.
  * @param  None
  * @retval Signed count delta: >0 forward, <0 reverse
  */
int16_t Encoder_Get(void)
{
	int16_t Temp;
	Temp = TIM_GetCounter(TIM3);
	TIM_SetCounter(TIM3, 0);
	return Temp;
}
