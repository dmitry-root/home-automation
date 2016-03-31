#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/GPIO.h>

extern "C" {
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
}

#include "util/NonCopyable.h"
#include "onewire/onewire.h"

#include "board.h"

#define TELNET_PORT 23


class CanServer : util::NonCopyable
{
public:
	CanServer();
	~CanServer();

	void run();

private:
	void handle_connection(int client_fd);

private:
	int server_fd_;
	bool started_;
};


CanServer::CanServer() :
	server_fd_(-1),
	started_(false)
{
	server_fd_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_fd_ == -1)
		return;

	struct ::sockaddr_in local_addr;
	::memset(&local_addr, 0, sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(TELNET_PORT);

	if (::bind(server_fd_, reinterpret_cast<struct ::sockaddr*>(&local_addr), sizeof(local_addr)) == -1)
		return;

	if (::listen(server_fd_, 1) == -1)
		return;

	int optval = 1;
	if (::setsockopt(server_fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0)
		return;

	started_ = true;
}

CanServer::~CanServer()
{
	if (server_fd_ != -1)
		::close(server_fd_);
}

void CanServer::run()
{
	if (!started_)
		return;

	struct ::sockaddr_in client_addr;
	::socklen_t socklen = sizeof(client_addr);

	int client_fd = -1;
	while ((client_fd = ::accept(server_fd_, reinterpret_cast<struct ::sockaddr*>(&client_addr), &socklen)) != -1)
	{
		handle_connection(client_fd);
		::close(client_fd);
	}
}

static void send_str(int fd, const char* str)
{
	::send(fd, str, ::strlen(str), 0);
}

void CanServer::handle_connection(int client_fd)
{
	send_str(client_fd, "Measuring temperature...\n");

	util::Pin rx(Board_OW_RX), tx(Board_OW_TX);
	onewire::Master master(rx, tx);

	onewire::ThermometerSession session(master);
	send_str(client_fd, "Issue RESET...\n");
	if (!session.reset())
	{
		send_str(client_fd, "No PD pulse :(\n");
		return;
	}

	onewire::UniqueId id = 0;
	session.read_rom(id);

	char tmp[64];
	sprintf(tmp, "uid = 0x%llx\n", id);
	send_str(client_fd, tmp);
	return;

	session.skip_rom();

	send_str(client_fd, "CONVERT T\n");
	session.convert_t(onewire::ThermometerSession::Wait);

	send_str(client_fd, "Done, now reading temperature\n");

	if (!session.reset())
	{
		send_str(client_fd, "No PD pulse :(\n");
		return;
	}

	session.skip_rom();

	onewire::Temperature t = session.read_t();

	char buf[48];
	::sprintf(buf, "T = %f\n", t.fp_value());
	send_str(client_fd, buf);
}


extern "C"
void tcp_server(UArg, UArg)
{
	CanServer().run();
}
