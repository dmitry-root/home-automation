#pragma once

#include <ti/sysbios/hal/Hwi.h>

#include "NonCopyable.h"


namespace util
{

	class HwiLock : NonCopyable
	{
	public:
		HwiLock() : key_( Hwi_disable() ) {}
		~HwiLock() { Hwi_restore(key_); }
	private:
		const unsigned int key_;
	};

}
