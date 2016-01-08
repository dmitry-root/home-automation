#include "onewire/onewire.h"
#include "util/HwiLock.h"


namespace onewire
{

	using util::TimerParams;

	static
	void post_semaphore(unsigned int semaphore)
	{
		reinterpret_cast<util::Semaphore*>(semaphore)->post();
	}

	Master::Master(const util::Pin& input_pin, const util::Pin& output_pin, int timer_index) :
		input_pin_(input_pin),
		output_pin_(output_pin),
		timer_(
			timer_index, post_semaphore,
			TimerParams(
				TimerParams::RunMode_OneShot,
				TimerParams::StartMode_User,
				TimerParams::PeriodType_Microseconds,
				1).set_cb_arg(&semaphore_)
		)
	{
		input_pin_.set_config(util::Pin::Input_NoPull);
		output_pin_.set_config(util::Pin::Output_Standard | util::Pin::Output_Low | util::Pin::Output_HighStrength);
	}

	void Master::sleep(unsigned int mks)
	{
		semaphore_.reset(0);
		{
			util::HwiLock lock;
			timer_.set_period_mks(mks);
			timer_.start();
		}
		semaphore_.pend(util::Semaphore::WaitForever);
		timer_.stop();
	}

}
