#pragma once

#include <string>
#include <vector>
#include <list>

#include "util/NonCopyable.h"
#include "util/EventLoop.h"
#include "proto/CommonTypes.h"
#include "proto/SocketServer.h"

#include "Config.h"


namespace dh
{
namespace router
{

class Router : util::NonCopyable
{
public:
	Router(const util::EventLoop& event_loop, const Config& config);
	~Router();

	void start();
	void stop();

private:
	struct NetworkRecord
	{
		explicit NetworkRecord(const Config::Network* network = 0) :
		    network(network) {}

		const Config::Network* network;
		proto::ConnectionPtr connection;
	};
	typedef std::vector<NetworkRecord> NetworkList;

	struct BindingRecord
	{
		explicit BindingRecord(const Config::Binding* binding = 0) :
		    binding(binding) {}

		const Config::Binding* binding;
		std::shared_ptr<proto::SocketServer> server;
	};
	typedef std::vector<BindingRecord> BindingList;

	typedef std::map<uint32_t, proto::ConnectionPtr> ConnectionMap;

	struct FilterRecord
	{
		enum Flags
		{
			Flag_OneTime = 0x01,
			Flag_Reply = 0x02
		};

		explicit FilterRecord(uint32_t connection_id = 0) :
		    connection_id(connection_id) {}

		uint32_t connection_id;
		const Config::Device* device = 0;
		uint32_t address = 0;
		uint32_t address_mask = 0;
		uint32_t flags_ = 0;
	};
	typedef std::list<FilterRecord> FilterList;

	enum SendType
	{
		SendType_Request,
		SendType_Data
	};

private:
	void init_networks();

	void connect_networks();
	void disconnect_networks();
	void update_reconnect_timer();
	void handle_device_packet(const proto::Packet& packet, size_t network_index);
	bool passes_filter(const proto::Message& message, const Config::Network& network, const FilterRecord& filter);
	bool route_to(const proto::Message& message, const Config::Device& from_device, uint32_t connection_id, bool reply);

	void on_reconnect_timeout();
	void on_device_packet_received(proto::PacketPtr packet, size_t network_index);


	void init_bindings();
	void start_bindings();
	void stop_bindings();

	void on_accepted_connection(proto::ConnectionPtr connection);

	void close_connections();
	void close_connection(uint32_t connection_id);

	void on_connection_packet_received(proto::PacketPtr packet, uint32_t connection_id);

	void remove_filters(uint32_t connection_id);
	bool handle_command(const proto::ServiceCommand& command, uint32_t connection_id);

	bool handle_filter(const proto::ServiceCommand& command, uint32_t connection_id);
	bool handle_send(const proto::ServiceCommand& command, uint32_t connection_id, SendType type);

private:
	util::ScopedEventLoop scoped_loop_;
	const Config& config_;
	bool started_ = false;

	NetworkList networks_;
	size_t connected_networks_ = 0;
	util::Timeout reconnect_timeout_;

	BindingList bindings_;

	ConnectionMap connections_;
	uint32_t next_connection_id_ = 0;

	FilterList filters_;
};

}
}
