#include "stm32f10x.h"
#include "Delay.h"

uint8_t Key_Num;

/**
  * @brief  Key GPIO initialization (pull-up inputs, polled)
  * @param  None
  * @retval None
  */
void Key_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/**
  * @brief  Return the debounced key event latched by Key_Tick, then clear it
  * @param  None
  * @retval 0 if no new event, otherwise key number 1-4
  */
uint8_t Key_GetNum(void)
{
	uint8_t Temp;
	if (Key_Num)
	{
		Temp = Key_Num;
		Key_Num = 0;
		return Temp;
	}
	return 0;
}

/**
  * @brief  Instantaneous raw scan: which key is currently held (no debounce)
  * @param  None
  * @retval 0 if none held, otherwise 1-4 (lowest-numbered key wins if several)
  */
uint8_t Key_GetState(void)
{
	if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_10) == 0)
	{
		return 1;
	}
	if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11) == 0)
	{
		return 2;
	}
	if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11) == 0)
	{
		return 3;
	}
	if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12) == 0)
	{
		return 4;
	}
	return 0;
}

/**
  * @brief  Periodic key scan (call every 1ms). Samples every 20ms to debounce,
  *         registers the key on the release edge into Key_Num.
  * @param  None
  * @retval None
  */
void Key_Tick(void)
{
	static uint8_t Count;
	static uint8_t CurrState, PrevState;

	
	Count ++;
	// Called in 1ms interuption function, till 20 to step in, debounce (50Hz)
	if (Count >= 20)
	{
		Count = 0;
		
		PrevState = CurrState;
		CurrState = Key_GetState();
		
		// Edge detection, register when loose hand.
		if (CurrState == 0 && PrevState != 0)
		{
			Key_Num = PrevState;	// Give the comfirmed Key nummer that just pressed
		}
	}
}

