#pragma once

#include <memory>

namespace dh
{
namespace controller
{

	class HttpServer;
	class Request;

	typedef std::shared_ptr<Request> RequestHolder;

}
}
