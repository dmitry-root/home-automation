#pragma once

#include "error.h"

/**
 * Here we define a module that handles a group of
 * CAN node addresses.
 */

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
} CAN_Node_Module;
