#include "client/CanNode.h"


namespace dh
{
namespace client
{

ChannelBase::ChannelBase(CanNode& owner, ChannelType type, uint8_t id) :
    owner_(owner),
    type_(type),
    id_(id)
{
}

ChannelBase::~ChannelBase()
{
}

ChannelType ChannelBase::type() const
{
	return type_;
}

uint8_t ChannelBase::id() const
{
	return id_;
}


CanNode::CanNode(Dispatcher& dispatcher, const std::string& name) :
    Device(dispatcher, name)
{
}

CanNode::~CanNode()
{
}

void CanNode::save()
{
	const DeviceCommand command(name(), DeviceCommand::Type_Send, HA_CAN_Node_ChannelSaveConfig);
	dispatcher().send(command);
}

void CanNode::query_channel_count(Result<uint8_t>& result)
{
	query(HA_CAN_Node_ChannelCount, result);
}

void CanNode::query_channel_type(uint8_t channel_id, Result<uint8_t>& result)
{
	query(HA_CAN_Node_function_address(channel_id), result);
}

void CanNode::assign_channel_type(uint8_t channel_id, ChannelType type)
{
	send(HA_CAN_Node_function_address(channel_id), static_cast<uint8_t>(type));
}

SwitchChannel& CanNode::get_switch(uint8_t channel_id)
{
	return get_channel<SwitchChannel>(channel_id);
}
/*
ButtonChannel& CanNode::get_button(uint8_t channel_id)
{
	return get_channel<ButtonChannel>(channel_id);
}

AnalogChannel& CanNode::get_analog(uint8_t channel_id)
{
	return get_channel<AnalogChannel>(channel_id);
}

PwmChannel& CanNode::get_pwm(uint8_t channel_id)
{
	return get_channel<PwmChannel>(channel_id);
}

Ds18b20Channel& CanNode::get_ds18b20(uint8_t channel_id)
{
	return get_channel<Ds18b20Channel>(channel_id);
}
*/
template <typename T>
T& CanNode::get_channel(uint8_t channel_id)
{
	ChannelMap::iterator it = channels_.find(channel_id);
	if (it == channels_.end())
	{
		std::shared_ptr<T> result = std::make_shared<T>(*this, channel_id);
		channels_.insert( std::make_pair(channel_id, std::static_pointer_cast<ChannelBase, T>(result) ) );
		return *result;
	}

	return *dynamic_cast<T*>( it->second.get() );
}



SwitchChannel::SwitchChannel(CanNode& owner, uint8_t id) :
    ChannelBase(owner, ChannelType_Switch, id)
{
}

SwitchChannel::~SwitchChannel()
{
}

void SwitchChannel::query_config(Result<uint8_t>& result)
{
	owner().query(field_address(HA_CAN_Node_Switch_Config), result);
}

void SwitchChannel::assign_config(uint8_t config)
{
	owner().send(field_address(HA_CAN_Node_Switch_Config), config);
}

void SwitchChannel::query_value(Result<uint8_t>& result)
{
	owner().query(field_address(HA_CAN_Node_Switch_Value), result);
}

void SwitchChannel::assign_value(uint8_t value)
{
	owner().send(field_address(HA_CAN_Node_Switch_Value), value);
}

void SwitchChannel::query_off_delay(Result<uint16_t>& result)
{
	owner().query(field_address(HA_CAN_Node_Switch_OffDelay), result);
}

void SwitchChannel::assign_off_delay(uint16_t value)
{
	owner().send(field_address(HA_CAN_Node_Switch_OffDelay), value);
}

void SwitchChannel::subscribe(const ValueHandler& handler, uint32_t filter_options)
{
	owner().subscribe(field_address(HA_CAN_Node_Switch_Value), handler, filter_options);
}

}
}
