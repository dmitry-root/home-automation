#include "clock.h"
#include "stm8s_tim4.h"


enum { Tim4Period = 124, ClockPostscaler = 100 };

static volatile bool clock_pending = FALSE;
static uint8_t clock_counter = ClockPostscaler;

void CAN_Node_Clock_init()
{
	/* Configure TIM4 to run every 1 ms at 16 MHz clock. */
	TIM4_TimeBaseInit(TIM4_PRESCALER_128, Tim4Period);
	TIM4_ClearFlag(TIM4_FLAG_UPDATE);
	TIM4_ITConfig(TIM4_IT_UPDATE, ENABLE);
}

bool CAN_Node_Clock_check_pending()
{
	bool result = clock_pending;
	clock_pending = FALSE;
	return result;
}


INTERRUPT_HANDLER(TIM4_UPD_OVF_IRQHandler, 23)
{
	TIM4_ClearITPendingBit(TIM4_IT_UPDATE);

	if (--clock_counter == 0)
	{
		clock_counter = ClockPostscaler;
		clock_pending = TRUE;
	}
}
