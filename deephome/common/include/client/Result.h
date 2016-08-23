#pragma once

#include <mutex>
#include <condition_variable>
#include <limits>

#include "util/NonCopyable.h"
#include "util/EventLoop.h"
#include "util/Assert.h"

#include "client/CommonTypes.h"


namespace dh
{
namespace client
{

template <typename T>
class Result : util::NonCopyable
{
public:
	virtual void set(const T* value) = 0;

protected:
	Result() = default;
	~Result() = default;
};

template <typename T>
class BlockingResult : public Result<T>
{
public:
	BlockingResult(T& result) : result_(result) {}
	~BlockingResult() = default;

	bool wait()
	{
		std::unique_lock<std::mutex> lock(guard_);
		while (!notified_)
			signal_.wait(lock);
		return succeeded_;
	}

private:
	virtual void set(const T* value)
	{
		{
			std::unique_lock<std::mutex> lock(guard_);
			notified_ = true;
			succeeded_ = (value != nullptr);
			if (succeeded_)
				result_ = *value;
		}

		signal_.notify_all();
	}

private:
	T& result_;

	std::mutex guard_;
	std::condition_variable signal_;
	bool notified_ = false;
	bool succeeded_ = false;
};

template <typename T>
class CallbackResult : public Result<T>
{
public:
	typedef std::function< void(const T*) > Handler;

	CallbackResult(const Handler& handler) :
	    handler_(handler)
	{
		DH_VERIFY( handler_ );
	}

	~CallbackResult() = default;

private:
	virtual void set(const T* value)
	{
		handler_(value);
	}

private:
	const Handler handler_;
};

}
}
