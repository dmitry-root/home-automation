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

	if (line.find("dev-q:") == 0)
		result.reset( new DeviceInit() );
	else if (line.find("cmd:") == 0)
		result.reset( new ServiceCommand() );
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
	ss << std::hex << static_cast<unsigned>(old_id_) << static_cast<unsigned>(new_id_);
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



static bool check_string(const std::string& s)
{
	return s.find_first_not_of("0123456789_-abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") == std::string::npos;
}

ServiceCommand::ServiceCommand(const std::string& action) :
    action_(action)
{
	DH_VERIFY( check_string(action_) );
}

ServiceCommand::~ServiceCommand()
{
}

std::string ServiceCommand::action() const
{
	return action_;
}

void ServiceCommand::set_action(const std::string& value)
{
	DH_VERIFY( check_string(value) );
	action_ = value;
}

void ServiceCommand::add_argument(const std::string& key, const std::string& value)
{
	DH_VERIFY( check_string(key) );
	DH_VERIFY( key != "cmd" );
	DH_VERIFY( check_string(value) );
	arguments_[key] = value;
}

void ServiceCommand::add_argument(const std::string& key, unsigned int value)
{
	const int value_width = (value < 256) ? 2 : ((value < 256*256) ? 4 : 8);
	std::ostringstream ss;
	ss.width(value_width);
	ss.fill('0');
	ss << std::hex << value;
	add_argument(key, ss.str());
}

std::string ServiceCommand::get_argument(const std::string& key, const std::string& default_value) const
{
	const Arguments::const_iterator it = arguments_.find(key);
	return it == arguments_.cend() ? default_value : it->second;
}

unsigned int ServiceCommand::get_argument(const std::string& key, unsigned int default_value) const
{
	const std::string& string_result = get_argument(key);
	if (string_result.empty())
		return default_value;

	unsigned int result = 0;
	std::istringstream ss(string_result);
	ss >> std::hex >> result;
	return ss.bad() ? default_value : result;
}

std::string ServiceCommand::serialize() const
{
	DH_VERIFY( !action_.empty() );

	std::ostringstream ss;
	ss << "cmd:" << action_;

	for (const Arguments::value_type& argument : arguments_)
		ss << " " << argument.first << ":" << argument.second;

	return ss.str();
}

void ServiceCommand::deserialize(const std::string& data)
{
	static const std::string cmd_line = "cmd:";
	static const std::string space = " \t";

	const size_t data_len = data.length();
	const size_t cmd_len = cmd_line.length();

	if (data_len < cmd_len || data.substr(0, cmd_len) != cmd_line)
		throw std::invalid_argument("service command format error: cmd field expected");

	size_t arg_end = data.find_first_of(space, cmd_len);
	if (arg_end == std::string::npos)
		arg_end = data_len;
	const std::string new_action = data.substr(cmd_len, arg_end - cmd_len);
	if (new_action.empty() || !check_string(new_action))
		throw std::invalid_argument("service command format error: wrong cmd field");

	Arguments new_arguments;
	while (arg_end < data_len)
	{
		const size_t arg_begin = data.find_first_not_of(space, arg_end);
		if (arg_begin == std::string::npos)
			break; // space till end of line
		arg_end = data.find_first_of(space, arg_begin);
		if (arg_end == std::string::npos)
			arg_end = data_len;

		const size_t sep = data.find(':', arg_begin);
		if (sep == std::string::npos || sep > arg_end)
			throw std::invalid_argument("service command format error: argument syntax error");

		const std::string key = data.substr(arg_begin, sep - arg_begin);
		const std::string value = data.substr(sep + 1, arg_end - sep - 1);

		if (key.empty() || !check_string(key) || !check_string(value))
			throw std::invalid_argument("service command format error: wrong argument field");

		new_arguments[key] = value;
	}

	action_ = new_action;
	arguments_.swap(new_arguments);
}

std::string ServiceCommand::impl_convert_to_string() const
{
	return serialize();
}

void ServiceCommand::impl_assign_from_string(const std::string& data)
{
	deserialize(data);
}

PacketType ServiceCommand::impl_packet_type() const
{
	return PacketType_ServiceCommand;
}

}
}
