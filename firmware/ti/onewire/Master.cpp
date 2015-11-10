#include "onewire/onewire.h"


namespace onewire
{

	using util::TimerParams;

	Master::Master(const util::Pin& input_pin, const util::Pin& output_pin, int timer_index) :
		input_pin_(input_pin),
		output_pin_(output_pin),
		timer_(timer_index, 0, TimerParams(TimerParams::RunMode_Continuous, TimerParams::StartMode_User, TimerParams::PeriodType_Microseconds, 1))
	{
		input_pin_.set_config(util::Pin::Input_NoPull);
		output_pin_.set_config(util::Pin::Output_Standard | util::Pin::Output_Low);
	}

}
