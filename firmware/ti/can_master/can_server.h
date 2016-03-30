#pragma once

#include "util/NonCopyable.h"

class CanServer : util::NonCopyable
{
public:
	~CanServer();

	static CanServer& instance();

	void run();

	void send_signal();

private:
	struct Client : util::NonCopyable
	{
		Client();
		~Client();
		bool valid() const { return fd != -1; }
		void clear();

		int fd;
		bool interactive;
	};

private:
	void handle_connect(size_t server_index);
	void handle_signal();
	void handle_client(size_t client_index);

	void greeting(size_t client_index);

private:
	CanServer();

	enum { MaxClients = 4 };

	enum
	{
		Server_Interactive = 0,
		Server_Raw = 1,
		ServerCount
	};

private:
	int server_fd_[ServerCount];
	int signal_send_, signal_recv_;
	Client clients_[MaxClients];
	bool started_;
};
