#include <algorithm>

#include "util/Assert.h"
#include "proto/SocketConnection.h"
#include "proto/Message.h"

#include "client/Dispatcher.h"


namespace dh
{
namespace client
{

#define DH_LOG_CLASS "Dispatcher"

Dispatcher::Dispatcher(const util::EventLoop& event_loop) :
    scoped_loop_(event_loop)
{
}

Dispatcher::~Dispatcher()
{
	DH_VERIFY( !connection_ );
}

void Dispatcher::start_tcp(const std::string& address, const std::string& service)
{
	const int socket = proto::SocketConnection::connect_tcp(address, service);
	start_internal(socket);
}

void Dispatcher::start_unix(const std::string& socket_filename)
{
	const int socket = proto::SocketConnection::connect_unix(socket_filename);
	start_internal(socket);
}

void Dispatcher::stop()
{
	if (!connection_)
		return;

	connection_->unsubscribe();
	connection_.reset();

	filters_.clear();
	queries_.clear();

	DH_LOG(Info) << "stopped";
}

void Dispatcher::set_event_handler(const EventHandler& handler)
{
	event_handler_ = handler;
}

void Dispatcher::set_unknown_handler(const DeviceCommandHandler& handler)
{
	unknown_handler_ = handler;
}

void Dispatcher::add_filter(const Filter& filter, const DeviceCommandHandler& handler)
{
	DH_VERIFY( connection_ );
	DH_VERIFY( handler );

	proto::PacketPtr packet = filter.service_command();
	connection_->send(*packet);

	filters_.push_back( FilterRecord(filter, handler) );
}

void Dispatcher::send(const DeviceCommand& command)
{
	DH_VERIFY( connection_ );
	DH_VERIFY( command.type() != DeviceCommand::Type_Reply );

	DH_LOG(Info) << "send command to device: " << command.device();

	proto::PacketPtr packet = command.service_command();
	connection_->send(*packet);
}

void Dispatcher::send_query(const DeviceCommand& query, const DeviceCommandHandler& handler, uint32_t timeout_ms)
{
	DH_VERIFY( connection_ );
	DH_VERIFY( query.type() == DeviceCommand::Type_Query );
	DH_VERIFY( handler );

	send(query);

	QueryRecord query_record(query, handler, ++last_query_id_);
	if (timeout_ms != infinite_timeout)
	{
		query_record.timeout = std::make_shared<util::Timeout>(
		            scoped_loop_.event_loop(),
		            timeout_ms,
		            std::bind(&Dispatcher::on_query_timeout, this, query_record.unique_id));
		query_record.timeout->start();
	}

	queries_.push_back(query_record);
}

void Dispatcher::start_internal(int socket)
{
	if (connection_)
		stop();

	DH_LOG(Info) << "start on socket: " << socket;
	connection_ = std::make_shared<proto::SocketConnection>( scoped_loop_.event_loop(), socket );
	connection_->subscribe( std::bind(&Dispatcher::on_packet, this, std::placeholders::_1) );
}

void Dispatcher::close_query(const QueryList::iterator it, const DeviceCommand* response)
{
	if (it->timeout)
	{
		it->timeout->stop();
		it->timeout.reset();
	}

	const DeviceCommandHandler handler = it->handler;
	queries_.erase(it);

	handler(response);
}

void Dispatcher::on_packet(proto::PacketPtr packet)
{
	if (!packet)
	{
		report_event(Event_ConnectionClosed);
		return;
	}

	DH_VERIFY( packet->packet_type() == proto::PacketType_ServiceCommand );
	ServiceCommandPtr service_command = std::static_pointer_cast<proto::ServiceCommand>(packet);

	if (service_command->action() == "ok")
	{
		DH_LOG(Info) << "command succeeded";
	}
	else if (service_command->action() == "fail")
	{
		DH_LOG(Error) << "command failed";
		report_event(Event_CommandFailed);
	}
	else if (service_command->action() == "send" || service_command->action() == "reply")
	{
		const DeviceCommand device_command(*service_command);
		handle_device_command(device_command);
	}
	else
	{
		DH_LOG(Error) << "unknown service command received";
		report_event(Event_UnknownCommand);
	}
}

void Dispatcher::on_query_timeout(uint32_t unique_id)
{
	const QueryList::iterator it = std::find_if(
	            queries_.begin(), queries_.end(),
	            [unique_id](const QueryRecord& r) { return r.unique_id == unique_id; });
	if (it == queries_.end())
		return;

	DH_LOG(Warning) << "timeout waiting for reply from device: " << it->query.device();

	close_query(it, nullptr);
}

void Dispatcher::handle_device_command(const DeviceCommand& device_command)
{
	if (device_command.type() == DeviceCommand::Type_Reply)
	{
		DH_LOG(Info) << "reply received from device: " << device_command.device();

		const QueryList::iterator it = std::find_if(
		            queries_.begin(), queries_.end(),
		            [device_command](const QueryRecord& r) { return r.query.device() == device_command.device() && r.query.address() == device_command.address(); });

		if (it == queries_.end())
		{
			DH_LOG(Warning) << "could not find query that corresponds to received reply, device: " << device_command.device();
			if (unknown_handler_)
				unknown_handler_(&device_command);
			return;
		}

		close_query(it, &device_command);
	}
	else
	{
		DH_LOG(Info) << "status received from device: " << device_command.device();

		const FilterList::iterator it = std::find_if(
		            filters_.begin(), filters_.end(),
		            [device_command](const FilterRecord& r) { return (r.filter.device().empty() || r.filter.device() == device_command.device()) && ((r.filter.address() & r.filter.mask()) == (device_command.address() & r.filter.mask())); });
		if (it == filters_.end())
		{
			DH_LOG(Warning) << "status command did not match any specified filters, device: " << device_command.device();
			if (unknown_handler_)
				unknown_handler_(&device_command);
			return;
		}

		const DeviceCommandHandler handler = it->handler;

		if (it->filter.options() & Filter::Option_OneTime)
			filters_.erase(it);

		handler(&device_command);
	}
}

void Dispatcher::report_event(Event event)
{
	if (event_handler_)
	{
		scoped_loop_.enqueue( std::bind(event_handler_, event) );
		return;
	}

	// Default handling of event
	switch (event)
	{
		case Event_CommandFailed:
			DH_LOG(FatalError) << "execution of previous command failed";
			std::terminate();
			break;

		case Event_ConnectionClosed:
			DH_LOG(FatalError) << "router closed connection";
			std::terminate();
			break;

		case Event_UnknownCommand:
			DH_LOG(FatalError) << "unknown service command received";
			std::terminate();
			break;
	}
}

}
}
