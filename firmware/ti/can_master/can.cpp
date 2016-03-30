#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

#define PART_TM4C1290NCPDT 1
#include <inc/hw_can.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/hw_ints.h>
#include <driverlib/can.h>
#include <driverlib/sysctl.h>
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>

#include "can.h"
#include "can_server.h"


uint32_t get_system_clock_freqency();


CanMaster& CanMaster::instance()
{
	static CanMaster self;
	return self;
}

void CanMaster::interrupt_handler(UArg)
{
	CanServer::instance().send_signal();
}

CanMaster::CanMaster() :
	hwi_(0)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	GPIOPinConfigure(GPIO_PA0_CAN0RX);
	GPIOPinConfigure(GPIO_PA1_CAN0TX);
	GPIOPinTypeCAN(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);
	CANInit(CAN0_BASE);
	CANBitRateSet(CAN0_BASE, get_system_clock_freqency(), 125000);
	CANEnable(CAN0_BASE);

	Error_Block eb;
	Error_init(&eb);
	hwi_ = ti_sysbios_hal_Hwi_create(INT_CAN0, &CanMaster::interrupt_handler, NULL, &eb);

	CANIntEnable(CAN_INT_MASTER);
}

CanMaster::~CanMaster()
{
	CANIntDisable(CAN_INT_MASTER);
	ti_sysbios_hal_Hwi_delete(hwi_);
	CANDisable(CAN0_BASE);
	SysCtlPeripheralDisable(SYSCTL_PERIPH_CAN0);
}
