#pragma once

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>

#include "Common.h"
#include "NonCopyable.h"


namespace util
{

	class Semaphore : NonCopyable
	{
	public:
		enum SpecialTimeout
		{
			WaitForever = BIOS_WAIT_FOREVER,
			NoWait = BIOS_NO_WAIT
		};

		Semaphore(ti_sysbios_knl_Semaphore_Handle handle) : semaphore_(handle) {}

		int count() const { return ti_sysbios_knl_Semaphore_getCount(semaphore_); }
		bool pend(uint32_t timeout) { return ti_sysbios_knl_Semaphore_pend(semaphore_, timeout); }
		void post() { ti_sysbios_knl_Semaphore_post(semaphore_); }
		void reset(int count) { ti_sysbios_knl_Semaphore_reset(semaphore_, count); }

	protected:
		ti_sysbios_knl_Semaphore_Handle& handle() { return semaphore_; }
		const ti_sysbios_knl_Semaphore_Handle& handle() const { return semaphore_; }

	private:
		ti_sysbios_knl_Semaphore_Handle semaphore_;
	};

	class SemaphoreParams : NonCopyable
	{
	public:
		SemaphoreParams()
		{
			ti_sysbios_knl_Semaphore_Params_init(&params_);
			params_.event = NULL;
			params_.eventId = 0;
			params_.mode = ti_sysbios_knl_Semaphore_Mode_COUNTING;
		}

		ti_sysbios_knl_Semaphore_Params& data() { return params_; }
		const ti_sysbios_knl_Semaphore_Params& data() const { return params_; }

	private:
		ti_sysbios_knl_Semaphore_Params params_;
	};

	class StaticSemaphore : public Semaphore
	{
	public:
		StaticSemaphore(int count = 0, const SemaphoreParams& params = SemaphoreParams(), xdc_runtime_Error_Block* eb = 0) :
			Semaphore(0)
		{
			ti_sysbios_knl_Semaphore_construct(&semaphore_data_, count, &params.data());
			handle() = ti_sysbios_knl_Semaphore_handle(&semaphore_data_);
		}

		~StaticSemaphore()
		{
			ti_sysbios_knl_Semaphore_destruct(&semaphore_data_);
		}

	private:
		ti_sysbios_knl_Semaphore_Struct semaphore_data_;
	};

}
