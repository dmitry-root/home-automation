#include "stm8s.h"
#include "stm8s_clk.h"
#include "stm8s_gpio.h"

#include "led.h"
#include "clock.h"
#include "module.h"
#include "can.h"

#include "sysinfo.h"
#include "ha/can_node.h"
#include "mod_proto.h"
#include "mod_switch.h"
#include "mod_ds18b20.h"


static void configure_clock(void);
static void configure_unused_gpio(void);
static void register_modules(void);


void main()
{
	disableInterrupts();

	configure_clock();
	configure_unused_gpio();
	register_modules();

	CAN_Node_load_modules();
	CAN_Node_Led_init_all();
	if (CAN_Node_CAN_init() != CAN_InitStatus_Success)
		CAN_Node_Led_switch(CAN_Node_Led_Error, ENABLE);
	CAN_Node_Clock_init();

	CAN_Node_Led_blink(CAN_Node_Led_Info, 10);

	while (1)
	{
		//wfi();
		//disableInterrupts();

		if (CAN_Node_Clock_check_pending())
		{
			CAN_Node_Led_clock();
			CAN_Node_handle_timer();
		}

		CAN_Node_CAN_handle_packets();
	}
}


/** Configure clock */
void configure_clock(void)
{
	// Try to use 8 MHz XTAL, and fall back to HSI on failure
	CLK_HSECmd(ENABLE);
	if (CLK_ClockSwitchConfig(CLK_SWITCHMODE_AUTO, CLK_SOURCE_HSE, DISABLE, CLK_CURRENTCLOCKSTATE_DISABLE) == ERROR)
	{
		// Error starting HSE, fall back to HSI
		CLK_HSECmd(DISABLE);
		CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV2); // Configure 8 MHz clock from HSI
	}

	// disable unneeded peripherals
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_SPI, DISABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_UART1, DISABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_UART2, DISABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_UART3, DISABLE);
	// TODO consider which timers to use, disable unneeded
}

void configure_unused_gpio(void)
{
	GPIO_TypeDef* const gpio[] = { GPIOA, GPIOB, GPIOC, GPIOD, GPIOE };
	const uint8_t unused_pins[] =
	{
		GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6, /* A */
		GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7, /* B */
		GPIO_PIN_5 | GPIO_PIN_6, /* C */
		GPIO_PIN_0 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6, /* D */
		GPIO_PIN_0 | GPIO_PIN_5 | GPIO_PIN_6 /* E */
	};

	uint8_t i;

	for (i = 0; i < sizeof(unused_pins); i++)
		GPIO_Init(gpio[i], unused_pins[i], GPIO_MODE_IN_PU_NO_IT);
}

void register_modules(void)
{
	CAN_Node_register_sysinfo_module( CAN_Node_Sysinfo_get_module() );
	CAN_Node_register_proto_module( CAN_Node_mod_proto() );
	CAN_Node_register_function_module(HA_CAN_Node_Function_Switch, CAN_Node_mod_switch());
	CAN_Node_register_function_module(HA_CAN_Node_Function_DS18B20, CAN_Node_mod_ds18b20());
}
