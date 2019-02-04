#pragma once

#include "util/NonCopyable.h"
#include "util/Config.h"
#include "util/EventLoop.h"

#include <functional>

extern "C"
{
	struct MHD_Daemon;
	struct MHD_Connection;
}

namespace dh
{
namespace controller
{

	class HttpServer : util::NonCopyable
	{
	public:
		typedef std::function< void(RequestHolder) > RequestHandler;

		HttpServer(util::EventLoop& event_loop, util::Config& config, const RequestHandler& request_handler);
		~HttpServer();

		void start();
		void stop();
		bool is_started() const;

	private:
		static int access_handler_cb(void *cls,
		                             struct MHD_Connection *connection,
		                             const char *url,
		                             const char *method,
		                             const char *version,
		                             const char *upload_data,
		                             size_t *upload_data_size,
		                             void **con_cls);

		void access_handler(struct MHD_Connection* connection, const std::string& url, int request_type);

	private:
		util::ScopedEventLoop event_loop_;
		RequestHandler request_handler_;
		uint16_t port_ = 0;
		struct MHD_Daemon* mhd_daemon_ = 0;
	};

}
}

