#pragma once

#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <list>

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

}
}
