#pragma once

#include "stm8s.h"

typedef enum CAN_Node_OW_Presence_e
{
	CAN_Node_OW_Obsent,
	CAN_Node_OW_Present
} CAN_Node_OW_Presence;

/**
 * One-wire protocol implementation based on TIM1 timer.
 */
void CAN_Node_OW_init(void);
void CAN_Node_OW_deinit(void);

CAN_Node_OW_Presence CAN_Node_OW_reset(void);

void CAN_Node_OW_write_byte(uint8_t byte);
void CAN_Node_OW_write_bytes(uint8_t count, const uint8_t* bytes);

uint8_t CAN_Node_OW_read_byte(void);
void CAN_Node_OW_read_bytes(uint8_t count, uint8_t* bytes);

