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


class EventLoop::Connection : NonCopyable
{
public:
	static struct ev_loop* get_loop(EventLoop& event_loop)
	{
		return event_loop.loop_;
	}

private:
	Connection() = delete;
	~Connection() = delete;
};



IoListener::IoListener(EventLoop& event_loop, int fd, const HandlerProc& callback, uint32_t events) :
    loop_( EventLoop::Connection::get_loop(event_loop) ),
    callback_(callback),
    events_(events)
{
	DH_VERIFY(callback_);

	ev_io_init(&io_, io_handler, fd, (events & Event_Read ? EV_READ : 0) | (events & Event_Write ? EV_WRITE : 0));
	io_.data = this;
}

IoListener::~IoListener()
{
	DH_VERIFY( !started_ );
}

void IoListener::start()
{
	if (started_)
		return;
	ev_io_start(loop_, &io_);
	started_ = true;
}

void IoListener::stop()
{
	if (!started_)
		return;
	ev_io_stop(loop_, &io_);
	started_ = false;
}

void IoListener::io_handler(struct ev_loop*, struct ev_io* io, int revents)
{
	IoListener* const self = static_cast<IoListener*>(io->data);
	DH_VERIFY(self);
	self->on_io( (revents & EV_READ ? Event_Read : 0) | (revents & EV_WRITE ? Event_Write : 0) );
}

void IoListener::on_io(uint32_t revents)
{
	callback_(io_.fd, revents);
}

}
}