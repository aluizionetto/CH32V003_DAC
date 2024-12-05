/*
 * usart_com.c
 *
 *  Created on: Nov 28, 2024
 *      Author: Aluizio d'Affonseca Netto
 */

#include "usart_com.h"
#include "buffer_fifo.h"
#include <ch32v00x.h>

//buffer para serial in
#define N_BUFF_SERIAL (32)
char ar_serial_in[N_BUFF_SERIAL];
char ar_serial_out[N_BUFF_SERIAL];
SBUFF bf_serial_in, bf_serial_out;


/*********************************************************************
 * @fn      USARTx_CFG
 *
 * @brief   Initializes the USART2 & USART3 peripheral.
 *
 * @return  none
 */
void USARTx_CFG(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure = {0};
	USART_InitTypeDef USART_InitStructure = {0};
	NVIC_InitTypeDef  NVIC_InitStructure = {0};

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1, ENABLE);

	/* USART1 TX-->D.5   RX-->D.6 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

	USART_Init(USART1, &USART_InitStructure);

	//Ativa e configura interrupção da porta serial
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	//configura buffer serial
	Init_FIFO(&bf_serial_in, ar_serial_in,sizeof(char),N_BUFF_SERIAL);
	Init_FIFO(&bf_serial_out, ar_serial_out,sizeof(char),N_BUFF_SERIAL);

	USART_Cmd(USART1, ENABLE);
}

char USART_get(void) {
	char v;
	get_sample_FIFO(&bf_serial_in,&v);
	return v;
}

int USART_available_rx (void) {
	return n_sample_in_FIFO(&bf_serial_in);
}

void USART_char(char c) {
	//while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
	//USART_SendData(USART1, c);

	if (n_sample_out_FIFO (&bf_serial_out) > 0) {
		put_sample_FIFO(&bf_serial_out,&c);
	}
	//habilita interrupção de envio
	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);

}
void USART_str(char *d) {

	while (*d != '\0') {
		USART_char(*d++);
	}
}

void USART_int(int32_t num, int16_t pdiv)
{
	int16_t g;
	uint32_t div = 1000000000;
	int8_t s0 = 0;
	if (num < 0)
	{
		USART_char('-');
		num = -1*num;
	}

	if (num == 0)
	{
		USART_char('0');
	}
	else
	{
		for (g = 10 ; g > 0; g--)
		{
			if ((pdiv > 0) && (g == pdiv)) USART_char('.');
			if (num >= div)
			{
				s0 = 1;
				USART_char((num/div) + '0');
				num %= div;
			}
			else if(s0)
			{
				USART_char('0');
			}
			div /=10;
		}
	}
}

void conv_int2str (char *bf,uint32_t num, uint8_t cat, int16_t pdiv) {
	int16_t g;
	uint32_t div = 1000000000;
	int8_t s0 = 0;

	if (cat) {
		while (*bf) bf++;
	}

	if (num < 0)
	{
		*bf++ = '-';
		*bf = '\0';
		num = -1*num;
	}

	if (num == 0)
	{
		*bf++ = '0';
		*bf = '\0';
	}
	else
	{
		for (g = 10 ; g > 0; g--)
		{
			if ((pdiv > 0) && (g == pdiv)){
				*bf++ = '.';
			}
			if (num >= div)
			{
				s0 = 1;
				*bf++ = (num/div) + '0';
				num %= div;
			}
			else if(s0)
			{
				*bf++ = '0';
			}
			*bf = '\0';
			div /=10;
		}
	}
}
void str_cat(char *bf, char *cat) {
	while (*bf) bf++;
	while (*cat) *bf++ = *cat++;
	*bf = '\0';
}

uint32_t str_len(char *bf) {
	uint32_t l=0;
	while (*bf++) l++;

	return l;
}

int32_t conv_strtoint(char *bf){
	int32_t n = 0;
	uint32_t k;

	for (k = 0; (bf[k] != '\0') && (k < 10);k++) {
		if ((k == 0) && (bf[k] == '-')) {
			n = -n;
			continue;
		}
		if ((bf[k] >= '0') && (bf[k] <= '9')) {
			n = 10*n + (bf[k] - '0');
		}
	}
	return n;
}

void USART1_IRQHandler(void) __attribute__((interrupt));
void USART1_IRQHandler(void)
{
	char c;
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		c = USART_ReceiveData(USART1);

		if (n_sample_out_FIFO (&bf_serial_in) > 0) {
			put_sample_FIFO(&bf_serial_in,&c);
		}
	}

	//interupção de envio
	if(USART_GetITStatus(USART1, USART_IT_TXE) != RESET)
	{
		if (n_sample_in_FIFO(&bf_serial_out) > 0) {
			get_sample_FIFO(&bf_serial_out,&c);
			USART_SendData(USART1, c);
		}else {  //não possui dados para envio
			USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
		}
	}
}


