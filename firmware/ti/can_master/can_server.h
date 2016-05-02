#pragma once

#include "util/NonCopyable.h"

extern "C" {
#include <socketndk.h>
}


struct CanPacket;

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
		bool valid() const { return fd != INVALID_SOCKET; }
		void clear();

		SOCKET fd;
		bool interactive;
	};

private:
	void handle_connect(size_t server_index);
	void handle_signal();
	void handle_client(size_t client_index);

	void handle_can_packet(const CanPacket& packet);

	void greeting(size_t client_index);
	bool parse_packet(const char* message, CanPacket& packet, bool& request);
	bool parse_dev_id_packet(const char* message, CanPacket& packet, bool& request);

	int send_to_client(size_t client_index, const char* message, size_t length = (size_t)-1);

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
	static const size_t max_message_size = 128;

	SOCKET server_fd_[ServerCount] = { INVALID_SOCKET };
	Client clients_[MaxClients];
	char message_[max_message_size];
	bool started_ = false;
	void* volatile task_handle_ = 0;
};
