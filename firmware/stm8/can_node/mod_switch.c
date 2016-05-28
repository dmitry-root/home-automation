#include "stm8s_gpio.h"
#include "ha/can_node.h"
#include "can.h"
#include "mod_switch.h"

#include <stddef.h>


typedef enum
{
	SwitchMode_Off,
	SwitchMode_On
} SwitchMode;


typedef struct Switch_s
{
	GPIO_TypeDef* gpio;
	GPIO_Pin_TypeDef pin;
	uint8_t config;
	SwitchMode mode;
} Switch;


static Switch switches[CAN_Node_ChannelCount] = {
	{
		.gpio = GPIOB,
		.pin = GPIO_PIN_0,
		.config = 0,
		.mode = SwitchMode_Off
	},
	{
		.gpio = GPIOB,
		.pin = GPIO_PIN_1,
		.config = 0,
		.mode = SwitchMode_Off
	},
	{
		.gpio = GPIOB,
		.pin = GPIO_PIN_2,
		.config = 0,
		.mode = SwitchMode_Off
	},
	{
		.gpio = GPIOB,
		.pin = GPIO_PIN_3,
		.config = 0,
		.mode = SwitchMode_Off
	}
};


static
void switch_init(uint8_t channel_id)
{
	GPIO_Init(switches[channel_id].gpio, switches[channel_id].pin, GPIO_MODE_OUT_PP_LOW_FAST);
}

static
void switch_deinit(uint8_t channel_id)
{
	GPIO_Init(switches[channel_id].gpio, switches[channel_id].pin, GPIO_MODE_IN_FL_NO_IT);
}

static
void switch_update(uint8_t channel_id)
{
	uint8_t level =
		((switches[channel_id].mode == SwitchMode_On) ? 1 : 0) ^
		((switches[channel_id].config & HA_CAN_Node_SwitchConfig_Polarity) ? 1 : 0);
	if (level)
		GPIO_WriteHigh(switches[channel_id].gpio, switches[channel_id].pin);
	else
		GPIO_WriteLow(switches[channel_id].gpio, switches[channel_id].pin);
}

static
uint8_t switch_request(uint8_t channel_id, uint8_t address, uint8_t* reply)
{
	switch (address)
	{
		case HA_CAN_Node_Switch_Config:
			reply[0] = switches[channel_id].config;
			return 1;

		case HA_CAN_Node_Switch_Value:
			reply[0] = switches[channel_id].mode;
			return 1;
	}

	return NO_RESPONSE;
}

static
void switch_write(uint8_t channel_id, uint8_t address, uint8_t length, const uint8_t* value)
{
	switch (address)
	{
		case HA_CAN_Node_Switch_Config:
			if (length == 1)
				switches[channel_id].config = value[0] & HA_CAN_Node_SwitchConfigMask;
			break;

		case HA_CAN_Node_Switch_Value:
			if (length == 1)
				switches[channel_id].mode = value[0] ? SwitchMode_On : SwitchMode_Off;
			break;
	}

	switch_update(channel_id);
}

static
void switch_save(uint8_t channel_id, CAN_Node_EEStream* stream)
{
	CAN_Node_EEStream_write_byte(stream, switches[channel_id].config);
}

static
void switch_load(uint8_t channel_id, CAN_Node_EEStream* stream)
{
	switches[channel_id].config = CAN_Node_EEStream_read_byte(stream);

	if (switches[channel_id].config & HA_CAN_Node_SwitchConfig_DefaultOn)
		switches[channel_id].mode = SwitchMode_On;
	else
		switches[channel_id].mode = SwitchMode_Off;

	switch_update(channel_id);
}


static CAN_Node_Module mod_switch =
{
	.on_init = switch_init,
	.on_deinit = switch_deinit,
	.on_request = switch_request,
	.on_write = switch_write,
	.on_timer = NULL,
	.on_save = switch_save,
	.on_load = switch_load
};


CAN_Node_Module* CAN_Node_mod_switch(void)
{
	return &mod_switch;
}
