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
}

#include "util/NonCopyable.h"
#include "util/Pin.h"

#include "board.h"
#include "can_server.h"
#include "can.h"

#define TELNET_PORT 23
#define RAW_PORT 2323


// === CanServer ===

CanServer::CanServer()
{
	static const uint16_t ports[] = { TELNET_PORT, RAW_PORT };

	for (size_t i = 0; i < ServerCount; ++i)
	{
		server_fd_[i] = NDK_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (server_fd_[i] == INVALID_SOCKET)
			return;

		struct ::sockaddr_in local_addr;
		::memset(&local_addr, 0, sizeof(local_addr));
		local_addr.sin_family = AF_INET;
		local_addr.sin_addr.s_addr = 0; //htonl(INADDR_ANY);
		local_addr.sin_port = htons(ports[i]);

		if (NDK_bind(server_fd_[i], reinterpret_cast<struct ::sockaddr*>(&local_addr), sizeof(local_addr)) == SOCKET_ERROR)
			return;

		if (NDK_listen(server_fd_[i], MaxClients) == SOCKET_ERROR)
			return;

		int optval = 1;
		if (NDK_setsockopt(server_fd_[i], SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) == SOCKET_ERROR)
			return;
	}

	SOCKET pipe_handles[2];
	if (::pipe(&pipe_handles[0], &pipe_handles[1]) == SOCKET_ERROR)
		return;

	signal_send_ = pipe_handles[0];
	signal_recv_ = pipe_handles[1];

	started_ = true;
}

CanServer::~CanServer()
{
	for (size_t i = 0; i < ServerCount; ++i)
	{
		if (server_fd_[i] != INVALID_SOCKET)
			fdClose(server_fd_[i]);
	}

	if (signal_send_ != INVALID_SOCKET)
		fdClose(signal_send_);
	if (signal_recv_ != INVALID_SOCKET)
		fdClose(signal_recv_);
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
	items[signal_index].fd = signal_recv_;
	items[server_index[0]].fd = server_fd_[0];
	items[server_index[1]].fd = server_fd_[1];

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
	int socklen = sizeof(client_addr);

	SOCKET client_fd = NDK_accept(server_fd_[server_index], reinterpret_cast<struct ::sockaddr*>(&client_addr), &socklen);
	if (client_fd == INVALID_SOCKET)
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
	NDK_send(client_fd, (void*)busy, ::strlen(busy), 0);
	fdClose(client_fd);
}

void CanServer::handle_signal()
{
	util::Pin(EK_TM4C1294XL_D1).toggle();
	{
		char recv_buf[16];
		NDK_recv(signal_recv_, recv_buf, sizeof(recv_buf), 0);
	}

	CanPacket packet;
	while (CanMaster::instance().receive(packet))
		handle_can_packet(packet);
}

void CanServer::handle_client(size_t client_index)
{
	Client& client = clients_[client_index];

	char message[128];
	const ssize_t received = NDK_recv(client.fd, message, sizeof(message), 0);
	if (received < 0)
	{
		client.clear();
		return;
	}

	if (received == 0 || received >= 128)
		return;
	message[received] = 0;

	CanPacket packet;
	bool request = false, result = false;

	if (!parse_packet(message, packet, request))
	{
		if (client.interactive)
		{
			static const char usage[] = "COMMAND SYNTAX:\r\n\tdev:XX  addr:XXXX  =XXXX...\r\n\tdev:XX  addr:XXXX  ?\r\n> ";
			NDK_send(client.fd, (void*)usage, strlen(usage), 0);
		}
		return;
	}

	if (request)
		result = CanMaster::instance().send_request(packet.id);
	else
		result = CanMaster::instance().send(packet);

	if (client.interactive)
	{
		static const char success[] = "The packet was sent.\r\n> ";
		static const char busy[] = "Busy, try again.\r\n> ";
		const char* reply = result ? success : busy;
		NDK_send(client.fd, (void*)reply, ::strlen(reply), 0);
	}
}

bool CanServer::parse_packet(const char* message, CanPacket& packet, bool& request)
{
	unsigned int dev_id = 0, address = 0;
	if (::sscanf(message, "dev:%x", &dev_id) != 1)
		return false;

	char* ptr = ::strstr(message, "addr:");
	if (ptr == 0 || ::sscanf(ptr, "addr:%x", &address) != 1)
		return false;

	packet.id.device_id = dev_id;
	packet.id.address = address;

	request = ::strchr(ptr, '?') != 0;
	if (request)
		return true;

	ptr = ::strchr(ptr, '=');
	if (ptr == 0)
		return false;
	ptr++;

	unsigned byte = 0;
	char buf[4];
	while (packet.length < 8 && *ptr != 0 && *(ptr + 1) != 0)
	{
		buf[0] = *ptr++;
		buf[1] = *ptr++;
		buf[2] = 0;
		if (::sscanf(buf, "%x", &byte) != 1)
			break;
		packet.data[packet.length++] = byte;
	}

	return true;
}

void CanServer::handle_can_packet(const CanPacket& packet)
{
	char message[128];
	::sprintf(message, "dev:%.2x  addr:%.4x  =", (unsigned)packet.id.device_id, (unsigned)packet.id.address);
	size_t pos = ::strlen(message);

	for (size_t i = 0; i < packet.length; ++i)
	{
		if (pos >= sizeof(message) - 4)
			break;

		::sprintf(message + pos, "%.2x", (unsigned)packet.data[i]);
		pos += 2;
	}

	::strcpy(message + pos, "\r\n");
	pos += 2;

	for (size_t i = 0; i < MaxClients; ++i)
	{
		if (!clients_[i].valid())
			continue;
		ssize_t sent = NDK_send(clients_[i].fd, message, pos, 0); // TODO nonblock?
		if (sent < 0 || (size_t)sent != pos)
			clients_[i].clear();
	}
}

void CanServer::greeting(size_t client_index)
{
	Client& client = clients_[client_index];
	if (!client.interactive)
		return;

	// TODO
	static const char message[] = "Welcome to the HA Master Board!\r\nType 'help' for command list.\r\n\r\n> ";
	NDK_send(client.fd, (void*)message, ::strlen(message), 0);
}

void CanServer::send_signal()
{
	char dummy = 0;
	NDK_send(signal_send_, &dummy, 1, MSG_DONTWAIT);
	util::Pin(EK_TM4C1294XL_D2).toggle();
}


CanServer& CanServer::instance()
{
	static CanServer self;
	return self;
}


// === CanServer::Client ===

CanServer::Client::Client() :
	fd(INVALID_SOCKET)
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

	fdClose(fd);
	fd = INVALID_SOCKET;
}


extern "C"
void tcp_server(UArg, UArg)
{
	CanServer::instance().run();
}
