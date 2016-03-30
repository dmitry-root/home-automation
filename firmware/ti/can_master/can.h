#pragma once

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/hal/Hwi.h>

#include "ha/can_proto.h"
#include "util/NonCopyable.h"


class CanMaster : util::NonCopyable
{
public:
	~CanMaster();

	static CanMaster& instance();

private:
	CanMaster();

	static void interrupt_handler(UArg arg);

private:
	ti_sysbios_hal_Hwi_Handle hwi_;
};
