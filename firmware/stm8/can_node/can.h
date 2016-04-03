#pragma once

#include "stm8s_can.h"
#include "ha/can_proto.h"

/**
 * CAN protocol handling functions.
 */

CAN_InitStatus_TypeDef CAN_Node_CAN_init(void);

void CAN_Node_CAN_handle_packets(void);

void CAN_Node_CAN_send_reply(uint8_t length, uint8_t* value);
void CAN_Node_CAN_send_data(uint8_t channel_id, uint8_t index, uint8_t length, uint8_t* value);
