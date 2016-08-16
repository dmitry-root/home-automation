#include <algorithm>
#include <limits>
#include <set>

#include "proto/SocketConnection.h"
#include "proto/Message.h"
#include "util/EventLoop.h"
#include "util/Assert.h"

#include "Router.h"


namespace dh
{
namespace router
{

static const uint32_t reconnect_timeout_ms = 30000;

#define DH_LOG_CLASS "Router"


Router::Router(const util::EventLoop& event_loop, const Config& config) :
    scoped_loop_(event_loop),
    config_(config),
    reconnect_timeout_(event_loop, reconnect_timeout_ms, std::bind(&Router::on_reconnect_timeout, this))
{
	init_bindings();
	init_networks();
}

Router::~Router()
{
	DH_VERIFY(!started_);
}

void Router::start()
{
	if (started_)
	{
		DH_LOG(Warning) << "start called while already started";
		return;
	}

	start_bindings();
	connect_networks();

	started_ = true;
}

void Router::stop()
{
	if (!started_)
		return;

	disconnect_networks();
	stop_bindings();
	close_connections();

	scoped_loop_.invalidate();
	started_ = false;
}


// === Device networks handling ===

void Router::init_networks()
{
	Config::NetworkList config_networks;
	config_.get_networks(config_networks);
	DH_VERIFY( !config_networks.empty() );

	for (const Config::Network* network : config_networks)
		networks_.push_back( NetworkRecord(network) );
}

void Router::connect_networks()
{
	if (connected_networks_ >= networks_.size())
		return;

	DH_LOG(Info) << connected_networks_ << " of " << networks_.size() << " device networks are currently connected, trying to connect others...";

	for (size_t i = 0, n = networks_.size(); i < n; ++i)
	{
		NetworkRecord& network = networks_[i];

		if (network.connection)
			continue;

		try
		{
			DH_LOG(Info) << "connecting device network " << network.network->name << ": " << network.network->address << ":" << network.network->service;
			const int socket = proto::SocketConnection::connect_tcp(network.network->address, network.network->service);

			DH_LOG(Info) << "device network " << network.network->name << " successfully connected.";
			network.connection = std::make_shared<proto::SocketConnection>(scoped_loop_.event_loop(), socket);
		}
		catch (const std::runtime_error& e)
		{
			DH_LOG(Warning) << "network " << network.network->name << " connection failed (" << e.what() << "), to be retried later";
			continue;
		}

		using namespace std::placeholders;
		network.connection->subscribe( std::bind(&Router::on_device_packet_received, this, _1, i) );

		++connected_networks_;
	}

	update_reconnect_timer();
}

void Router::disconnect_networks()
{
	reconnect_timeout_.stop();

	for (NetworkRecord& network : networks_)
	{
		network.connection->unsubscribe();
		network.connection.reset();
	}

	connected_networks_ = 0;
}

void Router::update_reconnect_timer()
{
	if (connected_networks_ < networks_.size())
		reconnect_timeout_.start();
}

void Router::on_reconnect_timeout()
{
	if (!started_)
		return;
	connect_networks();
}

void Router::on_device_packet_received(proto::PacketPtr packet, size_t network_index)
{
	if (!started_)
		return;
	NetworkRecord& network = networks_[network_index];

	if (!packet)
	{
		DH_LOG(Warning) << "device network " << network.network->name << " disconnected, to be reconnected";
		network.connection->unsubscribe();
		network.connection.reset();
		--connected_networks_;
		update_reconnect_timer();
		remove_reply_filters(network.network->name);
		return;
	}

	handle_device_packet(*packet, network_index);
}

void Router::handle_device_packet(const proto::Packet& packet, size_t network_index)
{
	if (packet.packet_type() != proto::PacketType_Message)
	{
		DH_LOG(Warning) << "packet with type: " << packet.packet_type() << " received from device, ignored";
		return;
	}

	DH_VERIFY(network_index < networks_.size());
	const NetworkRecord& network_record = networks_[network_index];
	const proto::Message& message = static_cast<const proto::Message&>(packet);

	const std::string& network_name = network_record.network->name;
	const Config::Device* device = config_.find_device(network_name, message.device_id());
	if (!device)
	{
		DH_LOG(Warning) << "packet received from unknown device, network: " << network_name << ", device id: " << (unsigned)message.device_id();
		return;
	}

	std::set<uint32_t> dead_connections;

	for (FilterList::iterator it = filters_.begin(); it != filters_.end();)
	{
		if (!passes_filter(message, *network_record.network, *it))
		{
			++it;
			continue;
		}

		const uint32_t connection_id = it->connection_id;
		const bool reply = (it->flags_ & FilterRecord::Flag_Reply) != 0;
		const bool routed = route_to(message, *device, connection_id, reply);

		if (!routed)
			dead_connections.insert(connection_id);
		else if (reply || (it->flags_ & FilterRecord::Flag_OneTime) != 0)
			it = filters_.erase(it);
		else
			++it;
	}

	for (uint32_t connection_id : dead_connections)
		close_connection(connection_id);
}

bool Router::passes_filter(const proto::Message& message, const Config::Network& network, const FilterRecord& filter)
{
	const uint16_t address = message.address();
	if (address & filter.address_mask != filter.address)
		return false;

	if (filter.device)
	{
		const uint8_t device_id = message.device_id();
		if (filter.device->id != device_id || filter.device->network_name != network.name)
			return false;
	}

	return true;
}

bool Router::route_to(const proto::Message& message, const Config::Device& from_device, uint32_t connection_id, bool reply)
{
	ConnectionMap::iterator it = connections_.find(connection_id);
	if (it == connections_.end())
	{
		DH_LOG(Warning) << "removing filter for absent connection: " << connection_id;
		return false;
	}

	proto::ServiceCommand command(reply ? "reply" : "send");
	command.add_argument("device", from_device.name);
	command.add_argument("network", from_device.network_name);
	command.add_argument("address", message.address());
	command.add_raw_argument("data", message.body());

	try
	{
		it->second->send(command);
	}
	catch (const std::range_error&)
	{
		DH_LOG(Error) << "connection send buffer exhausted: " << connection_id;
		return false;
	}

	return true;
}


// === Bindings handling ===

void Router::init_bindings()
{
	Config::BindingList bindings;
	config_.get_bindings(bindings);
	DH_VERIFY(!bindings.empty());

	for (const Config::Binding* binding : bindings)
		bindings_.push_back( BindingRecord(binding) );
}

void Router::start_bindings()
{
	for (BindingRecord& binding : bindings_)
	{
		int socket = -1;

		try
		{
			if (binding.binding->type == Config::Binding::Type_TCP)
				socket = proto::SocketServer::listen_tcp(binding.binding->address, binding.binding->service, binding.binding->backlog);
			else
				socket = proto::SocketServer::listen_unix(binding.binding->address, binding.binding->backlog);
		}
		catch (const std::runtime_error& e)
		{
			DH_LOG(FatalError) << "could not bind address: " << binding.binding->address << ", service: " << binding.binding->service << " - " << e.what();
			throw std::runtime_error("binding to address failed");
		}

		using namespace std::placeholders;
		binding.server = std::make_shared<proto::SocketServer>(
		            scoped_loop_.event_loop(),
		            socket,
		            std::bind(&Router::on_accepted_connection, this, _1));
		binding.server->start();

		DH_LOG(Info) << "listening on address: " << binding.binding->address << ", service: " << binding.binding->service;
	}
}

void Router::stop_bindings()
{
	for (BindingRecord& binding : bindings_)
	{
		binding.server->stop();
		binding.server.reset();
	}
}

void Router::on_accepted_connection(proto::ConnectionPtr connection)
{
	if (!started_)
		return;

	const uint32_t id = ++next_connection_id_;

	DH_LOG(Info) << "accepted new connection, id: " << id;

	connections_.insert( std::make_pair(id, connection) );

	using namespace std::placeholders;
	connection->subscribe( std::bind(&Router::on_connection_packet_received, this, _1, id) );
}

void Router::close_connections()
{
	for (ConnectionMap::value_type& item : connections_)
		item.second->unsubscribe();
	connections_.clear();
	filters_.clear();
}

void Router::close_connection(uint32_t connection_id)
{
	remove_filters(connection_id);

	ConnectionMap::iterator it = connections_.find(connection_id);
	if (it == connections_.end())
		return;
	it->second->unsubscribe();
	connections_.erase(it);
}


// == Connections handling ==

void Router::on_connection_packet_received(proto::PacketPtr packet, uint32_t connection_id)
{
	if (!started_)
		return;

	if (packet->packet_type() != proto::PacketType_ServiceCommand)
	{
		DH_LOG(Warning) << "wrong packet type: " << packet->packet_type() << " received from connection: " << connection_id;
		return;
	}

	ConnectionMap::iterator it = connections_.find(connection_id);
	if (it == connections_.end())
	{
		DH_LOG(Warning) << "packet received from absent connection: " << connection_id;
		return;
	}

	if (!packet)
	{
		DH_LOG(Warning) << "connection terminated, id: " << connection_id;
		close_connection(connection_id);
		return;
	}

	const proto::ServiceCommand& command = static_cast<const proto::ServiceCommand&>(*packet);
	DH_LOG(Info) << "received command: " << command.action() << " from connection: " << connection_id;

	const bool result = handle_command(command, connection_id);
	if (!result)
		DH_LOG(Warning) << "command handling failed, connection: " << connection_id;

	proto::ServiceCommand reply(result ? "ok" : "fail");
	reply.add_argument("command", command.action());

	try
	{
		it->second->send(reply);
	}
	catch (const std::range_error&)
	{
		DH_LOG(Error) << "connection send buffer exhausted: " << connection_id;
		close_connection(connection_id);
	}
}

void Router::remove_filters(uint32_t connection_id)
{
	auto it = std::remove_if(filters_.begin(), filters_.end(),
	                         [connection_id](const FilterRecord& r) { return r.connection_id == connection_id; });
	filters_.erase(it, filters_.end());
}

void Router::remove_reply_filters(const std::string& network_name)
{
	auto it = std::remove_if(filters_.begin(), filters_.end(),
	                         [&network_name](const FilterRecord& r) { return (r.flags_ & FilterRecord::Flag_Reply) != 0 && r.device->network_name == network_name; });
	filters_.erase(it, filters_.end());
}

bool Router::handle_command(const proto::ServiceCommand& command, uint32_t connection_id)
{
	const std::string& action = command.action();

	if (action == "filter")
		return handle_filter(command, connection_id);
	else if (action == "send")
		return handle_send(command, connection_id, SendType_Data);
	else if (action == "query")
		return handle_send(command, connection_id, SendType_Request);
	else
	{
		DH_LOG(Warning) << "unknown command: " << action << " received from connection: " << connection_id;
		return false;
	}
}

bool Router::handle_filter(const proto::ServiceCommand& command, uint32_t connection_id)
{
	FilterRecord filter(connection_id);

	const std::string& device_name = command.get_argument("device");
	if (!device_name.empty())
	{
		const Config::Device* device = config_.find_device(device_name);
		if (!device)
		{
			DH_LOG(Warning) << "trying to create filter on unknown device: " << device_name << ", connection: " << connection_id;
			return false;
		}

		filter.device = device;
	}

	static const uint32_t invalid_value = std::numeric_limits<uint32_t>::max();
	const uint32_t address = command.get_argument("address", invalid_value);
	const uint32_t mask = command.get_argument("mask", invalid_value);

	if (address != invalid_value && mask != invalid_value)
	{
		if (address > 0xffff || mask > 0xffff)
		{
			DH_LOG(Warning) << "trying to create filter with invalid address or mask, connection: " << connection_id;
			return false;
		}

		filter.address = address & mask;
		filter.address_mask = mask;
	}

	const std::string& options = command.get_argument("options");
	if (options.find("one-time") != std::string::npos)
		filter.flags_ |= FilterRecord::Flag_OneTime;

	filters_.push_back(filter);
}

bool Router::handle_send(const proto::ServiceCommand& command, uint32_t connection_id, SendType type)
{
	const std::string& device_name = command.get_argument("device");
	const Config::Device* device = config_.find_device(device_name);
	if (!device)
	{
		DH_LOG(Warning) << "send to unknown device: " << device_name << ", connection: " << connection_id;
		return false;
	}

	auto network_it = std::find_if(networks_.begin(), networks_.end(),
	                               [device](const NetworkRecord& r) { return r.network == &device->network; });
	DH_VERIFY(network_it != networks_.end());
	if (!network_it->connection)
	{
		DH_LOG(Warning) << "send to network that is currently down, device: " << device_name <<
		                   ", network: " << device->network_name << ", connection: " << connection_id;
		return false;
	}

	proto::Connection& connection = *network_it->connection;

	const uint32_t address = command.get_argument("address", std::numeric_limits<uint32_t>::max());
	if (address > 0xffff)
	{
		DH_LOG(Warning) << "send to invalid address, connection: " << connection_id;
		return false;
	}

	proto::Message message;
	message.set_type( type == SendType_Data ? proto::Message::Type_Data : proto::Message::Type_Request );
	message.set_device_id(device->id);
	message.set_address(address);

	if (type == SendType_Data)
	{
		proto::Message::Body data;
		if (!command.get_raw_argument("data", data) || data.size() > 8)
		{
			DH_LOG(Warning) << "packet data not specified or invalid, connection: " << connection_id;
			return false;
		}
		message.set_body(data);
	}

	try
	{
		connection.send(message);
	}
	catch (const std::range_error& e)
	{
		DH_LOG(Error) << "write buffer exceeded, network: " << device->network_name;
		return false;
	}

	if (type == SendType_Request)
	{
		// Create filter for handling response
		FilterRecord filter(connection_id);
		filter.device = device;
		filter.address = address;
		filter.address_mask = 0xffff;
		filter.flags_ = FilterRecord::Flag_Reply;
		filters_.push_back(filter);
	}

	return true;
}

}
}
