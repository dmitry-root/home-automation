#include "module.h"
#include "ha/can_node.h"
#include "ha/can_proto.h"

#include "stm8s.h"
#include "stm8s_can.h"

#include <stddef.h>


static CAN_Node_Module* sysinfo_module = NULL;
static CAN_Node_Module* proto_module = NULL;
static CAN_Node_Module* function_modules[HA_CAN_Node_FunctionCount] = { NULL };
static CAN_Node_Module* channel_modules[CAN_Node_ChannelCount] = { NULL };
static uint8_t          module_ids[CAN_Node_ChannelCount] = { 0 };


void CAN_Node_register_sysinfo_module(CAN_Node_Module* module)
{
	sysinfo_module = module;
}


void CAN_Node_register_proto_module(CAN_Node_Module* module)
{
	proto_module = module;
}


void CAN_Node_register_function_module(uint8_t function_id, CAN_Node_Module* module)
{
	assert_param(function_id < HA_CAN_Node_FunctionCount);

	function_modules[function_id] = module;
}


void CAN_Node_set_channel(uint8_t channel_id, uint8_t function_id)
{
	assert_param(channel_id < CAN_Node_ChannelCount);
	assert_param(function_id < HA_CAN_Node_FunctionCount);

	if (channel_modules[channel_id] != NULL)
	{
		channel_modules[channel_id]->on_deinit(channel_id);
		channel_modules[channel_id] = NULL;
	}

	channel_modules[channel_id] = function_modules[function_id];
	module_ids[channel_id] = function_id;

	if (channel_modules[channel_id] != NULL)
		channel_modules[channel_id]->on_init(channel_id);
}

uint8_t CAN_Node_get_channel(uint8_t channel_id)
{
	assert_param(channel_id < CAN_Node_ChannelCount);

	return module_ids[channel_id];
}

void CAN_Node_handle_packet(uint8_t rtr, HA_CAN_PacketId* packet_id, uint8_t length, const uint8_t* data)
{
	uint8_t module_index, address_lo;
	CAN_Node_Module* module = NULL;

	/**
	 * We assume here that device_id field is correct in this packet. This is because the CAN engine has a
	 * hardware filter setup for the device_id, and the special id for update is handled by CAN engine itself.
	 *
	 * So we're really interested only in address field from packet_id.
	 */

	module_index = (packet_id->address >> 8) & 0xff;
	address_lo = packet_id->address & 0xff;

	switch (module_index)
	{
		case 0:
			module = sysinfo_module;
			break;

		case 1:
			module = proto_module;
			break;

		default:
			module_index -= 2;
			if (module_index < CAN_Node_ChannelCount)
				module = channel_modules[module_index];
			break;
	}

	if (module == NULL)
		return;

	if (rtr == CAN_RTR_Data)
		module->on_write(module_index, address_lo, length, data);
	else
		module->on_request(module_index, address_lo);
}


void CAN_Node_handle_timer()
{
	uint8_t i;

	/* System modules do not require timer callback */
	for (i = 0; i < CAN_Node_ChannelCount; ++i)
	{
		if (channel_modules[i] != NULL && channel_modules[i]->on_timer != NULL)
			channel_modules[i]->on_timer(i);
	}
}


enum { Header = 0x53 };

void CAN_Node_save_modules()
{
	uint8_t i;
	CAN_Node_EEStream stream;

	if (sysinfo_module == NULL || proto_module == NULL)
		return;

	CAN_Node_EEStream_open(&stream, CAN_Node_EEStream_Write);
	CAN_Node_EEStream_write_byte(&stream, Header);

	if (sysinfo_module->on_save != NULL)
		sysinfo_module->on_save(0, &stream);
	if (proto_module->on_save != NULL)
		proto_module->on_save(0, &stream);

	for (i = 0; i < CAN_Node_ChannelCount; ++i)
		CAN_Node_EEStream_write_byte(&stream, module_ids[i]);

	for (i = 0; i < CAN_Node_ChannelCount; ++i)
	{
		if (channel_modules[i] != NULL && channel_modules[i]->on_save != NULL)
			channel_modules[i]->on_save(i, &stream);
	}

	CAN_Node_EEStream_close(&stream);
}

void CAN_Node_load_modules()
{
	uint8_t i;
	CAN_Node_EEStream stream;
	uint8_t function_id;

	if (sysinfo_module == NULL || proto_module == NULL)
		return;

	CAN_Node_EEStream_open(&stream, CAN_Node_EEStream_Read);

	i = CAN_Node_EEStream_read_byte(&stream);
	if (i != Header) // magic number
		return;

	if (sysinfo_module->on_load != NULL)
		sysinfo_module->on_load(0, &stream);
	if (proto_module->on_load != NULL)
		proto_module->on_load(0, &stream);

	for (i = 0; i < CAN_Node_ChannelCount; ++i)
	{
		function_id = CAN_Node_EEStream_read_byte(&stream);
		CAN_Node_set_channel(i, function_id < HA_CAN_Node_FunctionCount ? function_id : HA_CAN_Node_Function_None);
	}

	for (i = 0; i < CAN_Node_ChannelCount; ++i)
	{
		if (channel_modules[i] != NULL && channel_modules[i]->on_load != NULL)
			channel_modules[i]->on_load(i, &stream);
	}

	CAN_Node_EEStream_close(&stream);
}
