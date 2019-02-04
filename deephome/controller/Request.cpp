#include "Request.h"
#include "HttpServer.h"

#include "util/Log.h"
#include "util/Assert.h"

#include <microhttpd.h>

namespace dh
{
namespace controller
{

#define DH_LOG_CLASS "Request"

	static const char* request_type_to_string(Request::Type type)
	{
		switch (type)
		{
			case Request::Type_Get    : return "GET";
			case Request::Type_Create : return "CREATE";
			case Request::Type_Delete : return "DELETE";
			case Request::Type_Update : return "UPDATE";
			default                   : return "(unknown)";
		}
	}


	Request::Request(HttpServer& owner, struct MHD_Connection* connection, const std::string& url, Type type) :
	    owner_(owner),
	    connection_(connection),
	    url_(url),
	    type_(type)
	{
		DH_LOG(Info) << "created, type: " << request_type_to_string(type_) << ",  url: " << url_;
	}

	Request::~Request()
	{
		if (!completed_ && owner_.is_started())
			complete(Result_InternalServerError);
	}


	Request::Type Request::type() const
	{
		return type_;
	}

	std::string Request::url() const
	{
		return url_;
	}

	std::string Request::argument(const std::string& key) const
	{
		const char* const get_arg = MHD_lookup_connection_value(connection_, MHD_GET_ARGUMENT_KIND, key.c_str());
		if (get_arg)
			return get_arg;

		const char* const post_arg = MHD_lookup_connection_value(connection_, MHD_POST_DATA_KIND, key.c_str());
		if (post_arg)
			return post_arg;

		return std::string();
	}

	void Request::add_reply_string(const std::string& key, const std::string& value)
	{
		reply_strings_.push_back( ReplyString(key, value) );
	}

	void Request::redirect(const std::string& uri)
	{
		DH_LOG(Info) << "redirect to: " << uri;

		static char* const empty_body = "";
		struct MHD_Response* response = MHD_create_response_from_buffer(0, empty_body, MHD_RESPMEM_PERSISTENT);
		DH_ASSERT(response);

		init_response(response);
		MHD_add_response_header(response, "Location", uri.c_str());
		send_response(response, MHD_HTTP_MOVED_PERMANENTLY);
	}

	void Request::complete(Result result)
	{
		DH_LOG(Info) << "complete, result: " << static_cast<int>(result);

		// TODO check Accept header and format the response according to client's desired format.

		std::string response_body;
		if (result == Result_OK)
		{
			for (const ReplyString& reply_string : reply_strings_)
			{
				response_body += reply_string.key + ": " + reply_string.value + "\n";
			}
		}

		struct MHD_Response* response = MHD_create_response_from_buffer(response_body.length(), const_cast<char*>(response_body.c_str()), MHD_RESPMEM_MUST_COPY);
		DH_ASSERT(response);

		init_response(response);
		MHD_add_response_header(response, "Content-Type", "text/plain; charset=utf-8");
		send_response(response, static_cast<unsigned int>(result));
	}

	void Request::init_response(struct MHD_Response* response)
	{
		// TODO fill response with some common headers etc
		(void)response;
	}

	void Request::send_response(struct MHD_Response* response, unsigned int code)
	{
		const int rc = MHD_queue_response(connection_, code, response);
		if (rc == MHD_NO)
			DH_LOG(Error) << "failed to queue response for connection";
		MHD_destroy_response(response);

		completed_ = true;
	}

}
}
