/*
 * main.c
 *
 *  Created on: Oct 4, 2024
 *      Author: Aluizio d'Affonsêca Netto
 *
 *      Gera SENO com MCP4725 com frequencia de amotragem 10kHz
 *
 *      Comandos são enviados pela USART em 115200 bps
 *
 *      a -> START
 *      b -> STOP
 *      f [valor] -> define frequência do sinal de 1 a 1000 Hz
 */
#include "debug.h"

//biblioteca para DAC
#include "DAC_MCP4725.h"
#include "table_sin.h"


volatile uint16_t freq1 = 60;
volatile uint16_t adv;
volatile uint16_t state_dac = 0, a_dac = 0;
volatile uint32_t p_sample;

//buffer para comando de serial
#define LEN_BUFF_SERIAL_IN (32)
char buff_serial_in[LEN_BUFF_SERIAL_IN]; // buffer para recepção da serial
volatile int8_t buff_serial_idx = 0;
volatile uint16_t flag_cmd = 0;


/* Global define */
void ADC_Function_Init(void)
{
	ADC_InitTypeDef  ADC_InitStructure = {0};
	GPIO_InitTypeDef GPIO_InitStructure = {0};

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div2);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	ADC_DeInit(ADC1);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 1, ADC_SampleTime_241Cycles);
	ADC_Calibration_Vol(ADC1, ADC_CALVOL_50PERCENT);
	ADC_AutoInjectedConvCmd(ADC1, ENABLE);
	ADC_Cmd(ADC1, ENABLE);

	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1));
}

uint16_t Get_ADC_Val()
{
	uint16_t val;

	ADC_SoftwareStartConvCmd(ADC1, ENABLE);

	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));

	val = ADC_GetConversionValue(ADC1);

	return val;
}

void TIM2_INT_Init(u16 arr, u16 psc)
{
	TIM_TimeBaseInitTypeDef TIMBase_InitStruct;
	NVIC_InitTypeDef NVIC_InitStruct;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	TIMBase_InitStruct.TIM_Period = arr;
	TIMBase_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIMBase_InitStruct.TIM_Prescaler = psc;
	TIMBase_InitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM2, &TIMBase_InitStruct);

	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

	NVIC_InitStruct.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
	TIM_Cmd(TIM2, ENABLE);
}

//interrupção do tiemer 2
//interrupção com frequencia de 10kHz
//controle de estados do conversor DAC
void TIM2_IRQHandler() __attribute__((interrupt));
void TIM2_IRQHandler(void)
{
	static uint16_t flag_pin = 1;
	static uint16_t count_s = 0;
	static uint16_t count_pin = 0;

	//static uint16_t flag_pin;
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		// this can be replaced with your code of flag so that in main's that flag can be handled
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update); //

		switch (state_dac) {
		case 0:
			if (a_dac) {
				MCP_fast_start(0);
				count_s = 0;
				state_dac = 1;
			}
			break;
		case 1:
			p_sample = (uint32_t)(freq1*9*count_s)/250;
			//reinicia contagem de amostras para inicio do ciclo
			if (p_sample >= (2*NTABLE_SN_2)) {
				count_s = 0;
				p_sample = 0;
			}
			MCP_fast_sample(tsin(p_sample));
			count_s++;



			if (!a_dac) {
				state_dac = 2;
			}
			break;
		case 2:
			MCP_fast_stop();
			state_dac= 0;
			break;
		}

		//MCP_fast_sample_single(0, tsin((uint32_t)(freq1*9*count_s)/250));
		//count_s++;
		count_pin++;
		if ((count_pin >= 10000)) {
			GPIO_WriteBit(GPIOC,GPIO_Pin_7,flag_pin);
			flag_pin ^=0x01;
			count_pin = 0;

			/*
			adv = Get_ADC_Val();
			if (adv < 20) adv = 20;
			freq1 = (adv-20 / 2) + 1;
			 */
		}
	}
}


void gpio_init(void) {
	GPIO_InitTypeDef GPIO_InitStructure={0};
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init( GPIOC, &GPIO_InitStructure );
}


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


	USART_Cmd(USART1, ENABLE);
}


void USART_char(char c) {
	while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
	USART_SendData(USART1, c);
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
	char c_in;
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		c_in = USART_ReceiveData(USART1);

		if (c_in == '\n') return;  //desconsidera caracter

		if (c_in == '\r') {
			buff_serial_idx = 0;
			flag_cmd = 1;
			return;
		}

		buff_serial_in[buff_serial_idx] = c_in;
		buff_serial_idx++;
		if(buff_serial_idx > (LEN_BUFF_SERIAL_IN-1)) buff_serial_idx = LEN_BUFF_SERIAL_IN-1;
		buff_serial_in[buff_serial_idx] = '\0';

	}
}

//processa comandos e argumentos passados pela serial
void processa_cmd(void) {

	int8_t n_arg = 0;
	char *p_arg[10];
	char *p = buff_serial_in;

	p_arg[n_arg++] = p;
	while (*p != '\0') {
		if (*p == ' ' || *p ==',' || *p ==';') {
			*p++ = '\0';
			p_arg[n_arg++] = p;
		}
		else p++;
	}

	//comandos
	//start no dac
	if (*p_arg[0] == 'a') {
		a_dac = 1;
		USART_str("START\n\r");
	}

	//le stop no dac
	if (*p_arg[0] == 'b') {
		a_dac = 0;
		USART_str("STOP\n\r");
	}

	//define frequencia do sinal
	if (*p_arg[0] == 'f' && n_arg > 1) {
		freq1 = conv_strtoint(p_arg[1]);
		USART_str("Freq:");
		USART_int(freq1,-1);
		USART_str("\n\r");
	}

}

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	SystemInit();
	SystemCoreClockUpdate();
	Delay_Init();
	//Delay_Ms(100);
	//RCC_HCLKConfig(RCC_SYSCLK_Div2);

#if (SDI_PRINT == SDI_PR_OPEN)
	SDI_Printf_Enable();
#else
	//USART_Printf_Init(115200);
#endif
	USARTx_CFG();
	USART_str("SystemClk:");
	USART_int(SystemCoreClock,6);
	USART_str("\r\n");

	gpio_init();
	ADC_Function_Init();
	MCP_Init(0);  //MCP4725 com A0 = 0;

	GPIO_WriteBit(GPIOC,GPIO_Pin_7,1);
	TIM2_INT_Init(100-1,48-1);

	Delay_Ms(10);
	a_dac = 1;


	while(1)
	{
		if (flag_cmd) {
			USART_str(buff_serial_in);
			processa_cmd();
			flag_cmd = 0;
		}

	}
}


