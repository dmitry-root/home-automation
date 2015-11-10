#include "onewire/onewire.h"

namespace onewire
{

	enum ThermoCommands
	{
		CONVERT_T = 0x44,
		WRITE_SCRATCHPAD = 0x4E,
		READ_SCRATCHPAD = 0xBE
	};

	ThermometerSession::ThermometerSession(Master& master) :
		Session(master)
	{
	}

	void ThermometerSession::convert_t(WaitMode wait_mode)
	{
		write(CONVERT_T);

		if (wait_mode == NoWait)
			return;

		while (read(1) == 0)
			;
	}

	void ThermometerSession::write_scratchpad(Temperature th, Temperature tl, uint8_t config)
	{
		write(WRITE_SCRATCHPAD);
		write(th.raw8());
		write(tl.raw8());
		write(config);
	}

	void ThermometerSession::read_scratchpad(Scratchpad& scratchpad)
	{
		write(READ_SCRATCHPAD);

		uint8_t raw_data[8];
		for (size_t i = 0; i < sizeof(raw_data); ++i)
			raw_data[i] = read();

		scratchpad.crc_match = crc(raw_data, sizeof(raw_data)) == 0;
		scratchpad.temperature.from_raw16(raw_data[0], raw_data[1]);
		scratchpad.th.from_raw8(raw_data[2]);
		scratchpad.tl.from_raw8(raw_data[3]);
		scratchpad.config = raw_data[4];
	}

	Temperature ThermometerSession::read_t()
	{
		write(READ_SCRATCHPAD);

		const uint8_t lo = read();
		const uint8_t hi = read();

		Temperature result;
		result.from_raw16(lo, hi);
		return result;
	}

}
