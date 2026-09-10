#include "stm32f10x.h"                

/**
  * @brief  ADC1 Initialization for the pendulum's Angle signal
  * @param  None
  * @retval None
  */
void AD_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AIN;	// Analogue input
	GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_50MHz; // no need
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// Steady resistance for enough time for sampling
	ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 1, ADC_SampleTime_55Cycles5);
	
	ADC_InitTypeDef ADC_InitStructure;
	ADC_InitStructure.ADC_Mode 				 = ADC_Mode_Independent;		// ADC1 and ADC2 work independently
	ADC_InitStructure.ADC_DataAlign 		 = ADC_DataAlign_Right;			// 12-bit result right-aligned in the data register
	ADC_InitStructure.ADC_ExternalTrigConv 	 = ADC_ExternalTrigConv_None;	// Software trigger only, no hardware trigger
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ScanConvMode 		 = DISABLE;
	ADC_InitStructure.ADC_NbrOfChannel 		 = 1;
	ADC_Init(ADC1, &ADC_InitStructure);
	
	ADC_Cmd(ADC1, ENABLE);
	
/*
	Reset and restart for a trustworthy sampling value
*/
	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1) == SET);
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1) == SET);
}

/**
  * @brief  ADC1 working to acquire angle value, realise in TIM1_UP_IRQHandler with 1ms frequency
  * @param  None
  * @retval ADC_GetConversionValue(ADC1)
  */
uint16_t AD_GetValue(void)
{
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);					// Trigger conversion for once
	while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
	return ADC_GetConversionValue(ADC1);
}


