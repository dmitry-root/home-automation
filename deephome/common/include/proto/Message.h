#pragma once

#include <string>
#include <map>
#include <vector>
#include <cstdint>

#include "proto/CommonTypes.h"


namespace dh
{
namespace proto
{

enum PacketType
{
	PacketType_Message,
	PacketType_DeviceInit,
	PacketType_ServiceCommand
};

class Packet
{
public:
	std::string convert_to_string() const;
	void assign_from_string(const std::string& data);
	PacketType packet_type() const;
	virtual ~Packet();

	static PacketPtr create(const std::string& line);

protected:
	Packet();

private:
	virtual std::string impl_convert_to_string() const = 0;
	virtual void impl_assign_from_string(const std::string& data) = 0;
	virtual PacketType impl_packet_type() const = 0;
};


class Message : public Packet
{
public:
	enum Type
	{
		Type_Request,
		Type_Data
	};

	typedef uint8_t DeviceId;
	typedef uint16_t Address;
	typedef std::vector<uint8_t> Body;

public:
	explicit Message(Type type = Type_Data, DeviceId device_id = 0, Address address = 0, const Body& body = Body());
	Message(const std::string& line);
	virtual ~Message();

	Message& operator = (const std::string& line);
	operator std::string () const;

	Type type() const;
	void set_type(Type value);
	bool is_request() const { return type() == Type_Request; }
	bool is_data() const    { return type() == Type_Data; }

	DeviceId device_id() const;
	void set_device_id(DeviceId value);

	Address address() const;
	void set_address(Address value);

	const Body& body() const;
	void set_body(const Body& value);

private:
	std::string serialize() const;
	void deserialize(const std::string& data);

	virtual std::string impl_convert_to_string() const;
	virtual void impl_assign_from_string(const std::string& data);
	virtual PacketType impl_packet_type() const;

private:
	Type type_ = Type_Request;
	DeviceId device_id_ = 0;
	Address address_ = 0;
	Body body_;
};


class DeviceInit : public Packet
{
public:
	typedef uint8_t DeviceId;

public:
	explicit DeviceInit(DeviceId new_id = 0xff, DeviceId old_id = 0xff);
	DeviceInit(const std::string& line);
	virtual ~DeviceInit();

	DeviceInit& operator = (const std::string& line);
	operator std::string () const;

	DeviceId new_id() const;
	void set_new_id(DeviceId value);

	DeviceId old_id() const;
	void set_old_id(DeviceId value);

private:
	std::string serialize() const;
	void deserialize(const std::string& data);

	virtual std::string impl_convert_to_string() const;
	virtual void impl_assign_from_string(const std::string& data);
	virtual PacketType impl_packet_type() const;

private:
	DeviceId new_id_ = 0xff;
	DeviceId old_id_ = 0xff;
};


class ServiceCommand : public Packet
{
public:
	explicit ServiceCommand(const std::string& action = std::string());
	virtual ~ServiceCommand();

	std::string action() const;
	void set_action(const std::string& value);

	void add_argument(const std::string& key, const std::string& value);
	void add_argument(const std::string& key, unsigned int value);

	std::string get_argument(const std::string& key, const std::string& default_value = std::string()) const;
	unsigned int get_argument(const std::string& key, unsigned int default_value) const;

private:
	std::string serialize() const;
	void deserialize(const std::string& data);

	virtual std::string impl_convert_to_string() const;
	virtual void impl_assign_from_string(const std::string& data);
	virtual PacketType impl_packet_type() const;

private:
	typedef std::map<std::string, std::string> Arguments;
	std::string action_;
	Arguments arguments_;
};


}
}
