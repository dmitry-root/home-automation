#include "proto/Connection.h"


namespace dh
{
namespace proto
{

Connection::Connection(const util::EventLoop& loop) :
    scoped_loop_(loop)
{
}

Connection::~Connection()
{
}

void Connection::subscribe(const PacketHandler& packet_handler)
{
	if (packet_handler_)
		unsubscribe();

	packet_handler_ = packet_handler;
	impl_subscribe();
}

void Connection::unsubscribe()
{
	if (!packet_handler_)
		return;

	impl_unsubscribe();
	packet_handler_ = 0;
	scoped_loop_.invalidate();
}

void Connection::send(const Packet& packet)
{
	impl_send(packet);
}

util::ScopedEventLoop& Connection::scoped_loop()
{
	return scoped_loop_;
}

void Connection::packet_received(PacketPtr packet)
{
	if (packet_handler_)
		scoped_loop_.enqueue( std::bind(packet_handler_, packet) );
}

}
}
