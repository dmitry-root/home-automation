#include "sysinfo.h"
#include "can.h"
#include "ha/can_proto.h"
#include "ha/can_node.h"
#include "stm8s_wwdg.h"

#include <stddef.h>


/* === sysinfo module functions === */

static uint8_t device_id = 0xff;
static uint8_t label[8] = {0};

#define SERIAL_NO ((volatile uint8_t*)0x48CD)

static
uint8_t label_length()
{
	uint8_t i;
	for (i = 0; i < sizeof(label); ++i)
		if (label[i] == 0)
			return i;
	return sizeof(label);
}

static
void sysinfo_init(uint8_t channel_id)
{
	(void)channel_id;
}

static
void sysinfo_deinit(uint8_t channel_id)
{
	(void)channel_id;
}

static
void sysinfo_request(uint8_t channel_id, uint8_t address)
{
	uint8_t reply[8];

	(void)channel_id;

	switch (address)
	{
		case HA_CAN_Common_DeviceType:
			reply[0] = HA_CAN_NodeInfo_DeviceType >> 24;
			reply[1] = (HA_CAN_NodeInfo_DeviceType >> 16) & 0xff;
			reply[2] = (HA_CAN_NodeInfo_DeviceType >> 8) & 0xff;
			reply[3] = HA_CAN_NodeInfo_DeviceType & 0xff;
			CAN_Node_CAN_send_reply(4, reply);
			break;

		case HA_CAN_Common_FirmwareVersion:
			reply[0] = 0;
			reply[1] = HA_CAN_Node_Version_Current;
			CAN_Node_CAN_send_reply(2, reply);
			break;

		case HA_CAN_Common_SerialHi:
			reply[0] = reply[1] = reply[2] = reply[3] = 0;
			reply[4] = *(SERIAL_NO + 11);
			reply[5] = *(SERIAL_NO + 10);
			reply[6] = *(SERIAL_NO + 9);
			reply[7] = *(SERIAL_NO + 8);
			CAN_Node_CAN_send_reply(8, reply);
			break;

		case HA_CAN_Common_SerialLo:
			reply[0] = *(SERIAL_NO + 7);
			reply[1] = *(SERIAL_NO + 6);
			reply[2] = *(SERIAL_NO + 5);
			reply[3] = *(SERIAL_NO + 4);
			reply[4] = *(SERIAL_NO + 3);
			reply[5] = *(SERIAL_NO + 2);
			reply[6] = *(SERIAL_NO + 1);
			reply[7] = *(SERIAL_NO + 0);
			CAN_Node_CAN_send_reply(8, reply);
			break;

		case HA_CAN_Common_DeviceLabel:
			CAN_Node_CAN_send_reply(label_length(), label);
			break;
	}
}

static
void sysinfo_write(uint8_t channel_id, uint8_t address, uint8_t length, const uint8_t* value)
{
	uint8_t i;
	(void)channel_id;

	switch (address)
	{
		case HA_CAN_Common_DeviceLabel:
			for (i = 0; i < sizeof(label); ++i)
				label[i] = (i < length) ? value[i] : 0;
			CAN_Node_save_modules();
			break;
	}
}

static
void sysinfo_save(uint8_t channel_id, CAN_Node_EEStream* stream)
{
	uint8_t i;
	(void)channel_id;

	CAN_Node_EEStream_write_byte(stream, device_id);
	for (i = 0; i < sizeof(label); ++i)
		CAN_Node_EEStream_write_byte(stream, label[i]);
}

static
void sysinfo_load(uint8_t channel_id, CAN_Node_EEStream* stream)
{
	uint8_t i;
	(void)channel_id;

	device_id = CAN_Node_EEStream_read_byte(stream);
	for (i = 0; i < sizeof(label); ++i)
		label[i] = CAN_Node_EEStream_read_byte(stream);
}


static
CAN_Node_Module sysinfo_module =
{
	.on_init = sysinfo_init,
	.on_deinit = sysinfo_deinit,
	.on_request = sysinfo_request,
	.on_write = sysinfo_write,
	.on_timer = NULL,
	.on_save = sysinfo_save,
	.on_load = sysinfo_load
};

/* public API */

CAN_Node_Module* CAN_Node_Sysinfo_get_module(void)
{
	return &sysinfo_module;
}

uint8_t CAN_Node_Sysinfo_get_device_id(void)
{
	return device_id;
}

void CAN_Node_Sysinfo_set_device_id(uint8_t value)
{
	/* TODO check some hw jumper is set */
	device_id = value;
	CAN_Node_save_modules();
	WWDG_SWReset();
}
