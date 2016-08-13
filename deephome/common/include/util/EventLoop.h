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

	void start();
	void stop();

	void enqueue(const EventHandler& handler);

private:
	void main_loop();
	void handle_events();
	void do_stop();

	static void async_event_handler(struct ev_loop*, struct ev_async* async_event, int);
	void on_async_event();

private:
	typedef std::list<EventHandler> EventQueue;

	std::unique_ptr< std::thread > thread_;
	std::mutex guard_;
	std::condition_variable started_;
	bool is_started_ = false;

	struct ev_loop* loop_;
	struct ev_async async_event_;
	EventQueue event_queue_;
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

	IoListener(EventLoop& event_loop, int fd, const HandlerProc& callback, uint32_t events = Event_All);
	~IoListener();

	void start();
	void stop();

private:
	static void io_handler(struct ev_loop*, struct ev_io* io, int revents);
	void on_io(uint32_t revents);

private:
	struct ev_loop* loop_;
	struct ev_io io_;
	bool started_ = false;
	const HandlerProc callback_;
	uint32_t events_;
};

}
}
