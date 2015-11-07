#pragma once

#include <ti/drivers/GPIO.h>

#include "utils/NonCopyable.h"
#include "utils/Timer.h"


namespace onewire
{

	class Session : utils::NonCopyable
	{
	public:
		Session(unsigned int gpio_index, int timer_index = utils::Timer::ANY);

	private:
		const unsigned int gpio_index_;
		utils::StaticTimer timer_;
	};

}
