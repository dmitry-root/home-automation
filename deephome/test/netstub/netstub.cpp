#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "util/EventLoop.h"
#include "util/Log.h"
#include "proto/SocketServer.h"

using namespace dh;
using namespace std::placeholders;

#define DH_LOG_CLASS "NetStub"

class NetStub : util::NonCopyable
{
public:
	NetStub(int argc, char** argv);
	int run();

private:
	void on_stdin_ready();
	void on_socket_ready();
	void on_accept(int server_fd);

	void receive_and_send(int recv_fd, int send_fd);

private:
	util::EventLoop event_loop_;
	util::IoListener stdin_listener_;
	util::IoListener accept_listener_;
	int socket_fd_ = -1;
	std::unique_ptr<util::IoListener> socket_listener_;
};


NetStub::NetStub(int argc, char** argv) :
    stdin_listener_(event_loop_, 0, std::bind(&NetStub::on_stdin_ready, this), util::IoListener::Event_Read),
    accept_listener_(event_loop_, proto::SocketServer::listen_tcp("", "2121"), std::bind(&NetStub::on_accept, this, _1), util::IoListener::Event_Read)
{
	(void)argc; (void)argv;
}

int NetStub::run()
{
	stdin_listener_.start();
	accept_listener_.start();

	DH_LOG(Info) << "Waiting connections on port 2121...";
	event_loop_.run();
	DH_LOG(Info) << "Terminated.";

	if (socket_listener_)
		socket_listener_->stop();
	accept_listener_.stop();
	stdin_listener_.stop();

	return 0;
}

void NetStub::on_stdin_ready()
{
	receive_and_send(0, socket_fd_);
}

void NetStub::on_socket_ready()
{
	receive_and_send(socket_fd_, 1);
}

void NetStub::receive_and_send(int recv_fd, int send_fd)
{
	char buffer[1024];
	const ssize_t amount = ::read(recv_fd, buffer, sizeof(buffer));
	if (amount < 0)
	{
		DH_LOG(Warning) << "Read error: " << ::strerror(errno);
		event_loop_.unloop();
		return;
	}

	if (amount == 0)
		return;

	const ssize_t written = ::write(send_fd, buffer, static_cast<size_t>(amount));
	if (written != amount)
	{
		DH_LOG(Warning) << "Write amount: " << ::strerror(errno);
		event_loop_.unloop();
		return;
	}
}

void NetStub::on_accept(int server_fd)
{
	const int fd = ::accept(server_fd, 0, 0);
	if (fd == -1)
	{
		DH_LOG(FatalError) << "accept() failed: " << ::strerror(errno);
		event_loop_.unloop();
		return;
	}

	DH_LOG(Info) << "Connection accepted, fd: " << fd;
	accept_listener_.stop();

	socket_listener_.reset( new util::IoListener(event_loop_, fd, std::bind(&NetStub::on_socket_ready, this), util::IoListener::Event_Read) );
	socket_listener_->start();
	socket_fd_ = fd;

	DH_LOG(Info) << "Ready to send and receive commands. Type commands and receive replies. EOF to stop.";
}


int main(int argc, char** argv)
{
	return NetStub(argc, argv).run();
}
