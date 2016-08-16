#pragma once

#include <string>
#include <cstdint>

#include "proto/Connection.h"


namespace dh
{
namespace proto
{

class SocketConnection : public Connection
{
public:
	static int connect_tcp(const std::string& address, const std::string& service);
	static int connect_unix(const std::string& path);

	SocketConnection(const util::EventLoop& loop, int socket);
	virtual ~SocketConnection();

private:
	void impl_subscribe();
	void impl_unsubscribe();
	void impl_send(const Packet& packet);

	void on_read_ready();
	void on_write_ready();
	void handle_line(const std::string& line);
	void handle_error();

private:
	const int socket_;
	util::IoListener read_listener_, write_listener_;
	std::string read_buffer_;
	std::string write_buffer_;
	bool got_error_ = false;
};

}
}
