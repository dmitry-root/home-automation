#include "led.h"

#include "stm8s_tim4.h"
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
	volatile LedMode mode;
	uint8_t on_time, off_time;
	volatile uint8_t counter;
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

enum { MaxPeriod = 250, Tim4Period = 124 };
static uint8_t period_counter = MaxPeriod;
static uint8_t timer_ref_count = 0;


void CAN_Node_Led_init_all()
{
	uint8_t led;

	/* Configure TIM4 to run every 1 ms at 16 MHz clock. */
	TIM4_TimeBaseInit(TIM4_PRESCALER_128, Tim4Period);
	TIM4_ClearFlag(TIM4_FLAG_UPDATE);
	TIM4_ITConfig(TIM4_IT_UPDATE, ENABLE);

	/* Configure GPIOs */
	for (led = 0; led < CAN_Node_LedCount; led++)
		GPIO_Init(leds[led].gpio, leds[led].pin, GPIO_MODE_OUT_PP_HIGH_FAST);
}


/* === TIM4 access functions === */

static void timer_add_ref()
{
	if (timer_ref_count++ == 0)
		TIM4_Cmd(ENABLE);
}

static void timer_release()
{
	if (timer_ref_count > 0 && --timer_ref_count == 0)
		TIM4_Cmd(DISABLE);
}

static void switch_led(CAN_Node_Led led, FunctionalState state)
{
	if (state == ENABLE)
		leds[led].gpio->ODR |= leds[led].pin;
	else
		leds[led].gpio->ODR &= ~leds[led].pin;
}

static inline bool uses_timer(LedMode mode)
{
	return mode == LedMode_Blink || mode == LedMode_BlinkContinuous;
}

static void set_led_mode(CAN_Node_Led led, LedMode new_mode)
{
	const uint8_t old_mode = leds[led].mode;

	if (uses_timer(old_mode) && !uses_timer(new_mode))
		timer_release();
	else if (!uses_timer(old_mode) && uses_timer(new_mode))
		timer_add_ref();

	leds[led].mode = new_mode;
}

static void update_led(CAN_Node_Led led)
{
	switch (leds[led].mode)
	{
		case LedMode_Blink:
			if (++leds[led].counter >= leds[led].off_time)
			{
				switch_led(led, DISABLE);
				set_led_mode(led, LedMode_Off);
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

INTERRUPT_HANDLER(TIM4_UPD_OVF_IRQHandler, 23)
{
	uint8_t led;

	TIM4_ClearITPendingBit(TIM4_IT_UPDATE);

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
	set_led_mode(led, state == ENABLE ? LedMode_On : LedMode_Off);
	switch_led(led, state);
}

void CAN_Node_Led_blink(CAN_Node_Led led, uint8_t scale)
{
	set_led_mode(led, LedMode_Off);

	leds[led].off_time = scale;
	leds[led].counter = 0;

	switch_led(led, ENABLE);
	set_led_mode(led, LedMode_Blink);
}

void CAN_Node_Led_blink_continuous(CAN_Node_Led led, uint8_t on_scale, uint8_t off_scale)
{
	set_led_mode(led, LedMode_Off);

	leds[led].on_time = on_scale;
	leds[led].off_time = on_scale + off_scale;
	leds[led].counter = 0;

	switch_led(led, ENABLE);
	set_led_mode(led, LedMode_BlinkContinuous);
}

