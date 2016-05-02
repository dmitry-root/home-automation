#include "onewire.h"
#include "ha/can_node.h"
#include "mod_ds18b20.h"
#include "can.h"

#include <stddef.h>


enum Commands
{
	Cmd_SKIP_ROM = 0xcc,
	Cmd_CONVERT_T = 0x44,
	Cmd_READ_SCRATCHPAD = 0xBE
};

static
void ds18b20_init(uint8_t channel_id)
{
	(void)channel_id;
	CAN_Node_OW_init();
}

static
void ds18b20_deinit(uint8_t channel_id)
{
	(void)channel_id;
	CAN_Node_OW_deinit();
}

static
uint8_t read_temp(uint8_t temp[2])
{
	uint16_t wait_count = 1565;
	uint8_t found = 0;

	if (CAN_Node_OW_reset() != CAN_Node_OW_Present)
		return 0;

	CAN_Node_OW_write_byte(Cmd_SKIP_ROM);
	CAN_Node_OW_write_byte(Cmd_CONVERT_T);

	/**
	 * Wait for conversion to complete.
	 * The maximum time to wait is 750 ms. Each read operation takes
	 * 60*8 = 480 mks, so we need to wait (750 / 0.48) = 1563 cycles max.
	 */
	while (wait_count > 0)
	{
		--wait_count;
		if (CAN_Node_OW_read_byte() != 0)
		{
			found = 1;
			break;
		}
	}

	if (!found)
		return 0;

	if (CAN_Node_OW_reset() != CAN_Node_OW_Present)
		return 0;

	CAN_Node_OW_write_byte(Cmd_SKIP_ROM);
	CAN_Node_OW_write_byte(Cmd_READ_SCRATCHPAD);

	CAN_Node_OW_read_bytes(2, temp);
	return 1;
}

static
void ds18b20_request(uint8_t channel_id, uint8_t address)
{
	uint8_t reply_count = 0;
	uint8_t reply[8];

	(void)channel_id;

	if (address != HA_CAN_Node_DS18B20_Temperature)
		return;

	reply_count = 2;
	if (!read_temp(reply))
	{
		reply[0] = 0xff;
		reply[1] = 0xff;
	}

	CAN_Node_CAN_send_reply(reply_count, reply);
}

static
void ds18b20_write(uint8_t channel_id, uint8_t address, uint8_t length, const uint8_t* value)
{
	(void)channel_id;
	(void)address;
	(void)length;
	(void)value;
}


static CAN_Node_Module mod_ds18b20 =
{
	.on_init = ds18b20_init,
	.on_deinit = ds18b20_deinit,
	.on_request = ds18b20_request,
	.on_write = ds18b20_write,
	.on_timer = NULL,
	.on_save = NULL,
	.on_load = NULL
};


CAN_Node_Module* CAN_Node_mod_ds18b20(void)
{
	return &mod_ds18b20;
}


