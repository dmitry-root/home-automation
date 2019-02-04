#pragma once

#include "CommonTypes.h"

#include "util/NonCopyable.h"
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>

extern "C"
{
	struct MHD_Connection;
	struct MHD_Response;
}

namespace dh
{
namespace controller
{

	class Request : util::NonCopyable
	{
	public:
		enum Type
		{
			Type_Get,
			Type_Update,
			Type_Create,
			Type_Delete
		};

		enum Result
		{
			Result_OK = 200,

			Result_NotFound = 404,

			Result_InternalServerError = 500
		};

		Type type() const;
		std::string url() const;
		std::string argument(const std::string& key) const;

		void add_reply_string(const std::string& key, const std::string& value);
		void redirect(const std::string& uri);
		void complete(Result result);

		~Request();

		template <typename T>
		T get_argument(const std::string& key) const
		{
			T result = T();
			std::istringstream ss( argument(key) );
			ss >> result;
			if (ss.bad())
				throw std::runtime_error( "could not parse argument " + key );
			return result;
		}

		template <typename T>
		void add_reply(const std::string& key, const T& value)
		{
			std::ostringstream ss;
			ss << value;
			add_reply_string( key, ss.str() );
		}

		template <>
		void add_reply<const char*>(const std::string& key, const char* value)
		{
			add_reply_string(key, value);
		}

		template <>
		void add_reply<std::string>(const std::string& key, const std::string& value)
		{
			add_reply_string(key, value);
		}

	private:
		friend class HttpServer;
		Request(HttpServer& owner, struct MHD_Connection* connection, const std::string& url, Type type);

		void init_response(struct MHD_Response* response);
		void send_response(struct MHD_Response* response, unsigned int code);

	private:
		struct ReplyString
		{
			explicit ReplyString(const std::string& key = std::string(), const std::string& value = std::string()) :
			    key(key), value(value) {}
			std::string key;
			std::string value;
		};

	private:
		HttpServer& owner_;
		struct MHD_Connection* connection_;
		std::string url_;
		Type type_;
		bool completed_ = false;
		std::vector<ReplyString> reply_strings_;
	};

}
}
