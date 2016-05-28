#include "stm8s_gpio.h"
#include "ha/can_node.h"
#include "can.h"
#include "mod_button.h"

#include <stddef.h>


typedef enum
{
	ButtonState_Released,
	ButtonState_Pressed
} ButtonState;

typedef struct Button_s
{
	GPIO_TypeDef* gpio;
	GPIO_Pin_TypeDef pin;
	uint8_t config;
	ButtonState state;
} Button;


static Button buttons[CAN_Node_ChannelCount] = {
	{
		.gpio = GPIOB,
		.pin = GPIO_PIN_0,
		.config = 0,
		.state = ButtonState_Released
	},
	{
		.gpio = GPIOB,
		.pin = GPIO_PIN_1,
		.config = 0,
		.state = ButtonState_Released
	},
	{
		.gpio = GPIOB,
		.pin = GPIO_PIN_2,
		.config = 0,
		.state = ButtonState_Released
	},
	{
		.gpio = GPIOB,
		.pin = GPIO_PIN_3,
		.config = 0,
		.state = ButtonState_Released
	}
};


// Convert button config to GPIO mode bits
static
uint8_t button_gpio_mode(uint8_t channel_id)
{
	if (buttons[channel_id].config & HA_CAN_Node_ButtonConfig_PullUp)
		return GPIO_MODE_IN_PU_NO_IT;
	else
		return GPIO_MODE_IN_FL_NO_IT;
}


// Reinitialize GPIO pin according to the new configuration
static
void button_reinit(uint8_t channel_id)
{
	GPIO_Init(buttons[channel_id].gpio, buttons[channel_id].pin, button_gpio_mode(channel_id));
}

static
void button_init(uint8_t channel_id)
{
	button_reinit(channel_id);
}

static
void button_deinit(uint8_t channel_id)
{
	GPIO_Init(buttons[channel_id].gpio, buttons[channel_id].pin, GPIO_MODE_IN_FL_NO_IT);
}

static
uint8_t button_request(uint8_t channel_id, uint8_t address, uint8_t* reply)
{
	const Button* const button = buttons + channel_id;

	switch (address)
	{
		case HA_CAN_Node_Button_Config:
			reply[0] = button->config;
			return 1;

		case HA_CAN_Node_Button_Value:
			reply[0] = button->state;
			return 1;
	}

	return NO_RESPONSE;
}

static
void button_set_state(uint8_t channel_id)
{
	Button* const button = buttons + channel_id;
	const uint8_t reverse = (button->config & HA_CAN_Node_ButtonConfig_Polarity) ? 1 : 0;
	const uint8_t lock = (button->config & HA_CAN_Node_ButtonConfig_Lock) ? 1 : 0;
	const uint8_t active = (button->config & HA_CAN_Node_ButtonConfig_Active) ? 1 : 0;

	const uint8_t pin_state = (button->gpio->IDR & button->pin) ? 1 : 0;
	const ButtonState current_state = (pin_state ^ reverse) ? ButtonState_Pressed : ButtonState_Released;
	uint8_t value[8] = {0};

	if (button->state == current_state)
		return;
	if (lock && button->state == ButtonState_Pressed)
		return;

	/* now we know that the button state was definitely changed */
	button->state = current_state;
	if (active)
	{
		value[0] = button->state;
		CAN_Node_CAN_send_data(channel_id, HA_CAN_Node_Button_Value, 1, value);
	}
}

static
void button_write(uint8_t channel_id, uint8_t address, uint8_t length, const uint8_t* value)
{
	uint8_t new_config;
	Button* const button = buttons + channel_id;

	switch (address)
	{
		case HA_CAN_Node_Button_Config:
			new_config = value[0] & HA_CAN_Node_ButtonConfigMask;
			if (length == 1 && button->config != new_config)
			{
				button->config = new_config;
				button_reinit(channel_id);
				button_set_state(channel_id);
			}
			break;

		case HA_CAN_Node_Button_Value:
			/* Used to reset value to 0 */
			if (length == 1 && value[0] == 0)
			{
				button->state = ButtonState_Released;
				button_set_state(channel_id);
			}
			break;
	}
}

static
void button_timer(uint8_t channel_id)
{
	button_set_state(channel_id);
}

static
void button_save(uint8_t channel_id, CAN_Node_EEStream* stream)
{
	CAN_Node_EEStream_write_byte(stream, buttons[channel_id].config);
}

static
void button_load(uint8_t channel_id, CAN_Node_EEStream* stream)
{
	buttons[channel_id].config = CAN_Node_EEStream_read_byte(stream);
	buttons[channel_id].state = ButtonState_Released;
	button_reinit(channel_id);
	button_set_state(channel_id);
}


static CAN_Node_Module mod_button =
{
	.on_init = button_init,
	.on_deinit = button_deinit,
	.on_request = button_request,
	.on_write = button_write,
	.on_timer = button_timer,
	.on_save = button_save,
	.on_load = button_load
};


CAN_Node_Module* CAN_Node_mod_button(void)
{
	return &mod_button;
}

