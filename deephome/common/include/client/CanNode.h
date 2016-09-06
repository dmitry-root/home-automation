#pragma once

#include "Device.h"
#include "ha/can_node.h"

#include <map>
#include <memory>


namespace dh
{
namespace client
{

enum ChannelType
{
	ChannelType_None = HA_CAN_Node_Function_None,
	ChannelType_Switch = HA_CAN_Node_Function_Switch,
	ChannelType_Button = HA_CAN_Node_Function_Button,
	ChannelType_Analog = HA_CAN_Node_Function_Analog,
	ChannelType_PWM = HA_CAN_Node_Function_PWM,
	ChannelType_DS18B20 = HA_CAN_Node_Function_DS18B20
};


class CanNode;

class ChannelBase : util::NonCopyable
{
protected:
	ChannelBase(CanNode& owner, ChannelType type, uint8_t id);

public:
	virtual ~ChannelBase();

	ChannelType type() const;
	uint8_t id() const;

	CanNode& owner() { return owner_; }
	const CanNode& owner() const { return owner_; }

protected:
	uint16_t field_address(uint8_t field_id) const
	{
		return HA_CAN_Node_field_address(id_, field_id);
	}

private:
	CanNode& owner_;
	const ChannelType type_;
	const uint8_t id_;
};

typedef std::shared_ptr<ChannelBase> ChannelPtr;


class SwitchChannel;
class ButtonChannel;
class AnalogChannel;
//class PwmChannel;
class Ds18b20Channel;


class CanNode : public Device
{
public:
	CanNode(Dispatcher& dispatcher, const std::string& name);
	~CanNode();

	void save();
	void query_channel_count(Result<uint8_t>& result);

	void query_channel_type(uint8_t channel_id, Result<uint8_t>& result);
	void assign_channel_type(uint8_t channel_id, ChannelType type);

	SwitchChannel& get_switch(uint8_t channel_id);
	ButtonChannel& get_button(uint8_t channel_id);
	AnalogChannel& get_analog(uint8_t channel_id);
	//PwmChannel&    get_pwm(uint8_t channel_id);
	Ds18b20Channel& get_ds18b20(uint8_t channel_id);

private:
	template <typename T>
	T& get_channel(uint8_t channel_id);

private:
	typedef std::map<uint8_t, ChannelPtr> ChannelMap;
	ChannelMap channels_;
};


namespace sync
{

inline uint8_t get_can_node_channel_count(CanNode& can_node)
{
	return get<uint8_t, CanNode, &CanNode::query_channel_count>(can_node);
}

inline ChannelType get_can_node_channel_type(CanNode& can_node, uint8_t channel_id)
{
	BlockingResult<uint8_t> result;
	can_node.query_channel_type(channel_id, result);
	const BlockingResult<uint8_t>::ValueType value = result.wait();

	if (!value)
		throw Timeout();
	return static_cast<ChannelType>(*value);
}

}


class SwitchChannel : public ChannelBase
{
public:
	SwitchChannel(CanNode& owner, uint8_t id);
	virtual ~SwitchChannel();

	enum Config
	{
		Config_Polarity = HA_CAN_Node_SwitchConfig_Polarity,
		Config_DefaultOn = HA_CAN_Node_SwitchConfig_DefaultOn,
		Config_OffDelay = HA_CAN_Node_SwitchConfig_OffDelay
	};

	void query_config(Result<uint8_t>& result);
	void assign_config(uint8_t config);

	void query_value(Result<uint8_t>& result);
	void assign_value(uint8_t value);

	void query_off_delay(Result<uint16_t>& result);
	void assign_off_delay(uint16_t value);
};


namespace sync
{

inline uint8_t get_switch_config(SwitchChannel& channel)
{
	return get<uint8_t, SwitchChannel, &SwitchChannel::query_config>(channel);
}

inline uint8_t get_switch_value(SwitchChannel& channel)
{
	return get<uint8_t, SwitchChannel, &SwitchChannel::query_value>(channel);
}

inline uint16_t get_switch_off_delay(SwitchChannel& channel)
{
	return get<uint16_t, SwitchChannel, &SwitchChannel::query_off_delay>(channel);
}

}


class ButtonChannel : public ChannelBase
{
public:
	ButtonChannel(CanNode& owner, uint8_t id);
	virtual ~ButtonChannel();

	typedef std::function<void(uint8_t)> ValueHandler;

	enum Config
	{
		Config_Polarity = HA_CAN_Node_ButtonConfig_Polarity,
		Config_PullUp = HA_CAN_Node_ButtonConfig_PullUp,
		Config_Lock = HA_CAN_Node_ButtonConfig_Lock,
		Config_Active = HA_CAN_Node_ButtonConfig_Active
	};

	void query_config(Result<uint8_t>& result);
	void assign_config(uint8_t config);

	void query_value(Result<uint8_t>& result);
	void assign_value(uint8_t value);

	void subscribe(const ValueHandler& handler, uint32_t filter_options = 0);
};


namespace sync
{

inline uint8_t get_button_config(ButtonChannel& channel)
{
	return get<uint8_t, ButtonChannel, &ButtonChannel::query_config>(channel);
}

inline uint8_t get_button_value(ButtonChannel& channel)
{
	return get<uint8_t, ButtonChannel, &ButtonChannel::query_value>(channel);
}

}


class AnalogChannel : public ChannelBase
{
public:
	AnalogChannel(CanNode& owner, uint8_t id);
	virtual ~AnalogChannel();

	typedef std::function<void(uint16_t)> ValueHandler;

	typedef HA_CAN_Node_AnalogRange Range;

	enum Config
	{
		RangeCheck = HA_CAN_Node_AnalogConfig_RangeCheck,
		ReverseRange = HA_CAN_Node_AnalogConfig_ReverseRange
	};

	void query_config(Result<uint8_t>& result);
	void assign_config(uint8_t config);

	void query_range(Result<Range>& result);
	void assign_range(const Range& range);

	void query_value(Result<uint16_t>& value);

	void subscribe(const ValueHandler& handler, uint32_t filter_options = 0);
};


namespace sync
{

inline uint8_t get_analog_config(AnalogChannel& channel)
{
	return get<uint8_t, AnalogChannel, &AnalogChannel::query_config>(channel);
}

inline AnalogChannel::Range get_analog_range(AnalogChannel& channel)
{
	return get<AnalogChannel::Range, AnalogChannel, &AnalogChannel::query_range>(channel);
}

inline uint16_t get_analog_value(AnalogChannel& channel)
{
	return get<uint16_t, AnalogChannel, &AnalogChannel::query_value>(channel);
}

}


class Ds18b20Channel : public ChannelBase
{
public:
	Ds18b20Channel(CanNode& owner, uint8_t id);
	virtual ~Ds18b20Channel();

	void query_value(Result<Temperature>& result);
};

namespace sync
{

inline Temperature get_ds18b20_temperature(Ds18b20Channel& channel)
{
	return get<Temperature, Ds18b20Channel, &Ds18b20Channel::query_value>(channel);
}

}

}
}
