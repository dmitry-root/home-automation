#include "eestream.h"

#include "stm8s_flash.h"


void CAN_Node_EEStream_open(CAN_Node_EEStream* stream, CAN_Node_EEStream_OpenMode open_mode)
{
	stream->offset = 0;
	stream->open_mode = open_mode;

	if (open_mode == CAN_Node_EEStream_Write)
		FLASH_Unlock(FLASH_MEMTYPE_DATA);
}

void CAN_Node_EEStream_close(CAN_Node_EEStream* stream)
{
	if (stream->open_mode == CAN_Node_EEStream_Write)
		FLASH_Lock(FLASH_MEMTYPE_DATA);
	FLASH_DeInit();
}

uint8_t CAN_Node_EEStream_read_byte(CAN_Node_EEStream* stream)
{
	return FLASH_ReadByte(FLASH_DATA_START_PHYSICAL_ADDRESS + (stream->offset++));
}

void CAN_Node_EEStream_write_byte(CAN_Node_EEStream* stream, uint8_t byte)
{
	FLASH_ProgramByte(FLASH_DATA_START_PHYSICAL_ADDRESS + (stream->offset++), byte);
}
