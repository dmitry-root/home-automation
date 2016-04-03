#include "clock.h"
#include "stm8s_tim4.h"
#include "led.h"


enum { Tim4Period = 124, ClockPostscaler = 10 };

static volatile bool clock_pending = FALSE;
static uint8_t clock_counter = ClockPostscaler;

void CAN_Node_Clock_init(void)
{
	/* Configure TIM4 to run every 2 ms at 8 MHz clock. */
	TIM4_TimeBaseInit(TIM4_PRESCALER_128, Tim4Period);
	TIM4_ClearFlag(TIM4_FLAG_UPDATE);
	//TIM4_ITConfig(TIM4_IT_UPDATE, ENABLE);
	TIM4_Cmd(ENABLE);
}

bool CAN_Node_Clock_check_pending(void)
{
	if (TIM4_GetFlagStatus(TIM4_FLAG_UPDATE) == RESET)
		return FALSE;
	TIM4_ClearFlag(TIM4_FLAG_UPDATE);

	if (--clock_counter == 0)
	{
		clock_counter = ClockPostscaler;
		return TRUE;
	}

	//bool result = clock_pending;
	//clock_pending = FALSE;
	//return result;
	return FALSE;
}


INTERRUPT_HANDLER(TIM4_UPD_OVF_IRQHandler, 23)
{
	TIM4_ClearITPendingBit(TIM4_IT_UPDATE);
	//CAN_Node_Led_switch(CAN_Node_Led2, ENABLE);
/*
	if (--clock_counter == 0)
	{
		clock_counter = ClockPostscaler;
		clock_pending = TRUE;
	}
*/
}
