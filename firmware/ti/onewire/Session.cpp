#include "onewire/onewire.h"
#include "util/HwiLock.h"


namespace onewire
{

	enum RomCommands
	{
		READ_ROM = 0x33,
		MATCH_ROM = 0x55,
		SKIP_ROM = 0xCC,
		SEARCH_ROM = 0xF0,
		ALARM_SEARCH = 0xEC
	};


	Session::Session(Master& master) :
		master_(master)
	{
	}

	Session::~Session()
	{
	}

	bool Session::reset()
	{
		util::HwiLock hwi_lock;

		low(500);
		wait(70);

		const bool result = !master_.input_pin_;

		wait(500);
		return result;
	}

	uint32_t Session::read(unsigned int bits)
	{
		util::HwiLock hwi_lock;

		uint32_t result = 0;

		for (unsigned int bit = 0; bit < bits; bit++)
		{
			low(5);
			wait(10);
			if (master_.input_pin_)
				result |= 1 << bit;
			wait(45 + 2); // end of time slot + recovery time
		}

		return result;
	}

	void Session::write(uint32_t value, unsigned int bits)
	{
		util::HwiLock hwi_lock;

		for (unsigned int bit = 0; bit < bits; bit++)
		{
			if (value & (1 << bit))
			{
				// write 1
				low(5);
				wait(55);
			}
			else
			{
				// write 0
				low(60);
			}
			wait(2); // recovery time
		}
	}

	void Session::wait(uint32_t mks)
	{
		master_.sleep(mks);
	}

	void Session::low(uint32_t mks)
	{
		master_.output_pin_.write(1);
		wait(mks);
		master_.output_pin_.write(0);
	}


	void Session::read_rom(UniqueId& rom)
	{
		write(READ_ROM);
		const uint64_t lo = read(32);
		const uint64_t hi = read(32);
		rom = (hi << 32) | lo;
	}

	void Session::match_rom(const UniqueId& rom)
	{
		write(MATCH_ROM);
		write(rom & ~0u, 32);
		write(rom >> 32, 32);
	}

	void Session::skip_rom()
	{
		write(SKIP_ROM);
	}

	bool Session::search_rom(SearchContext& context)
	{
		if (context.done())
			return false;

		write(context.type_ == SearchContext::Type_Alarm ? ALARM_SEARCH : SEARCH_ROM);

		uint32_t next_fork = 0;

		for (size_t index = 0; index < 64; ++index)
		{
			// Read 2 bits from 1-wire bus. The first bit is the current ROM bit, the second bit is
			// it's complementary. If both bits are 0, it's a collision.
			const uint32_t answer = read(2);
			if (answer == 3) // must not happen
				return false;

			const bool value = (answer & 1) != 0;
			const bool complement = (answer & 2) != 0;

			if (value || complement)
			{
				// no collision
				context.set_bit(index, value);
			}
			else if (index == context.last_fork_)
			{
				context.set_bit(index, 1);
			}
			else if (index > context.last_fork_)
			{
				context.set_bit(index, 0);
				next_fork = index;
			}
			else if (!context.get_bit(index))
			{
				next_fork = index;
			}

			write(context.get_bit(index) ? 1 : 0, 1);
		}

		context.last_fork_ = next_fork;
		context.done_ = next_fork == 0;
		return true;
	}

}
