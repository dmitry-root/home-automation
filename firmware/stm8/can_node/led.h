#pragma once

#include "stm8s.h"


/**
 * This module allows working with two board LEDs.
 *
 * It allows to turn leds on and off, blink them one time or with
 * different patterns.
 *
 * It internally uses TIM4 to blink LEDs.
 */


typedef enum
{
	CAN_Node_Led1 = 0,
	CAN_Node_Led2 = 1,

	CAN_Node_LedCount,

	CAN_Node_Led_Info = CAN_Node_Led1,
	CAN_Node_Led_Error = CAN_Node_Led2
} CAN_Node_Led;


void CAN_Node_Led_init_all();

void CAN_Node_Led_switch(CAN_Node_Led led, FunctionalState state);
void CAN_Node_Led_blink(CAN_Node_Led led, uint8_t scale);
void CAN_Node_Led_blink_continuous(CAN_Node_Led led, uint8_t on_scale, uint8_t off_scale);
