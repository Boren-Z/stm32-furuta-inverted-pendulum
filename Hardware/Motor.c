#include "stm32f10x.h" 
#include "PWM.h"

/**
  * @brief  Motor rotation direction pins initialization (Push pull inputs)
  * @param  None
  * @retval None
  */
void Motor_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_12 | GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	PWM_Init();
}

/**
  * @brief  Set motor speed and direction: sign selects direction, magnitude is the duty
  * @param  PWM from -100 to 100
  * @retval None
  */
void Motor_SetPWM(int8_t PWM)
{
	if (PWM >= 0)
	{	// Positive Direction
		GPIO_ResetBits(GPIOB, GPIO_Pin_12);
		GPIO_SetBits(GPIOB, GPIO_Pin_13);
		PWM_SetCompare1(PWM);
	}
	else
	{	// Negative Direction
		GPIO_SetBits(GPIOB, GPIO_Pin_12);
		GPIO_ResetBits(GPIOB, GPIO_Pin_13);
		PWM_SetCompare1(-PWM);
	}
}
