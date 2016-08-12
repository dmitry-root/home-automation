#include <sstream>
#include <stdexcept>

#include "util/Assert.h"
#include "proto/Message.h"


namespace dh
{
namespace proto
{

Packet::Packet()
{
}

Packet::~Packet()
{
}

std::string Packet::convert_to_string() const
{
	return impl_convert_to_string();
}

void Packet::assign_from_string(const std::string& data)
{
	impl_assign_from_string(data);
}

PacketType Packet::packet_type() const
{
	return impl_packet_type();
}

/*static*/
PacketPtr Packet::create_packet(const std::string& line)
{
	PacketPtr result;

	if (line.find("dev-q") != std::string::npos)
		result.reset( new DeviceInit() );
	else
		result.reset( new Message() );

	result->assign_from_string(line);
	return result;
}



Message::Message(Type type, DeviceId device_id, Address address, const Body& body) :
    type_(type),
    device_id_(device_id),
    address_(address),
    body_(body)
{
	DH_VERIFY( body_.size() <= 8 );
}

Message::Message(const std::string& line)
{
	deserialize(line);
}

Message::~Message()
{
}

Message& Message::operator = (const std::string& line)
{
	deserialize(line);
	return *this;
}

Message::operator std::string () const
{
	return serialize();
}

Message::Type Message::type() const
{
	return type_;
}

void Message::set_type(Type value)
{
	type_ = value;
}

Message::DeviceId Message::device_id() const
{
	return device_id_;
}

void Message::set_device_id(DeviceId value)
{
	device_id_ = value;
}

Message::Address Message::address() const
{
	return address_;
}

void Message::set_address(Address value)
{
	address_ = value;
}

const Message::Body& Message::body() const
{
	return body_;
}

void Message::set_body(const Body& value)
{
	DH_VERIFY( value.size() <= 8 );
	body_ = value;
}

std::string Message::serialize() const
{
	std::ostringstream ss;

	ss.fill('0');
	ss << "dev:";
	ss.width(2);
	ss << std::hex << static_cast<unsigned>(device_id_) << "  ";

	ss.width(4);
	ss << "addr:" << address_;
	ss.width(0);

	ss << "  ";
	if (type_ == Type_Data)
	{
		ss << ":";
		ss.width(2);
		for (size_t i = 0, n = body_.size(); i < n; ++i)
			ss << static_cast<unsigned>(body_[i]);
	}
	else
	{
		ss << "?";
	}

	return ss.str();
}

void Message::deserialize(const std::string& data)
{
	static const std::string dev = "dev:";
	size_t pos = data.find(dev);
	if (pos == std::string::npos)
		throw std::invalid_argument("message format error: no device id field");
	{
		unsigned dev_id = 0;
		std::istringstream( data.substr(pos + dev.length(), 2) ) >> std::hex >> dev_id;
		device_id_ = dev_id;
	}

	static const std::string addr = "addr:";
	pos = data.find(addr, pos + dev.length());
	if (pos == std::string::npos)
		throw std::invalid_argument("message format error: no address field");
	std::istringstream( data.substr(pos + addr.length(), 4) ) >> std::hex >> address_;

	pos = data.find_first_of("?:", pos + addr.length() + 4);
	if (pos == std::string::npos)
		throw std::invalid_argument("message format error: no body field");
	body_.clear();
	if (data[pos] == '?')
	{
		type_ = Type_Request;
	}
	else
	{
		type_ = Type_Data;
		pos++;

		size_t end = data.find_first_not_of("0123456789abcdefABCDEF", pos);
		if (end == std::string::npos)
			end = data.length();

		const size_t bodylen = end - pos;
		if (bodylen % 2 != 0 || bodylen / 2 > 8)
			throw std::invalid_argument("message format error: invalid body length");

		const size_t size = bodylen / 2;

		for (size_t i = 0; i < size; ++i)
		{
			unsigned byte = 0;
			std::istringstream( data.substr(pos + i*2, 2) ) >> std::hex >> byte;
			body_.push_back(byte);
		}
	}
}

std::string Message::impl_convert_to_string() const
{
	return serialize();
}

void Message::impl_assign_from_string(const std::string& data)
{
	deserialize(data);
}

PacketType Message::impl_packet_type() const
{
	return PacketType_Message;
}



DeviceInit::DeviceInit(DeviceId new_id, DeviceId old_id) :
    new_id_(new_id),
    old_id_(old_id)
{
}

DeviceInit::DeviceInit(const std::string& line)
{
	deserialize(line);
}

DeviceInit::~DeviceInit()
{
}

DeviceInit& DeviceInit::operator = (const std::string& line)
{
	deserialize(line);
}

DeviceInit::operator std::string () const
{
	return serialize();
}

DeviceInit::DeviceId DeviceInit::new_id() const
{
	return new_id_;
}

void DeviceInit::set_new_id(DeviceId value)
{
	new_id_ = value;
}

DeviceInit::DeviceId DeviceInit::old_id() const
{
	return old_id_;
}

void DeviceInit::set_old_id(DeviceId value)
{
	old_id_ = value;
}

std::string DeviceInit::serialize() const
{
	std::ostringstream ss;
	ss << "dev-q:";
	ss.fill('0');
	ss.width(2);
	ss << static_cast<unsigned>(old_id_) << static_cast<unsigned>(new_id_);
	return ss.str();
}

void DeviceInit::deserialize(const std::string& data)
{
	static const std::string devq = "dev-q:";
	if (data.find(devq) != 0)
		throw std::invalid_argument("device init format error: dev-q not found");

	const size_t pos = devq.length();
	const size_t end = pos + 4;
	const std::string msg = data.substr(pos, end);

	if (msg.length() != 4 || msg.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
		throw std::invalid_argument("device init format error: dev-q body error");

	unsigned oid = 0, nid = 0;
	std::istringstream(msg.substr(0,2)) >> std::hex >> oid;
	std::istringstream(msg.substr(2,2)) >> std::hex >> nid;
	old_id_ = oid;
	new_id_ = nid;
}


std::string DeviceInit::impl_convert_to_string() const
{
	return serialize();
}

void DeviceInit::impl_assign_from_string(const std::string& data)
{
	deserialize(data);
}

PacketType DeviceInit::impl_packet_type() const
{
	return PacketType_DeviceInit;
}

}
}
