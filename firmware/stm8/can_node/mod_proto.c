#include "mod_proto.h"
#include "can.h"
#include "ha/can_node.h"

#include <stddef.h>


static
void proto_init(uint8_t channel_id)
{
	(void)channel_id;
}

static
void proto_deinit(uint8_t channel_id)
{
	(void)channel_id;
}

static
void proto_request(uint8_t channel_id, uint8_t address)
{
	uint8_t reply;

	(void)channel_id;

	if (address == HA_CAN_Node_ChannelCount)
	{
		reply = CAN_Node_ChannelCount;
		CAN_Node_CAN_send_reply(1, &reply);
	}
	else if (address >= HA_CAN_Node_ChannelFunction0 && address < HA_CAN_Node_ChannelFunction0 + CAN_Node_ChannelCount)
	{
		reply = CAN_Node_get_channel(address - HA_CAN_Node_ChannelFunction0);
		CAN_Node_CAN_send_reply(1, &reply);
	}
}

static
void proto_write(uint8_t channel_id, uint8_t address, uint8_t length, const uint8_t* value)
{
	uint8_t function_id;

	(void)channel_id;

	if (address == HA_CAN_Node_ChannelSaveConfig)
	{
		CAN_Node_save_modules();
		return;
	}

	if (address < HA_CAN_Node_ChannelFunction0 || address >= HA_CAN_Node_ChannelFunction0 + CAN_Node_ChannelCount || length != 1)
		return;

	function_id = value[0];
	if (function_id >= HA_CAN_Node_FunctionCount)
		return;

	CAN_Node_set_channel(address - HA_CAN_Node_ChannelFunction0, function_id);
}

static
void proto_save(uint8_t channel_id, CAN_Node_EEStream* stream)
{
	(void)channel_id;
	(void)stream;
}

static
void proto_load(uint8_t channel_id, CAN_Node_EEStream* stream)
{
	(void)channel_id;
	(void)stream;
}


static CAN_Node_Module mod_proto =
{
	.on_init = proto_init,
	.on_deinit = proto_deinit,
	.on_request = proto_request,
	.on_write = proto_write,
	.on_timer = NULL,
	.on_save = proto_save,
	.on_load = proto_load
};


CAN_Node_Module* CAN_Node_mod_proto(void)
{
	return &mod_proto;
}
