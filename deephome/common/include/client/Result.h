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
	typedef std::experimental::optional<T> ValueType;

	virtual void set(ValueType value) = 0;

protected:
	Result() = default;
	~Result() = default;
};

template <typename T>
class BlockingResult : public Result<T>
{
public:
	using typename Result<T>::ValueType;

	BlockingResult() = default;
	~BlockingResult() = default;

	ValueType wait()
	{
		std::unique_lock<std::mutex> lock(guard_);
		while (!notified_)
			signal_.wait(lock);
		return result_;
	}

private:
	virtual void set(ValueType value)
	{
		{
			std::unique_lock<std::mutex> lock(guard_);
			notified_ = true;
			result_ = value;
		}

		signal_.notify_all();
	}

private:
	ValueType result_;

	std::mutex guard_;
	std::condition_variable signal_;
	bool notified_ = false;
};

template <typename T>
class CallbackResult : public Result<T>
{
public:
	using typename Result<T>::ValueType;
	typedef std::function< void(ValueType) > Handler;

	CallbackResult(const Handler& handler) :
	    handler_(handler)
	{
		DH_VERIFY( handler_ );
	}

	~CallbackResult() = default;

private:
	virtual void set(ValueType value)
	{
		handler_(value);
	}

private:
	const Handler handler_;
};

}
}
