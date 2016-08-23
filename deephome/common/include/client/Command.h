#pragma once

#include <vector>

#include "proto/CommonTypes.h"
#include "client/CommonTypes.h"
#include "client/Serializer.h"


namespace dh
{
namespace client
{

typedef std::shared_ptr<proto::ServiceCommand> ServiceCommandPtr;

class DeviceCommand
{
public:
	enum Type
	{
		Type_Send,
		Type_Query,
		Type_Reply
	};

	DeviceCommand();
	explicit DeviceCommand(const proto::ServiceCommand& service_command);
	explicit DeviceCommand(const std::string& device, Type type = Type_Send, uint16_t address = 0, const Body& body = Body());
	~DeviceCommand();

	ServiceCommandPtr service_command() const;
	void set_from_service_command(const proto::ServiceCommand& service_command);

	std::string device() const;
	void set_device(const std::string& value);

	Type type() const;
	void set_type(Type type);

	uint16_t address() const;
	void set_address(uint16_t address);

	void get_body(Body& body) const;
	void set_body(const Body& body);

	template <typename T>
	void set_value(const T& value)
	{
		Body body;
		Serializer<T>::serialize(value, body);
		set_body(body);
	}

	template <typename T>
	bool get_value(T& value) const
	{
		Body body;
		get_body(body);
		return Serializer<T>::deserialize(body, value);
	}

private:
	std::string device_;
	Type type_ = Type_Send;
	uint16_t address_ = 0;
	Body body_;
};

class Filter
{
public:
	enum Options
	{
		Option_OneTime = 0x01
	};

	Filter() = default;
	Filter(const std::string& device, uint16_t address, uint16_t mask, uint32_t options = 0);
	~Filter();

	ServiceCommandPtr service_command() const;

	std::string device() const;
	void set_device(const std::string& value);

	uint16_t address() const;
	void set_address(uint16_t address);

	uint16_t mask() const;
	void set_mask(uint16_t mask);

	uint32_t options() const;
	void set_options(uint32_t options);

private:
	std::string device_;
	uint16_t address_ = 0;
	uint16_t mask_ = 0;
	uint32_t options_ = 0;
};

}
}
