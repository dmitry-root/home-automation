#pragma once

#include "util/NonCopyable.h"
#include "util/EventLoop.h"
#include "proto/CommonTypes.h"


namespace dh
{
namespace proto
{

class SocketServer : util::NonCopyable
{
public:
	typedef std::function< void(ConnectionPtr) > AcceptHandler;

	static int listen_tcp(const std::string& address, const std::string& service, int backlog = 5);
	static int listen_unix(const std::string& path, int backlog = 5);

	SocketServer(const util::EventLoop& loop, int socket, const AcceptHandler& accept_handler);
	~SocketServer();

	void start();
	void stop();

private:
	void on_accept_ready();

private:
	util::ScopedEventLoop scoped_loop_;
	const int socket_;
	util::IoListener socket_listener_;
	const AcceptHandler accept_handler_;
	bool started_ = false;
};

}
}
