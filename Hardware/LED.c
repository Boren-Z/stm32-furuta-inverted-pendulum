#include "stm32f10x.h"


/**
  * @brief  LED Initialization as signal for the pendulum is working
  * @param  None
  * @retval None
  */
void LED_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode  	= GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	GPIO_SetBits(GPIOC, GPIO_Pin_13);	// Blue pill LED, Lights up on low-level, stay off
}

/**
  * @brief  Lights LED up on low-level signal
  * @param  None
  * @retval None
  */
void LED_ON(void)
{
	GPIO_ResetBits(GPIOC, GPIO_Pin_13);
}

/**
  * @brief  Lights LED off on High-level signal
  * @param  None
  * @retval None
  */
void LED_OFF(void)
{
	GPIO_SetBits(GPIOC, GPIO_Pin_13);
}

/**
  * @brief  Flip LED's working condition
  * @param  None
  * @retval None
  */
void LED_Turn(void)
{
	if (GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13) == 0)
	{
		GPIO_SetBits(GPIOC, GPIO_Pin_13);
	}
	else
	{
		GPIO_ResetBits(GPIOC, GPIO_Pin_13);
	}
}

