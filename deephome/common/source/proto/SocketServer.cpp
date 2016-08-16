#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include "proto/SocketServer.h"
#include "proto/SocketConnection.h"
#include "util/Assert.h"


namespace dh
{
namespace proto
{

#define DH_LOG_CLASS "SocketServer"

/*static*/ int SocketServer::listen_tcp(const std::string& address, const std::string& service, int backlog)
{
	const bool passive = address.empty();

	struct ::addrinfo hints = {0};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = passive ? AI_PASSIVE : 0;

	DH_SLOG(Info) << "binding to address: " << address << ", service: " << service;

	struct ::addrinfo* result = 0;

	const int rc = ::getaddrinfo(passive ? NULL : address.c_str(), service.c_str(), &hints, &result);
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

		const int enable = 1;
		if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != -1 &&
		    ::bind(fd, rp->ai_addr, rp->ai_addrlen) != -1)
		{
			break;
		}

		::close(fd);
		fd = -1;
	}

	::freeaddrinfo(result);

	if (fd == -1)
	{
		DH_SLOG(Error) << "bind failed, address: " << address << ", service: " << service;
		throw std::runtime_error( "bind failed, address: " + address + ":" + service );
	}

	if (::listen(fd, backlog) == -1)
	{
		DH_SLOG(Error) << "listen failed: " << ::strerror(errno);
		::close(fd);
		throw std::runtime_error("listen failed");
	}

	return fd;
}

/*static*/ int SocketServer::listen_unix(const std::string& path, int backlog)
{
	struct ::sockaddr_un address = {0};
	address.sun_family = AF_UNIX;
	DH_VERIFY( path.length() < sizeof(address.sun_path) - 1 );

	std::copy(path.begin(), path.end(), address.sun_path);

	::unlink(path.c_str());

	const int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd == -1)
	{
		DH_SLOG(Error) << "socket error: " << ::strerror(errno);
		throw std::runtime_error( std::string("socket() failed for AF_UNIX: ") + ::strerror(errno) );
	}

	if (::bind(fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == -1)
	{
		::close(fd);
		DH_SLOG(Error) << "bind failed, path: " << path;
		throw std::runtime_error( "bind failed, path: " + path );
	}

	if (::listen(fd, backlog) == -1)
	{
		DH_SLOG(Error) << "listen failed: " << ::strerror(errno);
		::close(fd);
		throw std::runtime_error("listen failed");
	}

	return fd;
}


SocketServer::SocketServer(const util::EventLoop& loop, int socket, const AcceptHandler& accept_handler) :
    scoped_loop_(loop),
    socket_(socket),
    socket_listener_(
        loop, socket,
        std::bind(&SocketServer::on_accept_ready, this),
        util::IoListener::Event_Read),
    accept_handler_(accept_handler)
{
}

SocketServer::~SocketServer()
{
	DH_VERIFY( !started_ );
}

void SocketServer::start()
{
	if (started_)
		return;
	socket_listener_.start();
	started_ = true;
}

void SocketServer::stop()
{
	if (!started_)
		return;
	socket_listener_.stop();
	started_ = false;
	scoped_loop_.invalidate();
}

void SocketServer::on_accept_ready()
{
	const int fd = ::accept4(socket_, NULL, NULL, SOCK_NONBLOCK);
	if (fd == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;
		DH_LOG(Error) << "accept failed: " << ::strerror(errno);
		throw std::runtime_error( std::string("accept failed: ") + ::strerror(errno) );
	}

	ConnectionPtr connection = std::make_shared<SocketConnection>(scoped_loop_.event_loop(), fd);
	scoped_loop_.enqueue( std::bind(accept_handler_, connection) );
}

}
}
