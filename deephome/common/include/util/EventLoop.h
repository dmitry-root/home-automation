#pragma once

#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <list>
#include <exception>
#include <cstdint>

#include <ev.h>

#include "NonCopyable.h"


namespace dh
{
namespace util
{

class EventScope
{
public:
	EventScope() {}

	EventScope(std::shared_ptr<int> value) :
	    value_(value),
	    valid_value_( *value_ )
	{
	}

	bool valid() const { return !value_ || *value_ == valid_value_; }

private:
	std::shared_ptr<int> value_;
	int valid_value_ = 0;
};


typedef std::function< void() > EventHandler;
typedef std::function< void(const std::exception&) > ExceptionHandler;


class EventLoop : NonCopyable
{
public:
	class Connection;

	explicit EventLoop(const ExceptionHandler& exception_handler = ExceptionHandler());
	~EventLoop();

	// To be called outside of a loop
	void start();
	void stop();
	void run();

	// To be called inside or outside of a loop
	void enqueue(const EventHandler& handler, const EventScope& scope = EventScope()) const;

	// To be called inside of a loop
	void unloop() const;

	void handle_exception(const std::exception& e) const;

private:
	void main_loop();
	void handle_events();

	static void async_event_handler(struct ev_loop*, struct ev_async* async_event, int);
	void on_async_event();

private:
	typedef std::list< std::pair<EventHandler, EventScope> > EventQueue;

	const ExceptionHandler exception_handler_;
	std::unique_ptr< std::thread > thread_;
	mutable std::mutex guard_;
	std::condition_variable started_;
	bool is_started_ = false;

	mutable struct ev_loop* loop_;
	mutable struct ev_async async_event_;
	mutable EventQueue event_queue_;
};


class ScopedEventLoop : NonCopyable
{
public:
	ScopedEventLoop(const EventLoop& event_loop) :
	    event_loop_(event_loop),
	    scope_( std::make_shared<int>(0) )
	{
	}

	~ScopedEventLoop()
	{
		invalidate();
	}

	void enqueue(const EventHandler& handler)
	{
		event_loop_.enqueue(handler, scope_);
	}

	void invalidate()
	{
		++(*scope_);
	}

	const EventLoop& event_loop() const { return event_loop_; }

private:
	ScopedEventLoop() = delete;

private:
	const EventLoop& event_loop_;
	std::shared_ptr<int> scope_;
};


class IoListener : NonCopyable
{
public:
	enum Event
	{
		Event_Read = 1,
		Event_Write = 2,
		Event_All = 3
	};

	typedef std::function< void(int, uint32_t) > HandlerProc;

	IoListener(const EventLoop& event_loop, int fd, const HandlerProc& callback, uint32_t events = Event_All);
	~IoListener();

	void start();
	void stop();

private:
	static void io_handler(struct ev_loop*, struct ev_io* io, int revents);
	void on_io(uint32_t revents);

private:
	const EventLoop& event_loop_;
	struct ev_loop* const loop_;
	struct ev_io io_;
	bool started_ = false;
	const HandlerProc callback_;
	uint32_t events_;
};


class SignalListener : NonCopyable
{
public:
	SignalListener(const EventLoop& event_loop, int signal, const EventHandler& callback);
	~SignalListener();

	void start();
	void stop();

private:
	static void signal_handler(struct ev_loop*, struct ev_signal* signal, int);
	void on_signal();

private:
	const EventLoop& event_loop_;
	struct ev_loop* const loop_;
	struct ev_signal signal_;
	bool started_ = false;
	const EventHandler callback_;
};

}
}
