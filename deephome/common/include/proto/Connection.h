#pragma once

#include "util/EventLoop.h"
#include "proto/CommonTypes.h"


namespace dh
{
namespace proto
{

class Connection : util::NonCopyable
{
public:
	typedef std::function< void(PacketPtr) > PacketHandler;

	void subscribe(const PacketHandler& packet_handler);
	void unsubscribe();

	void send(const Packet& packet);

	virtual ~Connection();

protected:
	Connection(const util::EventLoop& loop);

	util::ScopedEventLoop& scoped_loop();
	void packet_received(PacketPtr packet);

private:
	virtual void impl_subscribe() {}
	virtual void impl_unsubscribe() {}
	virtual void impl_send(const Packet& packet) = 0;

private:
	util::ScopedEventLoop scoped_loop_;
	PacketHandler packet_handler_;
};

}
}
