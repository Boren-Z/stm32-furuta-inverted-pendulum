#include "stm32f10x.h"
#include <stdio.h>
#include <stdarg.h>
#include "Serial.h"

uint8_t Serial_RxData;		// Last byte received in the ISR
uint8_t Serial_RxFlag;		// Set on new data; must be cleared by the reader

/**
  * @brief  Initialize USART1: PA9 as TX (AF push-pull), PA10 as RX (input pull-up)
  * @param  None
  * @retval None
  */
void Serial_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate 				= 9600;
	USART_InitStructure.USART_HardwareFlowControl 	= USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode 					= USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStructure.USART_Parity 				= USART_Parity_No;
	USART_InitStructure.USART_StopBits 				= USART_StopBits_1;
	USART_InitStructure.USART_WordLength 			= USART_WordLength_8b;
	USART_Init(USART1, &USART_InitStructure);

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);		// Interrupt when RX buffer is not empty

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel 					 = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd 				 = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority 		 = 1;
	NVIC_Init(&NVIC_InitStructure);

	USART_Cmd(USART1, ENABLE);
}


/**
  * @brief  Send one byte, returning once the transmit data register is empty
  * @param  Byte byte to send
  * @retval None
  */
void Serial_SendByte(uint8_t Byte)
{
	USART_SendData(USART1, Byte);
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);		// Wait for TXE
}


/**
  * @brief  Send Length bytes from an array in order
  * @param  Array  pointer to the array
  * @param  Length number of bytes to send
  * @retval None
  */
void Serial_SendArray(uint8_t *Array, uint16_t Length)
{
	uint16_t i;
	for (i = 0; i < Length; i ++)
	{
		Serial_SendByte(Array[i]);
	}
}


/**
  * @brief  Send a string, stopping at the terminating '\0'
  * @param  String pointer to the string
  * @retval None
  */
void Serial_SendString(char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i ++)
	{
		Serial_SendByte(String[i]);
	}
}


/**
  * @brief  Integer power X^Y, used by Serial_SendNumber to extract each digit
  * @param  X base
  * @param  Y exponent
  * @retval X raised to the power Y
  */
uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y --)
	{
		Result *= X;
	}
	return Result;
}


/**
  * @brief  Send a number as decimal ASCII, most significant digit first,
  *         padded with leading zeros to Length digits
  * @param  Number number to send
  * @param  Length number of digits to display
  * @retval None
  */
void Serial_SendNumber(uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i ++)
	{
		Serial_SendByte(Number / Serial_Pow(10, Length - i - 1) % 10 + '0');
	}
}


/**
  * @brief  Retarget fputc to USART1 so printf outputs to the serial port
  * @param  ch character to output
  * @param  f  file pointer (unused)
  * @retval the character written
  */
int fputc(int ch, FILE *f)
{
	Serial_SendByte(ch);
	return ch;
}


/**
  * @brief  Formatted output to the serial port (same usage as printf), 100-byte buffer
  * @param  format format string
  * @param  ...    variable arguments
  * @retval None
  */
void Serial_Printf(char *format, ...)
{
	char String[100];
	va_list arg;
	va_start(arg, format);
	vsprintf(String, format, arg);
	va_end(arg);
	Serial_SendString(String);
}


/**
  * @brief  Poll the receive flag; return 1 and auto-clear it if new data arrived
  * @param  None
  * @retval 1: new data received; 0: no new data
  */
uint8_t Serial_GetRxFlag(void)
{
	if (Serial_RxFlag == 1)
	{
		Serial_RxFlag = 0;
		return 1;
	}
	return 0;
}


/**
  * @brief  Return the most recently received byte
  * @param  None
  * @retval the received data
  */
uint8_t Serial_GetRxData(void)
{
	return Serial_RxData;
}


/**
  * @brief  USART1 interrupt handler: on RXNE, read the data and set the receive flag
  * @param  None
  * @retval None
  */
void USART1_IRQHandler(void)
{
	if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
	{
		Serial_RxData = USART_ReceiveData(USART1);		// Reading DR also clears RXNE
		Serial_RxFlag = 1;
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}
}
