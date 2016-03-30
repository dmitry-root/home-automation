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

#include "board.h"
#include "can_server.h"

#define TELNET_PORT 23
#define RAW_PORT 2323


// === CanServer ===

CanServer::CanServer() :
	signal_send_(-1),
	signal_recv_(-1),
	started_(false)
{
	for (size_t i = 0; i < ServerCount; ++i)
		server_fd_[i] = -1;

	static const uint16_t ports[] = { TELNET_PORT, RAW_PORT };

	for (size_t i = 0; i < ServerCount; ++i)
	{
		server_fd_[i] = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (server_fd_[i] == -1)
			return;

		struct ::sockaddr_in local_addr;
		::memset(&local_addr, 0, sizeof(local_addr));
		local_addr.sin_family = AF_INET;
		local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		local_addr.sin_port = htons(ports[i]);

		if (::bind(server_fd_[i], reinterpret_cast<struct ::sockaddr*>(&local_addr), sizeof(local_addr)) == SOCKET_ERROR)
			return;

		if (::listen(server_fd_[i], MaxClients) == SOCKET_ERROR)
			return;

		int optval = 1;
		if (::setsockopt(server_fd_[i], SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) == SOCKET_ERROR)
			return;
	}

	void* pipe_handles[2];
	if (::pipe(&pipe_handles[0], &pipe_handles[1]) == SOCKET_ERROR)
		return;

	signal_send_ = (int)pipe_handles[0];
	signal_recv_ = (int)pipe_handles[1];

	started_ = true;
}

CanServer::~CanServer()
{
	for (size_t i = 0; i < ServerCount; ++i)
	{
		if (server_fd_[i] != -1)
			::close(server_fd_[i]);
	}

	if (signal_send_ != -1)
		::close(signal_send_);
	if (signal_recv_ != -1)
		::close(signal_recv_);
}

void CanServer::run()
{
	if (!started_)
		return;

	// We use internal fdPoll() function instead of select, to reduce the unnecessary overhead.
	const size_t max_poll_items = ServerCount + MaxClients + 1; // 1 is for signal fd.
	FDPOLLITEM items[max_poll_items];

	const size_t signal_index = 0;
	const size_t server_index[ServerCount] = { 1, 2 };
	const size_t client_index = 3;
	size_t client_map[MaxClients]; // client index -> poll item index

	// Fill poll items that will not change.
	items[signal_index].fd = (void*)signal_recv_;
	items[server_index[0]].fd = (void*)server_fd_[0];
	items[server_index[1]].fd = (void*)server_fd_[1];

	while (true)
	{
		for (size_t i = 0; i < max_poll_items; ++i)
		{
			items[i].eventsRequested = POLLIN;
			items[i].eventsDetected = 0;
		}

		// Fill client poll items.
		size_t poll_count = client_index;
		for (size_t i = 0; i < MaxClients; ++i)
		{
			client_map[i] = 0;
			if (!clients_[i].valid())
				continue;
			items[poll_count].fd = (void*)clients_[i].fd;
			client_map[i] = poll_count++;
		}

		// Poll request
		if (fdPoll(items, poll_count, POLLINFTIM) == SOCKET_ERROR)
			break;

		// Handle result
		if (items[signal_index].eventsDetected & POLLIN)
			handle_signal();

		for (size_t i = 0; i < ServerCount; ++i)
			if (items[server_index[i]].eventsDetected & POLLIN)
				handle_connect(i);

		for (size_t i = 0; i < MaxClients; ++i)
		{
			if (client_map[i] == 0 || !clients_[i].valid())
				continue;
			if (items[client_map[i]].eventsDetected & POLLIN)
				handle_client(i);
		}
	}
}

void CanServer::handle_connect(size_t server_index)
{
	// select() returned server_fd as readable socket. Accept incoming connection on it.
	struct ::sockaddr_in client_addr;
	::socklen_t socklen = sizeof(client_addr);

	int client_fd = ::accept(server_fd_[server_index], reinterpret_cast<struct ::sockaddr*>(&client_addr), &socklen);
	if (client_fd == SOCKET_ERROR)
		return;

	// Find an empty slot for a new connection in a client list.
	for (size_t i = 0; i < MaxClients; ++i)
	{
		if (!clients_[i].valid())
		{
			clients_[i].fd = client_fd;
			clients_[i].interactive = server_index == Server_Interactive;
			greeting(i);
			return;
		}
	}

	// No empty clients found -- reject the connection.
	static const char busy[] = "Busy.\r\n";
	::send(client_fd, busy, ::strlen(busy), 0);
	::close(client_fd);
}

void CanServer::handle_signal()
{
	{
		char recv_buf[16];
		::recv(signal_recv_, recv_buf, sizeof(recv_buf), 0);
	}

	// TODO
}

void CanServer::handle_client(size_t client_index)
{
	// TODO
}

void CanServer::greeting(size_t client_index)
{
	Client& client = clients_[client_index];
	if (!client.interactive)
		return;

	// TODO
	static const char message[] = "Welcome to the HA Master Board!\r\nType 'help' for command list.\r\n\r\n> ";
	::send(client.fd, message, ::strlen(message), 0);
}

void CanServer::send_signal()
{
	char dummy = 0;
	::send(signal_send_, &dummy, 1, 0);
}


CanServer& CanServer::instance()
{
	static CanServer self;
	return self;
}


// === CanServer::Client ===

CanServer::Client::Client() :
	fd(-1)
{
}

CanServer::Client::~Client()
{
	clear();
}

void CanServer::Client::clear()
{
	if (!valid())
		return;

	::close(fd);
	fd = -1;
}


extern "C"
void tcp_server(UArg, UArg)
{
	CanServer::instance().run();
}
