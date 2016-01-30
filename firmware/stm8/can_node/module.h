#pragma once

#include "error.h"
#include "eestream.h"
#include "ha/can_proto.h"

/**
 * Here we define a module that handles a group of
 * CAN node addresses.
 */

/** Module interface */
typedef struct CAN_Node_Module_s
{
	/** Initialize module on specified channel */
	void (* on_init)(uint8_t channel_id);

	/** Deinitialize module on specified channel */
	void (* on_deinit)(uint8_t channel_id);

	/** Request a value with specified (low part) address */
	void (* on_request)(uint8_t channel_id, uint8_t address);

	/** Write a value at specified (low part) address */
	void (* on_write)(uint8_t channel_id, uint8_t address, uint8_t length, const uint8_t* value);

	/** Handle idle timer event */
	void (* on_timer)(uint8_t channel_id);

	/** Save module internals to EEPROM */
	void (* on_save)(uint8_t channel_id, CAN_Node_EEStream* stream);

	/** Load module internals from EEPROM */
	void (* on_load)(uint8_t channel_id, CAN_Node_EEStream* stream);
} CAN_Node_Module;


enum
{
	CAN_Node_ChannelCount = 4
};

/* === Module management API === */

void CAN_Node_register_sysinfo_module(CAN_Node_Module* module);
void CAN_Node_register_proto_module(CAN_Node_Module* module);

void CAN_Node_register_function_module(uint8_t function_id, CAN_Node_Module* module);

void CAN_Node_set_channel(uint8_t channel_id, uint8_t function_id);
uint8_t CAN_Node_get_channel(uint8_t channel_id);

void CAN_Node_handle_packet(uint8_t rtr, HA_CAN_PacketId* packet_id, uint8_t length, const uint8_t* data);

void CAN_Node_handle_timer();

void CAN_Node_save_modules();
void CAN_Node_load_modules();

