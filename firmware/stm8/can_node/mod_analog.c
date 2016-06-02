#include "stm8s_gpio.h"
#include "stm8s_adc2.h"
#include "ha/can_node.h"
#include "can.h"
#include "mod_analog.h"

#include <stddef.h>


typedef enum
{
	AnalogState_Disabled,
	AnalogState_Inactive,
	AnalogState_Active
} AnalogState;

typedef struct AnalogSensor_s
{
	GPIO_TypeDef* gpio;
	GPIO_Pin_TypeDef pin;
	ADC2_Channel_TypeDef adc_channel;
	uint8_t config;
	AnalogState state;
	uint16_t range_min, range_max;
	uint16_t value;
	uint8_t in_range;
} AnalogSensor;


static AnalogSensor analog_sensors[CAN_Node_ChannelCount] = {
	{
		.gpio = GPIOB,
		.pin = GPIO_PIN_0,
		.adc_channel = ADC2_CHANNEL_0,
		.config = 0,
		.state = AnalogState_Disabled,
		.range_min = 0, .range_max = 0,
		.value = 0,
		.in_range = 0
	},
	{
		.gpio = GPIOB,
		.pin = GPIO_PIN_1,
		.adc_channel = ADC2_CHANNEL_1,
		.config = 0,
		.state = AnalogState_Disabled,
		.range_min = 0, .range_max = 0,
		.value = 0,
		.in_range = 0
	},
	{
		.gpio = GPIOB,
		.pin = GPIO_PIN_2,
		.adc_channel = ADC2_CHANNEL_2,
		.config = 0,
		.state = AnalogState_Disabled,
		.range_min = 0, .range_max = 0,
		.value = 0,
		.in_range = 0
	},
	{
		.gpio = GPIOB,
		.pin = GPIO_PIN_3,
		.adc_channel = ADC2_CHANNEL_3,
		.config = 0,
		.state = AnalogState_Disabled,
		.range_min = 0, .range_max = 0,
		.value = 0,
		.in_range = 0
	}
};

static uint8_t sensor_count = 0;
static uint8_t selected_sensor = 0;


static
void analog_activate(uint8_t channel_id)
{
	AnalogSensor* const sensor = analog_sensors + channel_id;
	ADC2_ConversionConfig(ADC2_CONVERSIONMODE_SINGLE, sensor->adc_channel, ADC2_ALIGN_RIGHT);
	ADC2_StartConversion();
	sensor->state = AnalogState_Active;
}

static
void analog_deactivate(uint8_t channel_id)
{
	AnalogSensor* const sensor = analog_sensors + channel_id;
	sensor->value = ADC2_GetConversionValue();
	ADC2_ClearFlag();
	sensor->state = AnalogState_Inactive;
}

static
void select_next_sensor(void)
{
	const uint8_t prev_selected = selected_sensor++;
	while (selected_sensor != prev_selected)
	{
		if (selected_sensor >= CAN_Node_ChannelCount)
			selected_sensor = 0;
		if (analog_sensors[selected_sensor].state != AnalogState_Disabled)
			break;
	}
}


static
void analog_init(uint8_t channel_id)
{
	AnalogSensor* const sensor = analog_sensors + channel_id;
	GPIO_Init(sensor->gpio, sensor->pin, GPIO_MODE_IN_FL_NO_IT);

	if (sensor_count == 0)
	{
		// First channel, initialize ADC2
		ADC2_Init(
		            ADC2_CONVERSIONMODE_SINGLE,
		            sensor->adc_channel,
		            ADC2_PRESSEL_FCPU_D2,
		            ADC2_EXTTRIG_TIM,
		            DISABLE,
		            ADC2_ALIGN_RIGHT,
		            ADC2_SCHMITTTRIG_ALL,
		            DISABLE);
		ADC2_Cmd(ENABLE);
		selected_sensor = channel_id;
	}
	sensor_count++;
	sensor->state = AnalogState_Inactive;
}

static
void analog_deinit(uint8_t channel_id)
{
	AnalogSensor* const sensor = analog_sensors + channel_id;

	sensor->state = AnalogState_Disabled;
	if (channel_id == selected_sensor && sensor_count > 1)
		select_next_sensor();

	sensor_count--;
	if (sensor_count == 0)
	{
		ADC2_Cmd(DISABLE);
		ADC2_DeInit();
	}
}


static
inline void write_uint16(uint8_t* response, uint16_t value)
{
	response[0] = (uint8_t)(value >> 8);
	response[1] = (uint8_t)(value & 0xff);
}

static
inline uint16_t read_uint16(const uint8_t* request)
{
	return ((uint16_t)request[0] << 8) | request[1];
}


static
uint8_t analog_request(uint8_t channel_id, uint8_t address, uint8_t* response)
{
	const AnalogSensor* const sensor = analog_sensors + channel_id;

	switch (address)
	{
		case HA_CAN_Node_Analog_Config:
			response[0] = sensor->config;
			return 1;

		case HA_CAN_Node_Analog_Range:
			write_uint16(response, sensor->range_min);
			write_uint16(response + 2, sensor->range_max);
			return 4;

		case HA_CAN_Node_Analog_Value:
			write_uint16(response, sensor->value);
			return 2;
	}

	return NO_RESPONSE;
}


static
void analog_write(uint8_t channel_id, uint8_t address, uint8_t length, const uint8_t* value)
{
	AnalogSensor* const sensor = analog_sensors + channel_id;

	switch (address)
	{
		case HA_CAN_Node_Analog_Config:
			if (length == 1)
				sensor->config = value[0] & HA_CAN_Node_AnalogConfigMask;
			break;

		case HA_CAN_Node_Analog_Range:
			if (length == 4)
			{
				sensor->range_min = read_uint16(value);
				sensor->range_max = read_uint16(value + 2);
			}
			break;
	}
}

static
void analog_range_check(uint8_t channel_id)
{
	AnalogSensor* const sensor = analog_sensors + channel_id;
	uint8_t value[8] = {0};
	uint8_t in_range;

	if ( (sensor->config & HA_CAN_Node_AnalogConfig_RangeCheck) == 0 )
		return;

	in_range = (sensor->range_min <= sensor->value) && (sensor->value <= sensor->range_max);
	if (sensor->config & HA_CAN_Node_AnalogConfig_ReverseRange)
		in_range = 1 - in_range;

	if (in_range && !sensor->in_range)
	{
		write_uint16(value, sensor->value);
		CAN_Node_CAN_send_data(channel_id, HA_CAN_Node_Analog_Value, 2, value);
	}

	sensor->in_range = in_range;
}

static
void analog_timer(uint8_t channel_id)
{
	AnalogSensor* const sensor = analog_sensors + channel_id;

	if (channel_id != selected_sensor)
		return;

	if (sensor_count == 1 && sensor->state != AnalogState_Disabled)
	{
		analog_activate(channel_id);
		return;
	}

	switch (sensor->state)
	{
		case AnalogState_Inactive:
			analog_activate(channel_id);
			break;

		case AnalogState_Active:
			analog_deactivate(channel_id);
			select_next_sensor();
			break;
	}
}

static
void analog_save(uint8_t channel_id, CAN_Node_EEStream* stream)
{
	const AnalogSensor* const sensor = analog_sensors + channel_id;
	uint8_t data[5];
	uint8_t i;

	data[0] = sensor->config;
	write_uint16(data, sensor->range_min);
	write_uint16(data, sensor->range_max);

	for (i = 0; i < sizeof(data); ++i)
		CAN_Node_EEStream_write_byte(stream, data[i]);
}

static
void analog_load(uint8_t channel_id, CAN_Node_EEStream* stream)
{
	AnalogSensor* const sensor = analog_sensors + channel_id;
	uint8_t data[5];
	uint8_t i;

	for (i = 0; i < sizeof(data); ++i)
		data[i] = CAN_Node_EEStream_read_byte(stream);

	sensor->config = data[0];
	sensor->range_min = read_uint16(data + 1);
	sensor->range_max = read_uint16(data + 3);
}


static CAN_Node_Module mod_analog =
{
    .on_init = analog_init,
    .on_deinit = analog_deinit,
    .on_request = analog_request,
    .on_write = analog_write,
    .on_timer = analog_timer,
    .on_save = analog_save,
    .on_load = analog_load
};


CAN_Node_Module* CAN_Node_mod_analog(void)
{
	return &mod_analog;
}

