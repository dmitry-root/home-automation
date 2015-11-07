#pragma once

#include <ti/sysbios/hal/Timer.h>

#include "Common.h"
#include "NonCopyable.h"


namespace utils
{

	class Timer : NonCopyable
	{
	public:
		static unsigned int num_timers()
		{
			return ti_sysbios_hal_Timer_getNumTimers();
		}

		static bool is_used(unsigned int index)
		{
			return ti_sysbios_hal_Timer_getStatus() == ti_sysbios_interfaces_ITimer_Status_INUSE;
		}

		static void startup()
		{
			ti_sysbios_hal_Timer_startup();
		}

		static const int ANY = ti_sysbios_hal_Timer_ANY;

		typedef void (*Callback)(unsigned int);

		Timer(ti_sysbios_hal_Timer_Handle timer_handle) :
			timer_handle_(timer_handle) {}

		uint32_t max_ticks() const { return ti_sysbios_hal_Timer_getMaxTicks(timer_handle_); }
		void set_next_tick(uint32_t ticks) { ti_sysbios_hal_Timer_setNextTick(timer_handle_, ticks); }
		void start() { ti_sysbios_hal_Timer_start(timer_handle_); }
		void stop() { ti_sysbios_hal_Timer_stop(timer_handle_); }
		void set_period(uint32_t period) { ti_sysbios_hal_Timer_setPeriod(timer_handle_, period); }
		void set_period_mks(uint32_t period_mks) { ti_sysbios_hal_Timer_setPeriodMicroSecs(timer_handle_, period_mks); }
		uint32_t period() const { return ti_sysbios_hal_Timer_getPeriod(timer_handle_); }
		uint32_t counter() const { return ti_sysbios_hal_Timer_getCount(timer_handle_); }
		void set_callback(Callback cb, unsigned int arg) { ti_sysbios_hal_Timer_setFunc(timer_handle_, cb, arg); }
		void trigger(uint32_t cycles) { ti_sysbios_hal_Timer_trigger(timer_handle_, cycles); }
		uint32_t expired_counts() const { return ti_sysbios_hal_Timer_getExpiredCounts(timer_handle_); }
		uint32_t expired_ticks(uint32_t period) const { return ti_sysbios_hal_Timer_getExpiredTicks(timer_handle_, period); }
		uint32_t current_tick(bool save = false) const { return ti_sysbios_hal_Timer_getCurrentTick(timer_handle_, save); }

	protected:
		ti_sysbios_hal_Timer_Handle& handle() { return timer_handle_; }
		const ti_sysbios_hal_Timer_Handle& handle() const { return timer_handle_; }

	private:
		ti_sysbios_hal_Timer_Handle timer_handle_;
	};


	class TimerParams : NonCopyable
	{
	public:
		TimerParams()
		{
			ti_sysbios_hal_Timer_Params_init(&timer_params_);
		}

		ti_sysbios_hal_Timer_Params& data() { return timer_params_; }
		const ti_sysbios_hal_Timer_Params& data() const { return timer_params_; }

		void set_run_continuous() { timer_params_.runMode = ti_sysbios_interfaces_ITimer_RunMode_CONTINUOUS; }
		void set_run_oneshot() { timer_params_.runMode = ti_sysbios_interfaces_ITimer_RunMode_ONESHOT; }
		void set_run_dynamic() { timer_params_.runMode = ti_sysbios_interfaces_ITimer_RunMode_DYNAMIC; }

		void set_start_auto() { timer_params_.startMode = ti_sysbios_interfaces_ITimer_StartMode_AUTO; }
		void set_start_user() { timer_params_.startMode = ti_sysbios_interfaces_ITimer_StartMode_USER; }

		void set_cb_arg(unsigned int arg) { timer_params_.arg = arg; }

		void set_period_count(uint32_t period)
		{
			timer_params_.period = period;
			timer_params_.periodType = ti_sysbios_interfaces_ITimer_PeriodType_COUNTS;
		}

		void set_period_mks(uint32_t period_mks)
		{
			timer_params_.period = period_mks;
			timer_params_.periodType = ti_sysbios_interfaces_ITimer_PeriodType_MICROSECS;
		}

	private:
		ti_sysbios_hal_Timer_Params timer_params_;
	};

	class StaticTimer : public Timer
	{
	public:
		using Timer::Callback;

		StaticTimer(int id, Callback cb, const TimerParams& params, xdc_runtime_Error_Block& eb) :
			Timer(0)
		{
			ti_sysbios_hal_Timer_construct(&timer_data_, id, cb, &params.data(), &eb);
			handle() = ti_sysbios_hal_Timer_handle(&timer_data_);
		}

		~StaticTimer()
		{
			ti_sysbios_hal_Timer_destruct(&timer_data_);
		}

	private:
		ti_sysbios_hal_Timer_Struct timer_data_;
	};


	class DynamicTimer : public Timer
	{
	public:
		using Timer::Callback;

		DynamicTimer(int id, Callback cb, const TimerParams& params, xdc_runtime_Error_Block& eb) :
			Timer( ti_sysbios_hal_Timer_create(id, cb, &params.data(), &eb) )
		{
		}

		~DynamicTimer()
		{
			ti_sysbios_hal_Timer_delete( handle() );
		}
	};

}
