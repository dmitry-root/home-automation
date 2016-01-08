#pragma once

#include "util/NonCopyable.h"
#include "util/Timer.h"
#include "util/Semaphore.h"
#include "util/Pin.h"


namespace onewire
{

	uint8_t crc(const uint8_t* data, size_t count);

	class Session;

	class Master : util::NonCopyable
	{
		friend class Session;
	public:
		Master(const util::Pin& input_pin, const util::Pin& output_pin, int timer_index = util::Timer::ANY);

		void sleep(unsigned int mks);

	private:
		util::Pin input_pin_;
		util::Pin output_pin_;
		util::StaticSemaphore semaphore_;
		util::StaticTimer timer_;
	};

	typedef uint64_t UniqueId;

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
			return (last_id_ & (1u << index)) != 0;
		}

		void set_bit(size_t index, bool value)
		{
			if (value)
				last_id_ |= 1u << index;
			else
				last_id_ &= ~(1u << index);
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

		void read_rom(UniqueId& rom);
		void match_rom(const UniqueId& rom);
		void skip_rom();
		bool search_rom(SearchContext& context);

	private:
		void wait(uint32_t mks);
		void low(uint32_t mks);

	private:
		Master& master_;
	};

	class Temperature
	{
		friend class ThermometerSession;
	public:
		static Temperature from_fp(float fp_value) { return Temperature( static_cast<int16_t>(fp_value * 16.f) ); }
		static Temperature from_mkgrad(int mkgrad_value) { return Temperature( static_cast<int16_t>(mkgrad_value * 16 / 1000) ); }

		Temperature() : value_(0) {}
		int16_t raw_value() const { return value_; }
		int mkgrad_value() const { return static_cast<int>(value_) * 1000 / 16; }
		float fp_value() const { return static_cast<float>(value_) / 16; }

	private:
		explicit Temperature(int16_t value) : value_(value) {}
		void from_raw16(uint8_t lo, uint8_t hi) { value_ = static_cast<int16_t>( (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo) ); }
		void from_raw8(uint8_t value) { value_ = static_cast<int16_t>(static_cast<int8_t>(value)) << 4; }
		uint16_t raw16() const { return static_cast<uint16_t>(value_); }
		uint8_t raw8() const { return static_cast<uint8_t>( static_cast<int8_t>( value_ >> 4 ) ); }

		int16_t value_;
	};

	class ThermometerSession : public Session
	{
	public:
		enum WaitMode { Wait, NoWait };
		enum Configuration
		{
			Resolution_9bit = 0,
			Resolution_10bit = 0x20,
			Resolution_11bit = 0x40,
			Resolution_12bit = 0x60
		};

		struct Scratchpad
		{
			Scratchpad() : config(0), crc_match(false) {}
			Temperature temperature;
			Temperature th, tl;
			uint8_t config;
			bool crc_match;
		};

		ThermometerSession(Master& master);
		void convert_t(WaitMode wait_mode);
		void write_scratchpad(Temperature th, Temperature tl, uint8_t config);
		void read_scratchpad(Scratchpad& scratchpad);
		Temperature read_t(); // convenience function, does not check crc. use read_scratchpad() instead.
	};

}
