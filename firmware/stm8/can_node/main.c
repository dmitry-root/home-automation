#include "stm8s.h"
#include "stm8s_clk.h"
#include "stm8s_gpio.h"

#include "led.h"
#include "clock.h"
#include "module.h"
#include "can.h"


static void configure_clock();
static void configure_unused_gpio();


void main()
{
	configure_clock();
	configure_unused_gpio();

	CAN_Node_Led_init_all();
	CAN_Node_CAN_init();
	CAN_Node_Clock_init();
	CAN_Node_load_modules();

	while (1)
	{
		wfi();
		disableInterrupts();

		if (CAN_Node_Clock_check_pending())
		{
			CAN_Node_Led_clock();
			CAN_Node_handle_timer();
		}

		CAN_Node_CAN_handle_packets();
	}
}


/** Configure clock */
void configure_clock()
{
	CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);

	// disable unneeded peripherals
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_SPI, DISABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_UART1, DISABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_UART2, DISABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_UART3, DISABLE);
	// TODO consider which timers to use, disable unneeded
}

void configure_unused_gpio()
{
	GPIO_Init(GPIOA, GPIO_PIN_4, GPIO_MODE_IN_PU_NO_IT);
	GPIO_Init(GPIOA, GPIO_PIN_5, GPIO_MODE_IN_PU_NO_IT);
	GPIO_Init(GPIOA, GPIO_PIN_6, GPIO_MODE_IN_PU_NO_IT);

	GPIO_Init(GPIOB, GPIO_PIN_4, GPIO_MODE_IN_PU_NO_IT);
	GPIO_Init(GPIOB, GPIO_PIN_5, GPIO_MODE_IN_PU_NO_IT);
	GPIO_Init(GPIOB, GPIO_PIN_6, GPIO_MODE_IN_PU_NO_IT);
	GPIO_Init(GPIOB, GPIO_PIN_7, GPIO_MODE_IN_PU_NO_IT);

	GPIO_Init(GPIOC, GPIO_PIN_5, GPIO_MODE_IN_PU_NO_IT);
	GPIO_Init(GPIOC, GPIO_PIN_6, GPIO_MODE_IN_PU_NO_IT);

	GPIO_Init(GPIOD, GPIO_PIN_0, GPIO_MODE_IN_PU_NO_IT);
	GPIO_Init(GPIOD, GPIO_PIN_4, GPIO_MODE_IN_PU_NO_IT);
	GPIO_Init(GPIOD, GPIO_PIN_5, GPIO_MODE_IN_PU_NO_IT);
	GPIO_Init(GPIOD, GPIO_PIN_6, GPIO_MODE_IN_PU_NO_IT);

	GPIO_Init(GPIOE, GPIO_PIN_0, GPIO_MODE_IN_PU_NO_IT);
	GPIO_Init(GPIOE, GPIO_PIN_5, GPIO_MODE_IN_PU_NO_IT);
	GPIO_Init(GPIOE, GPIO_PIN_6, GPIO_MODE_IN_PU_NO_IT);
}
