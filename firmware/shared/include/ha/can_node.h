#ifndef __HA_CAN_NODE_H_INCLUDED__
#define __HA_CAN_NODE_H_INCLUDED__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Device protocol version features.
 */
enum HA_CAN_Node_Versions
{
	HA_CAN_Node_Version_Initial = 0,        /*!< Support of Switch and Button functions */
	HA_CAN_Node_Version_DS18B20,            /*!< Support of 1-wire DS18B20 sensor */
	/* ... add new lines here ... */

	HA_CAN_Node_VersionCount,
	HA_CAN_Node_Version_Current = HA_CAN_Node_VersionCount - 1
};

/**
 * Device information for CAN NODE devices.
 */
enum HA_CAN_NodeInfo
{
	HA_CAN_NodeInfo_DeviceType = 0x00010001
};

/**
 * Below is data addressing rules defined for the CAN NODE board.
 *
 * The device is a 4-channel multi-purpose STM8-based controller.
 *
 * The 16-bit virtual address space has the following form:
 *   - 0x00xx   -- this range is reserved for protocol-specific information (can_proto.h)
 *   - 0x01xx   -- CAN NODE-specific static information
 *   - 0xccxx   -- channel-specific information. cc is (channel number + 0x80), xx is channel-specific address.
 *
 * Below is the codes for 0x01xx commands.
 */
enum HA_CAN_NodeMap
{
	/* == 0x01xx: device configuration */
	HA_CAN_Node_ChannelSaveConfig = 0x00,    /*!< W/O save device configuration */
	HA_CAN_Node_ChannelCount = 0x01,         /*!< R/O count of available channels, 1 byte */

	HA_CAN_Node_ChannelFunction0 = 0x80      /*!< R/W function number field for each channel, 1 byte */
};


static inline
uint16_t HA_CAN_Node_function_address(uint8_t channel_id)
{
	return (uint16_t)HA_CAN_Node_ChannelFunction0 + channel_id;
}

static inline
uint16_t HA_CAN_Node_field_address(uint8_t channel_id, uint8_t field)
{
	return ((uint16_t)(channel_id + 0x80) << 8) | field;
}


/** Channel functions */
enum HA_CAN_Node_Function
{
	/* General-purpose functions */
	HA_CAN_Node_Function_None,              /*!< No function, default */
	HA_CAN_Node_Function_Switch,            /*!< Digital switch, i.e. GPIO output */
	HA_CAN_Node_Function_Button,            /*!< Digital button, i.e. GPIO input (with locking and interrupt signalling) */
	HA_CAN_Node_Function_Analog,            /*!< ADC sensor */
	HA_CAN_Node_Function_PWM,               /*!< PWM output */

	/* Special device functions */
	HA_CAN_Node_Function_DS18B20,           /*!< 1-Wire Thermometer */

	HA_CAN_Node_FunctionCount
};


enum HA_CAN_Node_SwitchConfig
{
	HA_CAN_Node_SwitchConfig_Polarity = 0x01,
	HA_CAN_Node_SwitchConfig_DefaultOn = 0x02,

	HA_CAN_Node_SwitchConfigMask = 0x03
};

/** Switch fields */
enum HA_CAN_Node_SwitchFields
{
	HA_CAN_Node_Switch_Config,              /*!< Configuration bits, 1 byte, R/W */
	HA_CAN_Node_Switch_Value                /*!< Current value of the switch, 1 byte, R/W */
};


enum HA_CAN_Node_ButtonConfig
{
	HA_CAN_Node_ButtonConfig_Polarity = 0x01,         /*!< Reverse polarity, i.e. treat 1 as 0 and vice versa */
	HA_CAN_Node_ButtonConfig_PullUp = 0x02,           /*!< Turn on pull up resistor */
	HA_CAN_Node_ButtonConfig_Lock = 0x04,             /*!< Lock button value in memory until it's read (0: always report current state) */
	HA_CAN_Node_ButtonConfig_Active = 0x08,           /*!< Send button value when it's equal to 1 without user poll request */

	HA_CAN_Node_ButtonConfigMask = 0x0f
};

/** Button fields */
enum HA_CAN_Node_ButtonFields
{
	HA_CAN_Node_Button_Config,              /*!< Configuration bits, 1 byte, R/W */
	HA_CAN_Node_Button_Value                /*!< Current value of the button, 1 byte, R/W */
};


enum HA_CAN_Node_AnalogConfig
{
	HA_CAN_Node_AnalogConfig_RangeCheck = 0x01,       /*!< Check specified range and send signal to the bus when it's met */
	HA_CAN_Node_AnalogConfig_ReverseRange = 0x02      /*!< Range check succeeds when value is out of range */
};

/** Analog fields */
enum HA_CAN_Node_AnalogFields
{
	HA_CAN_Node_Analog_Config,              /*!< Configuration bits, 1 byte, R/W */
	HA_CAN_Node_Analog_RangeMin,            /*!< Range left bound, 2 bytes, R/W */
	HA_CAN_Node_Analog_RangeMax,            /*!< Range upper bound, 2 bytes, R/W */
	HA_CAN_Node_Analog_Value                /*!< Current value of analog input, 2 bytes, R/O */
};


enum HA_CAN_Node_DS18B20
{
	/* NOTE It may take up to 1 second to get a reply. */
	HA_CAN_Node_DS18B20_Temperature         /*!< Convert and read temperature. R/O, 2 bytes. Temperature is returned in DS18B20 internal format. 0xffff means no PD. */
};

#ifdef __cplusplus
}
#endif

#endif
