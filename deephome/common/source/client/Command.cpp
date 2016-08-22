#include "proto/Message.h"
#include "client/Command.h"
#include "util/Assert.h"


namespace dh
{
namespace client
{

DeviceCommand::DeviceCommand()
{
}

DeviceCommand::DeviceCommand(const proto::ServiceCommand& service_command)
{
	set_from_service_command(service_command);
}

DeviceCommand::DeviceCommand(const std::string& device, Type type, uint16_t address, const Body& body) :
    device_(device),
    type_(type),
    address_(address),
    body_(body)
{
}

DeviceCommand::~DeviceCommand()
{
}

ServiceCommandPtr DeviceCommand::service_command() const
{
	ServiceCommandPtr result = std::make_shared<proto::ServiceCommand>();

	switch (type_)
	{
		case Type_Send : result->set_action("send"); break;
		case Type_Query: result->set_action("query"); break;
		case Type_Reply: result->set_action("reply"); break;
		default: DH_VERIFY(false);
	}

	result->add_argument("device", device_);
	result->add_argument("address", static_cast<uint32_t>(address_));

	if (type_ != Type_Query)
		result->add_raw_argument("data", body_);
	else
		DH_VERIFY( body_.empty() );
}

void DeviceCommand::set_from_service_command(const proto::ServiceCommand& service_command)
{
	if (service_command.action() == "send")
		type_ = Type_Send;
	else if (service_command.action() == "query")
		type_ = Type_Query;
	else if (service_command.action() == "reply")
		type_ = Type_Reply;
	else
		DH_VERIFY(false);

	device_ = service_command.get_argument("device");
	DH_VERIFY( !device_.empty() );

	static const uint32_t invalid_address = -1;
	const uint32_t raw_address = service_command.get_argument("address", invalid_address);
	DH_VERIFY( raw_address != invalid_address );
	DH_VERIFY( raw_address <= 0xffff );
	address_ = raw_address;

	if (type_ != Type_Query)
	{
		service_command.get_raw_argument("data", body_);
		DH_VERIFY( body_.size() <= 8 );
	}
}

std::string DeviceCommand::device() const
{
	return device_;
}

void DeviceCommand::set_device(const std::string& value)
{
	device_ = value;
}

DeviceCommand::Type DeviceCommand::type() const
{
	return type_;
}

void DeviceCommand::set_type(Type type)
{
	type_ = type;
}

uint16_t DeviceCommand::address() const
{
	return address_;
}

void DeviceCommand::set_address(uint16_t address)
{
	address_ = address;
}

void DeviceCommand::get_body(Body& body) const
{
	body = body_;
}

void DeviceCommand::set_body(const Body& body)
{
	DH_VERIFY(body.size() <= 8);
	body_ = body;
}



Filter::Filter(const std::string& device, uint16_t address, uint16_t mask, uint32_t options) :
    device_(device),
    address_(address),
    mask_(mask),
    options_(options)
{
}

Filter::~Filter()
{
}

ServiceCommandPtr Filter::service_command() const
{
	ServiceCommandPtr result = std::make_shared<proto::ServiceCommand>("filter");

	if (!device_.empty())
		result->add_argument("device", device_);
	result->add_argument("address", static_cast<uint32_t>(address_));
	result->add_argument("mask", static_cast<uint32_t>(mask_));

	if (options_ & Option_OneTime)
		result->add_argument("options", "one-time");

	return result;
}

std::string Filter::device() const
{
	return device_;
}

void Filter::set_device(const std::string& value)
{
	device_ = value;
}

uint16_t Filter::address() const
{
	return address_;
}

void Filter::set_address(uint16_t address)
{
	address_ = address;
}

uint16_t Filter::mask() const
{
	return mask_;
}

void Filter::set_mask(uint16_t mask)
{
	mask_ = mask;
}

uint32_t Filter::options() const
{
	return options_;
}

void Filter::set_options(uint32_t options)
{
	options_ = options;
}

}
}
