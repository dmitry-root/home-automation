#pragma once

#include "util/NonCopyable.h"
#include "util/Timer.h"
#include "util/Pin.h"


namespace onewire
{

	class Session;

	class Master : util::NonCopyable
	{
		friend class Session;
	public:
		Master(const util::Pin& input_pin, const util::Pin& output_pin, int timer_index = util::Timer::ANY);

	private:
		util::Pin input_pin_;
		util::Pin output_pin_;
		util::StaticTimer timer_;
	};

	class UniqueId
	{
	public:
		explicit UniqueId(uint32_t lo = 0, uint32_t hi = 0) : lo_(lo), hi_(hi) {}

		uint8_t crc() const { return static_cast<uint8_t>(lo_ & 0xffu); }
		uint8_t device_id() const { return static_cast<uint8_t>(hi_ >> 24); }

		const uint32_t& lo() const { return lo_; }
		const uint32_t& hi() const { return hi_; }
		uint32_t& lo() { return lo_; }
		uint32_t& hi() { return hi_; }

	private:
		uint32_t lo_;
		uint32_t hi_;
	};

	class SearchContext
	{
		friend class Session;
	public:
		enum Type { Type_Normal, Type_Alarm };

		SearchContext(Type type = Type_Normal) :
			type_(type),
			done_(false),
			last_fork_(0)
		{
		}

		const UniqueId& last_id() const { return last_id_; }
		bool done() const { return done_; }

	private:
		bool get_bit(size_t index) const
		{
			const size_t bit_index = index % 32;
			const uint32_t part = index < 32 ? last_id_.lo() : last_id_.hi();
			return (part & (1 << bit_index)) != 0;
		}

		void set_bit(size_t index, bool value)
		{
			const size_t bit_index = index % 32;
			uint32_t& part = index < 32 ? last_id_.lo() : last_id_.hi();
			if (value)
				part |= 1u << bit_index;
			else
				part &= ~(1u << bit_index);
		}

	private:
		Type type_;
		UniqueId last_id_;
		bool done_;
		size_t last_fork_;
	};

	class Session : util::NonCopyable
	{
	public:
		Session(Master& master);
		~Session();

		bool reset();

		void write(uint32_t value, unsigned int bits = 8);
		uint32_t read(unsigned int bits = 8);

		// The following command issue a reset implicitly.
		bool read_rom(UniqueId& rom);
		bool match_rom(const UniqueId& rom);
		bool skip_rom();
		bool search_rom(SearchContext& context);

	private:
		void start_timing();
		void wait(uint32_t mks);
		void low(uint32_t mks);

	private:
		Master& master_;
		uint32_t time_;
	};

}
