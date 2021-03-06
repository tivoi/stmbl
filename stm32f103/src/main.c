#include "stm32f10x_conf.h"
#include <math.h>

#ifndef M_PI
#define M_PI		3.14159265358979323846
#define M_TWOPI         (M_PI * 2.0)
#endif

#define DATALENGTH 3
#define DATABAUD 2000000;

volatile uint32_t timeout = 99999;

typedef union{
	uint16_t data[DATALENGTH];
	uint8_t byte[DATALENGTH*2];
}data_t;

volatile unsigned int systime = 0;
volatile float u,v,w;

TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
TIM_OCInitTypeDef  TIM_OCInitStructure;
NVIC_InitTypeDef NVIC_InitStructure;
GPIO_InitTypeDef GPIO_InitStructure;
USART_InitTypeDef USART_InitStruct;

#define ADC_channels 13
__IO uint16_t ADCConvertedValue[ADC_channels];

void Wait(unsigned int ms){
	volatile unsigned int t = systime + ms;
	while(t >= systime){
	}
}

void SysTick_Handler(void)
{
  systime++;
}

//set pll to 24MHz
void PLL_Configurattion(void){
	RCC_PLLConfig(RCC_PLLSource_HSI_Div2,RCC_PLLMul_6); // 24MHz
	RCC_PLLCmd(ENABLE);

	/* Wait till PLL is ready */
	while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET){
	}

	/* Select PLL as system clock source */
	RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

	/* Wait till PLL is used as system clock source */
	while (RCC_GetSYSCLKSource() != 0x08){
	}

	SystemCoreClockUpdate();
}

void RCC_Configuration(void)
{
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
}

void GPIO_Configuration(void)
{
  
  //LED init
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  //Enable output init
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  

  //GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  //GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  //Analog pin configuration
  //GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  //GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  //GPIO_Init(GPIOA,&GPIO_InitStructure);
}

void tim1_init(){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	
    //TIM1
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    //TIM1 N
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	/* Channel 1, 2 and 3 Configuration in PWM mode */
	TIM_TimeBaseStructure.TIM_Period = 2400;
	TIM_TimeBaseStructure.TIM_Prescaler = 1;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
	TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
	//TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_Update);

    /* int NVIC setup */
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;

    NVIC_Init(&NVIC_InitStructure);

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;

	TIM_OC1Init(TIM1, &TIM_OCInitStructure);
	TIM_OC2Init(TIM1, &TIM_OCInitStructure);
	TIM_OC3Init(TIM1, &TIM_OCInitStructure);

	TIM_Cmd(TIM1, ENABLE);

	TIM_CtrlPWMOutputs(TIM1, ENABLE);
}

void usart_init(){
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	
    //USART TX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    //USART RX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
	
    USART_InitStruct.USART_BaudRate = DATABAUD;
    USART_InitStruct.USART_WordLength = USART_WordLength_9b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART2, &USART_InitStruct);
    /* Enable the USART2 */
    USART_Cmd(USART2, ENABLE);
}

// Setup ADC
void setup_adc(){
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define ADC1_DR_Address    ((uint32_t)0x4001244C)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
ADC_InitTypeDef ADC_InitStructure;
DMA_InitTypeDef DMA_InitStructure;

#if defined (STM32F10X_LD_VL) || defined (STM32F10X_MD_VL) || defined (STM32F10X_HD_VL)
  /* ADCCLK = PCLK2/2 */
  RCC_ADCCLKConfig(RCC_PCLK2_Div2); 
#else
  /* ADCCLK = PCLK2/4 */
  RCC_ADCCLKConfig(RCC_PCLK2_Div4); 
#endif
  /* Enable peripheral clocks ------------------------------------------------*/
  /* Enable DMA1 clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

  /* Enable ADC1 and GPIOC clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA, ENABLE);

  /* Configure PC.04 (ADC Channel14) as analog input -------------------------*/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  /* DMA1 channel1 configuration ----------------------------------------------*/
  DMA_DeInit(DMA1_Channel1);
  DMA_InitStructure.DMA_PeripheralBaseAddr = ADC1_DR_Address;
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)ADCConvertedValue;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = ADC_channels;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel1, &DMA_InitStructure);
  
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  /* Enable DMA1 channel1 */
  DMA_Cmd(DMA1_Channel1, ENABLE);
  DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);
  
  /* ADC1 configuration ------------------------------------------------------*/
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfChannel = ADC_channels;
  ADC_Init(ADC1, &ADC_InitStructure);

  ADC_TempSensorVrefintCmd(ENABLE);
  /* ADC1 regular channel14 configuration */ 
  //ADC_channels anpassen!
  ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_13Cycles5);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_TempSensor, 2, ADC_SampleTime_13Cycles5);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 3, ADC_SampleTime_13Cycles5); 
  
  ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 4, ADC_SampleTime_13Cycles5);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 5, ADC_SampleTime_13Cycles5);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 6, ADC_SampleTime_13Cycles5);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 7, ADC_SampleTime_13Cycles5);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 8, ADC_SampleTime_13Cycles5);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 9, ADC_SampleTime_13Cycles5);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 10, ADC_SampleTime_13Cycles5);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 11, ADC_SampleTime_13Cycles5);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 12, ADC_SampleTime_13Cycles5);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 13, ADC_SampleTime_13Cycles5);
 

  /* Enable ADC1 DMA */
  ADC_DMACmd(ADC1, ENABLE);
  
  /* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE);

  /* Enable ADC1 reset calibration register */   
  ADC_ResetCalibration(ADC1);
  /* Check the end of ADC1 reset calibration register */
  while(ADC_GetResetCalibrationStatus(ADC1));
  //TODO wat
  /* Start ADC1 calibration */
  ADC_StartCalibration(ADC1);
  /* Check the end of ADC1 calibration */
  while(ADC_GetCalibrationStatus(ADC1));
     
  /* Start ADC1 Software Conversion */ 
  //ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

void TIM1_UP_IRQHandler(){
	TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
	if(timeout > 30){
		GPIO_ResetBits(GPIOB,GPIO_Pin_6);//disable
		GPIO_SetBits(GPIOC,GPIO_Pin_1);//gelb
		GPIO_ResetBits(GPIOC,GPIO_Pin_2);//grün
	}else{
		GPIO_SetBits(GPIOB,GPIO_Pin_6);//Enable
		GPIO_SetBits(GPIOC,GPIO_Pin_2);//grün
		GPIO_ResetBits(GPIOC,GPIO_Pin_1);//gelb
		timeout ++;
	}
	//ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	//GPIO_SetBits(GPIOB,GPIO_Pin_12);
}

void DMA1_Channel1_IRQHandler(){
	DMA_ClearITPendingBit(DMA1_IT_TC1);
	/*
	int strom = 0;
	for(int i = 3;i<13;i++){
		strom += ADCConvertedValue[i];
	}
	int e;
	//strom /= 5;
	if(strom*50 < ADCConvertedValue[0]){
		e = strom - ADCConvertedValue[0]/100;
		float amp = 1.0;
		//amp = 1.0-e*(ADCConvertedValue[7]/4000);
		GPIO_ResetBits(GPIOB,GPIO_Pin_12);
		TIM1->CCR1 = u*amp;
		TIM1->CCR2 = v*amp;
		TIM1->CCR3 = w*amp;
	}else{
		GPIO_SetBits(GPIOB,GPIO_Pin_12);
		TIM1->CCR1 = 1200;
		TIM1->CCR2 = 1200;
		TIM1->CCR3 = 1200;
	}
	*/
}

int main(void)
{
	//PLL_Configurattion();
	RCC_Configuration();
	GPIO_Configuration();

	//GPIO_SetBits(GPIOC,GPIO_Pin_0);//rot
	//GPIO_SetBits(GPIOC,GPIO_Pin_1);//gelb
	//GPIO_SetBits(GPIOC,GPIO_Pin_2);//grün

	data_t data;
	int32_t datapos = -1;
	uint16_t buf = 0x0000;
	
	RCC_ClocksTypeDef RCC_Clocks;
	RCC_GetClocksFreq(&RCC_Clocks);
	SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000 - 1);

	//setup_adc();
	tim1_init();
	usart_init();

	TIM1->CCR1 = 0;
	TIM1->CCR2 = 0;
	TIM1->CCR3 = 0;

	while(1){
		if((USART_GetFlagStatus(USART2, USART_FLAG_RXNE) != RESET)){//rx buf not empty
			buf = USART_ReceiveData(USART2);
			if(buf == 0x155){//start condition
				datapos = 0;
				//GPIOC->BSRR = (GPIOC->ODR ^ GPIO_Pin_2) | (GPIO_Pin_2 << 16);//grün
			}else if(datapos >= 0 && datapos < DATALENGTH*2){
				data.byte[datapos++] = (uint8_t)buf;
			}
			if(datapos == DATALENGTH*2){//all data received
				datapos = -1;
				TIM1->CCR1 = data.data[0];
				TIM1->CCR2 = data.data[1];
				TIM1->CCR3 = data.data[2];
				timeout = 0;
				//GPIOC->BSRR = (GPIOC->ODR ^ GPIO_Pin_0) | (GPIO_Pin_0 << 16);//toggle red led
			}
		}
	}
}
