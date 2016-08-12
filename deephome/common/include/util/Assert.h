#pragma once

#include "Log.h"
#include <cassert>


#define DH_VERIFY(x) \
	do { \
		if ( !(x) ) { \
			DH_GLOG(FatalError) << "Assertion failed: " #x; \
			assert(false); \
		} \
	} while(0)


#ifdef _DEBUG

#define DH_ASSERT(x) DH_VERIFY(x)

#else

#define DH_ASSERT(x)

#endif

