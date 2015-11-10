#pragma once

#include <ti/drivers/GPIO.h>

#include "Common.h"
#include "NonCopyable.h"


namespace util
{

	class Pin
	{
	public:
		enum ConfigOutput
		{
			Output_Standard = GPIO_CFG_OUT_STD,
			Output_OpenDrainNoPull = GPIO_CFG_OUT_OD_NOPULL,
			Output_OpenDrainPullUp = GPIO_CFG_OUT_OD_PU,
			Output_OpenDrainPullDown = GPIO_CFG_OUT_OD_PD,

			Output_High = GPIO_CFG_OUT_HIGH,
			Output_Low = GPIO_CFG_OUT_LOW,

			Output_LowStrength = GPIO_CFG_OUT_STR_LOW,
			Output_MediumStrength = GPIO_CFG_OUT_STR_MED,
			Output_HighStrength = GPIO_CFG_OUT_STR_HIGH
		};

		enum ConfigInput
		{
			Input_NoPull = GPIO_CFG_IN_NOPULL,
			Input_PullUp = GPIO_CFG_IN_PU,
			Input_PullDown = GPIO_CFG_IN_PD
		};

		enum ConfigInterrupt
		{
			Interrupt_None = GPIO_CFG_IN_INT_NONE,
			Interrupt_FallingEdge = GPIO_CFG_IN_INT_FALLING,
			Interrupt_RisingEdge = GPIO_CFG_IN_INT_RISING,
			Interrupt_BothEdges = GPIO_CFG_IN_INT_BOTH_EDGES,
			Interrupt_Low = GPIO_CFG_IN_INT_LOW,
			Interrupt_High = GPIO_CFG_IN_INT_HIGH
		};

		explicit Pin(unsigned int index) : index_(index) {}

		void clear_interrupt() { GPIO_clearInt(index_); }
		void disable_interrupt() { GPIO_disableInt(index_); }
		void enable_interrupt() { GPIO_enableInt(index_); }
		unsigned int read() const { return GPIO_read(index_); }
		operator bool() const { return read() != 0; }

		void write(bool value) { GPIO_write(index_, value ? 1 : 0); }
		void toggle() { GPIO_toggle(index_); }

		void set_config(unsigned int config) { GPIO_setConfig(index_, config); }
		void set_interrupt_config(unsigned int config) { GPIO_setConfig(index_, config | GPIO_CFG_IN_INT_ONLY); }

	private:
		unsigned int index_;
	};

}
