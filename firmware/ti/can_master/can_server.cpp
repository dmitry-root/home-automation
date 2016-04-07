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

	started_ = true;
}

CanServer::~CanServer()
{
	for (size_t i = 0; i < ServerCount; ++i)
	{
		if (server_fd_[i] != INVALID_SOCKET)
			fdClose(server_fd_[i]);
	}
}

void CanServer::run()
{
	if (!started_)
		return;

	task_handle_ = ti_sysbios_knl_Task_self();

	fd_set rdset;
	while (true)
	{
		NDK_FD_ZERO(&rdset);

		for (size_t i = 0; i < ServerCount; ++i)
			NDK_FD_SET(server_fd_[i], &rdset);
		for (size_t i = 0; i < MaxClients; ++i)
			if (clients_[i].valid())
				NDK_FD_SET(clients_[i].fd, &rdset);

		// Perform wait operation
		if (fdSelect(0, &rdset, 0, 0, 0) == SOCKET_ERROR)
			break;

		handle_signal();

		for (size_t i = 0; i < ServerCount; ++i)
			if (NDK_FD_ISSET(server_fd_[i], &rdset))
				handle_connect(i);

		for (size_t i = 0; i < MaxClients; ++i)
		{
			if (clients_[i].valid() && NDK_FD_ISSET(clients_[i].fd, &rdset))
				handle_client(i);
		}
	}
	task_handle_ = 0;
	util::Pin(EK_TM4C1294XL_D2).toggle();
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
	NDK_send(client_fd, (void*)busy, ::strlen(busy), MSG_WAITALL);
	fdClose(client_fd);
}

void CanServer::handle_signal()
{
	util::Pin(EK_TM4C1294XL_D1).toggle();
	CanPacket packet;
	while (CanMaster::instance().receive(packet))
		handle_can_packet(packet);
}

int CanServer::send_to_client(size_t client_index, const char* message, size_t length)
{
	return NDK_send(clients_[client_index].fd, (void*)message, length == (size_t)-1 ? ::strlen(message) : length, MSG_WAITALL);
}

void CanServer::handle_client(size_t client_index)
{
	Client& client = clients_[client_index];

	const ssize_t received = NDK_recv(client.fd, message_, max_message_size, 0);
	if (received < 0)
	{
		client.clear();
		return;
	}

	if (received == 0 || received >= (ssize_t)max_message_size)
		return;
	message_[received] = 0;

	CanPacket packet;
	bool request = false, result = false;

	if (!parse_packet(message_, packet, request))
	{
		if (client.interactive)
		{
			static const char usage[] = "COMMAND SYNTAX:\r\n\tdev:XX  addr:XXXX  =XXXX...\r\n\tdev:XX  addr:XXXX  ?\r\n> ";
			send_to_client(client_index, usage);
		}
		return;
	}

	if (request)
	{
		result = CanMaster::instance().send_request(packet.id);
		if (result)
		{
			struct ::timeval ts = { 0, 10000 };
			bool got_packet = false;
			for (int i = 0; i < 10; i++)
			{
				fdSelect(0, 0, 0, 0, &ts);
				if (CanMaster::instance().receive_response(packet))
				{
					got_packet = true;
					break;
				}
			}
			if (got_packet)
			{
				handle_can_packet(packet);
				return;
			}
			else
			{
				CanMaster::instance().clear_request();
				static const char timeout[] = "Timeout while waiting reply.\r\n> ";
				send_to_client(client_index, timeout);
			}
		}
	}
	else
		result = CanMaster::instance().send(packet);

	if (client.interactive)
	{
		static const char success[] = "The packet was sent.\r\n> ";
		static const char busy[] = "Busy, try again.\r\n> ";
		const char* reply = result ? success : busy;
		send_to_client(client_index, reply);
	}
}

bool CanServer::parse_packet(const char* message, CanPacket& packet, bool& request)
{
	unsigned int dev_id = 0, address = 0;
	if (::sscanf(message, "dev:%x", &dev_id) != 1 || dev_id > 0xff)
		return false;

	char* ptr = ::strstr(message, "addr:");
	if (ptr == 0 || ::sscanf(ptr, "addr:%x", &address) != 1 || address > 0xffff)
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
	::sprintf(message_, "dev:%.2x  addr:%.4x  =", (unsigned)packet.id.device_id, (unsigned)packet.id.address);
	size_t pos = ::strlen(message_);

	for (size_t i = 0; i < packet.length; ++i)
	{
		if (pos >= max_message_size - 4)
			break;

		::sprintf(&message_[pos], "%.2x", (unsigned)packet.data[i]);
		pos += 2;
	}

	::strcpy(&message_[pos], "\r\n");
	pos += 2;

	for (size_t i = 0; i < MaxClients; ++i)
	{
		if (!clients_[i].valid())
			continue;

		static const char cr[] = "\r";
		static const char prompt[] = "> ";

		if (clients_[i].interactive)
			send_to_client(i, cr);

		ssize_t sent = send_to_client(i, message_, pos); // TODO nonblock?
		if (sent < 0 || (size_t)sent != pos)
			clients_[i].clear();
		else if (clients_[i].interactive)
			send_to_client(i, prompt);
	}
}

void CanServer::greeting(size_t client_index)
{
	Client& client = clients_[client_index];
	if (!client.interactive)
		return;

	// TODO
	static const char message[] = "Welcome to the HA Master Board!\r\nType 'help' for command list.\r\n\r\n> ";
	send_to_client(client_index, message);
}

void CanServer::send_signal()
{
	if (task_handle_)
		fdSelectAbort(task_handle_);
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
	fdOpenSession( Task_self() );
	CanServer::instance().run();
	fdCloseSession( Task_self() );
}
