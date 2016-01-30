#include "led.h"

#include "stm8s_gpio.h"


typedef enum
{
	LedMode_Off,
	LedMode_On,
	LedMode_Blink,
	LedMode_BlinkContinuous
} LedMode;

typedef struct Led_s
{
	GPIO_TypeDef* gpio;
	GPIO_Pin_TypeDef pin;
	LedMode mode;
	uint8_t on_time, off_time;
	uint8_t counter;
} Led;

static Led leds[CAN_Node_LedCount] = {
	{
		.gpio = GPIOD,
		.pin = GPIO_PIN_2,
		.mode = LedMode_Off,
		.on_time = 0, .off_time = 0, .counter = 0
	},
	{
		.gpio = GPIOD,
		.pin = GPIO_PIN_3,
		.mode = LedMode_Off,
		.on_time = 0, .off_time = 0, .counter = 0
	}
};

enum { MaxPeriod = 3 };
static uint8_t period_counter = MaxPeriod;


void CAN_Node_Led_init_all()
{
	uint8_t led;

	/* Configure GPIOs */
	for (led = 0; led < CAN_Node_LedCount; led++)
		GPIO_Init(leds[led].gpio, leds[led].pin, GPIO_MODE_OUT_PP_HIGH_FAST);
}


/* === TIM4 access functions === */

static void switch_led(CAN_Node_Led led, FunctionalState state)
{
	if (state == ENABLE)
		leds[led].gpio->ODR |= leds[led].pin;
	else
		leds[led].gpio->ODR &= ~leds[led].pin;
}

static void update_led(CAN_Node_Led led)
{
	switch (leds[led].mode)
	{
		case LedMode_Blink:
			if (++leds[led].counter >= leds[led].off_time)
			{
				switch_led(led, DISABLE);
				leds[led].mode = LedMode_Off;
			}
			break;

		case LedMode_BlinkContinuous:
			++leds[led].counter;
			if (leds[led].counter >= leds[led].off_time)
			{
				switch_led(led, ENABLE);
				leds[led].counter = 0;
			}
			else if (leds[led].counter == leds[led].on_time)
			{
				switch_led(led, DISABLE);
			}
			break;

		default:
			break;
	}
}

void CAN_Node_Led_clock()
{
	uint8_t led;

	if (--period_counter == 0)
	{
		period_counter = MaxPeriod;
		for (led = 0; led < CAN_Node_LedCount; led++)
			update_led(led);
	}
}


/* === Public API functions === */

void CAN_Node_Led_switch(CAN_Node_Led led, FunctionalState state)
{
	leds[led].mode = state == ENABLE ? LedMode_On : LedMode_Off;
	switch_led(led, state);
}

void CAN_Node_Led_blink(CAN_Node_Led led, uint8_t scale)
{
	leds[led].mode = LedMode_Off;

	leds[led].off_time = scale;
	leds[led].counter = 0;

	switch_led(led, ENABLE);
	leds[led].mode = LedMode_Blink;
}

void CAN_Node_Led_blink_continuous(CAN_Node_Led led, uint8_t on_scale, uint8_t off_scale)
{
	leds[led].mode = LedMode_Off;

	leds[led].on_time = on_scale;
	leds[led].off_time = on_scale + off_scale;
	leds[led].counter = 0;

	switch_led(led, ENABLE);
	leds[led].mode = LedMode_BlinkContinuous;
}

