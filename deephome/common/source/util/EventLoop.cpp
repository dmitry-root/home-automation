#include "util/Assert.h"
#include "util/EventLoop.h"


namespace dh
{
namespace util
{

EventLoop::EventLoop() :
    loop_( ev_loop_new() )
{
	ev_async_init(&async_event_, async_event_handler);
	async_event_.data = this;
}

EventLoop::~EventLoop()
{
	DH_VERIFY( thread_.get() == 0 );
	ev_loop_destroy(loop_);
}

void EventLoop::start()
{
	DH_VERIFY( thread_.get() == 0 );

	is_started_ = false;
	thread_.reset( new std::thread(std::bind(&EventLoop::main_loop, this)) );

	std::unique_lock<std::mutex> lock(guard_);
	while (!is_started_)
		started_.wait(lock);
}

void EventLoop::stop()
{
	DH_VERIFY( thread_.get() != 0 );

	enqueue( std::bind(&EventLoop::do_stop, this) );

	thread_->join();
	thread_.reset();
}

void EventLoop::enqueue(const EventHandler& handler)
{
	DH_VERIFY( thread_.get() != 0 );

	{
		std::unique_lock<std::mutex> lock(guard_);
		event_queue_.push_back( handler );
	}

	ev_async_send(loop_, &async_event_);
}

void EventLoop::main_loop()
{
	ev_async_start(loop_, &async_event_);

	{
		std::unique_lock<std::mutex> lock(guard_);
		is_started_ = true;
	}
	started_.notify_all();

	ev_loop(loop_, 0);

	handle_events();

	ev_async_stop(loop_, &async_event_);
}

void EventLoop::handle_events()
{
	std::unique_lock<std::mutex> lock(guard_);

	while (!event_queue_.empty())
	{
		const EventHandler handler = event_queue_.front();
		event_queue_.pop_front();

		lock.unlock();

		handler();

		lock.lock();
	}
}

void EventLoop::do_stop()
{
	ev_unloop(loop_, EVUNLOOP_ONE);
}

void EventLoop::async_event_handler(struct ev_loop*, struct ev_async* async_event, int)
{
	EventLoop* const self = static_cast<EventLoop*>(async_event->data);
	DH_VERIFY( self != 0 );
	self->on_async_event();
}

void EventLoop::on_async_event()
{
	handle_events();
}

}
}
