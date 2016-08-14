﻿#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include <vector>
#include <algorithm>

#include "proto/SocketConnection.h"
#include "proto/Message.h"
#include "util/Assert.h"


namespace dh
{
namespace proto
{

#define DH_LOG_CLASS "SocketConnection"

/*static*/ int SocketConnection::connect_tcp(const std::string& address, const std::string& service)
{
	struct ::addrinfo hints = {0};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	DH_SLOG(Info) << "connecting to address: " << address << ", service: " << service;

	struct ::addrinfo* result = 0;

	const int rc = ::getaddrinfo(address.c_str(), service.c_str(), &hints, &result);
	if (rc != 0)
	{
		DH_SLOG(Error) << "getaddrinfo failed, address: " << address << ", service: " << service << ", error: " << ::gai_strerror(rc);
		throw std::runtime_error( "getaddrinfo failed for address: " + address + ":" + service + ", error: " + ::gai_strerror(rc) );
	}

	int fd = -1;
	for (struct ::addrinfo* rp = result; rp; rp = rp->ai_next)
	{
		fd = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (fd == -1)
			continue;
		if (::connect(fd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;
		::close(fd);
		fd = -1;
	}

	::freeaddrinfo(result);

	if (fd == -1)
	{
		DH_SLOG(Error) << "connect failed, address: " << address << ", service: " << service;
		throw std::runtime_error( "connect failed, address: " + address + ":" + service );
	}

	return fd;
}

/*static*/ int SocketConnection::connect_unix(const std::string& path)
{
	struct ::sockaddr_un address = {0};
	address.sun_family = AF_UNIX;
	DH_VERIFY( path.length() < sizeof(address.sun_path) - 1 );

	std::copy(path.begin(), path.end(), address.sun_path);

	const int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd == -1)
	{
		DH_SLOG(Error) << "socket error: " << ::strerror(errno);
		throw std::runtime_error( std::string("socket() failed for AF_UNIX: ") + ::strerror(errno) );
	}

	if (::connect(fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == -1)
	{
		::close(fd);
		DH_SLOG(Error) << "connect failed, path: " << path;
		throw std::runtime_error( "connect failed, path: " + path );
	}

	return fd;
}


SocketConnection::SocketConnection(const util::EventLoop& loop, int socket) :
    Connection(loop),
    socket_(socket),
    socket_listener_(
        loop, socket_,
        std::bind(&SocketConnection::on_read_ready, this),
        util::IoListener::Event_Read)
{
}

SocketConnection::~SocketConnection()
{
	::close(socket_);
}

void SocketConnection::impl_subscribe()
{
	socket_listener_.start();
}

void SocketConnection::impl_unsubscribe()
{
	socket_listener_.stop();
}

void SocketConnection::impl_send(const Packet& packet)
{
	const std::string& packet_string = packet.convert_to_string();
	DH_LOG(Info) << "send: " << packet_string;

	const std::string line = packet_string + "\r\n";
	size_t pos = 0, len = line.length();
	while (pos < len)
	{
		// NOTE use blocking write here, as we expect there wouldn't be too huge amount of data.
		const ssize_t sent = ::send(socket_, line.data() + pos, len - pos, 0);
		if (sent < 0)
		{
			DH_LOG(Error) << "send error: " << ::strerror(errno);
			throw std::runtime_error( std::string("send error: ") + ::strerror(errno) );
		}

		pos += static_cast<size_t>(sent);
	}
}

void SocketConnection::on_read_ready()
{
	std::vector<char> buffer(4096);
	const ssize_t received = ::recv(socket_, buffer.data(), buffer.size(), MSG_DONTWAIT);

	if (received < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;

		if (errno == EPIPE || errno == ETIMEDOUT)
		{
			DH_LOG(Warning) << "recv error, connection closed: " << ::strerror(errno);
			packet_received( PacketPtr() );
			return;
		}

		DH_LOG(Error) << "recv error: " << ::strerror(errno);
		throw std::runtime_error( std::string("recv error: ") + ::strerror(errno) );
	}
	else if (received == 0)
		return;

	buffer_.append(buffer.data(), received);
	std::vector<char>().swap(buffer);

	size_t start = 0, end = 0;
	while (true)
	{
		end = buffer_.find_first_of("\r\n", start);
		if (end == std::string::npos)
		{
			buffer_.replace(0, start, std::string());
			break;
		}

		const std::string line = buffer_.substr(start, end - start);
		if (line.find_first_not_of(" \t") != std::string::npos)
			handle_line( line );

		start = buffer_.find_first_not_of("\r\n", end);
		if (start == std::string::npos)
		{
			buffer_.clear();
			break;
		}
	}
}

void SocketConnection::handle_line(const std::string& line)
{
	DH_LOG(Info) << "received packet: \"" << line << "\"";

	PacketPtr packet;

	try
	{
		packet = Packet::create(line);
	}
	catch (const std::exception& e)
	{
		DH_LOG(Error) << "packet parse error: " << e.what();
		return;
	}

	packet_received(packet);
}

}
}