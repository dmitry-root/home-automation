#pragma once

#include <string>
#include <memory>
#include <list>

#include "util/NonCopyable.h"
#include "util/EventLoop.h"
#include "proto/CommonTypes.h"
#include "client/Command.h"
#include "client/Result.h"
#include "client/Serializer.h"


namespace dh
{
namespace client
{

// NOTE nullptr argument means that reply timeout occured.
typedef std::function< void(const DeviceCommand*) > DeviceCommandHandler;


class Dispatcher : util::NonCopyable
{
public:
	enum Event
	{
		Event_CommandFailed,
		Event_ConnectionClosed,
		Event_UnknownCommand
	};
	typedef std::function< void(Event) > EventHandler;

	Dispatcher(const util::EventLoop& event_loop);
	~Dispatcher();

	void start_tcp(const std::string& address = "127.0.0.1", const std::string& service = "2020");
	void start_unix(const std::string& socket_filename);
	void stop();

	void set_event_handler(const EventHandler& handler);
	void set_unknown_handler(const DeviceCommandHandler& handler);

	void add_filter(const Filter& filter, const DeviceCommandHandler& handler);

	void send(const DeviceCommand& command);
	void send_query(const DeviceCommand& query, const DeviceCommandHandler& handler, uint32_t timeout_ms = infinite_timeout);

	template <typename T>
	void send_query(const DeviceCommand& query, Result<T>& result, uint32_t timeout_ms = infinite_timeout)
	{
		const DeviceCommandHandler handler =
		        [&result](const DeviceCommand* command) { T value = T(); if (command && command->get_value<T>(value)) result.set(&value); else result.set(nullptr); };
		send_query(query, handler, timeout_ms);
	}

private:
	struct FilterRecord
	{
		explicit FilterRecord(const Filter& filter = Filter(), const DeviceCommandHandler& handler = DeviceCommandHandler()) :
		    filter(filter), handler(handler) {}

		Filter filter;
		DeviceCommandHandler handler;
	};

	struct QueryRecord
	{
		explicit QueryRecord(const DeviceCommand& query = DeviceCommand(), const DeviceCommandHandler& handler = DeviceCommandHandler(), uint32_t unique_id = 0) :
		    query(query), handler(handler), unique_id(unique_id) {}

		DeviceCommand query;
		DeviceCommandHandler handler;
		uint32_t unique_id;
		std::shared_ptr<util::Timeout> timeout;
	};

	typedef std::list<FilterRecord> FilterList;
	typedef std::list<QueryRecord> QueryList;

private:
	void start_internal(int socket);
	void close_query(const QueryList::iterator it, const DeviceCommand* response);
	void on_packet(proto::PacketPtr packet);
	void on_query_timeout(uint32_t unique_id);

	void handle_device_command(const DeviceCommand& device_command);
	void report_event(Event event);

private:
	util::ScopedEventLoop scoped_loop_;
	proto::ConnectionPtr connection_;

	EventHandler event_handler_;
	DeviceCommandHandler unknown_handler_;
	FilterList filters_;
	QueryList queries_;
	uint32_t last_query_id_ = 0;
};

}
}
