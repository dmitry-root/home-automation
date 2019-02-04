#include "HttpServer.h"
#include "Request.h"

#include "util/Assert.h"
#include "util/Log.h"

#include <microhttpd.h>


namespace dh
{
namespace controller
{

#define DH_LOG_CLASS "HttpServer"

	HttpServer::HttpServer(util::EventLoop& event_loop, util::Config& config, const RequestHandler& request_handler) :
	    event_loop_(event_loop),
	    request_handler_(request_handler)
	{
		port_ = config.get<uint16_t>("http.port", 8080);
	}

	HttpServer::~HttpServer()
	{
		DH_ASSERT(!is_started());
	}

	void HttpServer::start()
	{
		if (is_started())
			return;

		DH_LOG(Info) << "starting http daemon on port: " << port_;

		mhd_daemon_ = MHD_start_daemon(
		            MHD_USE_POLL_INTERNALLY | MHD_USE_PIPE_FOR_SHUTDOWN,
		            port_,
		            NULL, NULL,
		            access_handler_cb, this,
		            MHD_OPTION_END);
		DH_ASSERT(mhd_daemon_);
	}

	void HttpServer::stop()
	{
		if (!is_started())
			return;

		DH_LOG(Debug) << "called";
		MHD_stop_daemon(mhd_daemon_);
		mhd_daemon_ = 0;
		event_loop_.invalidate();
	}

	bool HttpServer::is_started() const
	{
		return mhd_daemon_ != 0;
	}


	int HttpServer::access_handler_cb(void *cls,
	                                  struct MHD_Connection *connection,
	                                  const char *url,
	                                  const char *method,
	                                  const char *version,
	                                  const char *upload_data,
	                                  size_t *upload_data_size,
	                                  void **con_cls)
	{
		DH_SLOG(Debug) << "access handler, url: " << url << ", method: " << method << ", version: " << version;
		(void)upload_data;
		(void)upload_data_size;
		(void)con_cls;

		HttpServer* const http_server = reinterpret_cast<HttpServer*>(cls);
		DH_ASSERT(http_server);

		const std::string type = method;
		int request_type = 0;
		if (type == MHD_HTTP_METHOD_GET)
			request_type = Request::Type_Get;
		else if (type == MHD_HTTP_METHOD_POST)
			request_type = Request::Type_Create;
		else if (type == MHD_HTTP_METHOD_PUT)
			request_type = Request::Type_Update;
		else if (type == MHD_HTTP_METHOD_DELETE)
			request_type = Request::Type_Delete;
		else
		{
			DH_LOG(Error) << "unknown request method: " << method;
			return MHD_NO;
		}

		http_server->event_loop_.enqueue( std::bind(&HttpServer::access_handler, http_server, connection, std::string(url), request_type) );
		return MHD_YES;
	}

	void HttpServer::access_handler(struct MHD_Connection* connection, const std::string& url, int request_type)
	{
		DH_LOG(Debug) << "called, url: " << url.c_str();
		if (!is_started())
		{
			DH_LOG(Warning) << "server is not started, ignore";
			return;
		}

		RequestHolder request = std::make_shared<Request>(*this, connection, url, static_cast<Request::Type>(request_type));
		event_loop_.enqueue( std::bind(request_handler_, request) );
	}

}
}
