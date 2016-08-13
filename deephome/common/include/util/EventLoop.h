#pragma once

#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <list>
#include <cstdint>

#include <ev.h>

#include "NonCopyable.h"


namespace dh
{
namespace util
{

typedef std::function< void() > EventHandler;


class EventLoop : NonCopyable
{
public:
	class Connection;

	EventLoop();
	~EventLoop();

	// To be called outside of a loop
	void start();
	void stop();
	void run();

	// To be called inside or outside of a loop
	void enqueue(const EventHandler& handler) const;

	// To be called inside of a loop
	void unloop() const;

private:
	void main_loop();
	void handle_events();

	static void async_event_handler(struct ev_loop*, struct ev_async* async_event, int);
	void on_async_event();

private:
	typedef std::list<EventHandler> EventQueue;

	std::unique_ptr< std::thread > thread_;
	mutable std::mutex guard_;
	std::condition_variable started_;
	bool is_started_ = false;

	mutable struct ev_loop* loop_;
	mutable struct ev_async async_event_;
	mutable EventQueue event_queue_;
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
	struct ev_loop* const loop_;
	struct ev_signal signal_;
	bool started_ = false;
	const EventHandler callback_;
};

}
}
