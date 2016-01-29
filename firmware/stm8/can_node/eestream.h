#pragma once

#include <stdint.h>

/**
 * EEPROM streaming engine.
 */

typedef enum
{
	CAN_Node_EEStream_Read,
	CAN_Node_EEStream_Write
} CAN_Node_EEStream_OpenMode;

typedef struct CAN_Node_EEStream_s
{
	CAN_Node_EEStream_OpenMode open_mode;
	uint16_t offset;
} CAN_Node_EEStream;


void CAN_Node_EEStream_open(CAN_Node_EEStream* stream, CAN_Node_EEStream_OpenMode open_mode);
void CAN_Node_EEStream_close(CAN_Node_EEStream* stream);

uint8_t CAN_Node_EEStream_read_byte(CAN_Node_EEStream* stream);
void CAN_Node_EEStream_write_byte(CAN_Node_EEStream* stream, uint8_t byte);
