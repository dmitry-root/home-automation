#include "stm8s.h"
#include "stm8s_gpio.h"
#include "stm8s_clk.h"

static void show(uint8_t bright)
{
	uint32_t k, i;
	uint8_t nb = 100 - bright;
	for (k = 0; k < 50; k++)
	{
		for (i = 0; i < bright; i++)
			;
		GPIO_WriteHigh(GPIOD, GPIO_PIN_2);
		GPIO_WriteLow(GPIOD, GPIO_PIN_3);
		for (i = 0; i < nb; i++)
			;
		GPIO_WriteLow(GPIOD, GPIO_PIN_2);
		GPIO_WriteHigh(GPIOD, GPIO_PIN_3);
	}
}

void main()
{
	uint8_t j;

	CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
	GPIO_Init(GPIOD, GPIO_PIN_2 | GPIO_PIN_3, GPIO_MODE_OUT_PP_HIGH_FAST);
	GPIO_WriteHigh(GPIOD, GPIO_PIN_2);
	GPIO_WriteLow(GPIOD, GPIO_PIN_3);

	while (1)
	{
		for (j = 0; j < 100; j++)
			show(j);
		for (j = 100; j > 0; j--)
			show(j);
	}
}
