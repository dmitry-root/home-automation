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

#include <string.h>

#include "can.h"
#include "can_server.h"


uint32_t get_system_clock_freqency();


enum
{
	TxBegin = 0,
	TxEnd = 16,
	RxBegin = TxEnd,
	RxEnd = 32,

	RxMask = ~((1 << RxBegin) - 1),

	MsgLen = 8
};


CanMaster& CanMaster::instance()
{
	static CanMaster self;
	return self;
}

void CanMaster::interrupt_handler(UArg)
{
	// Clear Rx objects interrupts
	for (uint32_t i = RxBegin; i < RxEnd; ++i)
		CANIntClear(CAN0_BASE, i + 1);
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
	CANRetrySet(CAN0_BASE, true);
	CANEnable(CAN0_BASE);

	Error_Block eb;
	Error_init(&eb);
	hwi_ = ti_sysbios_hal_Hwi_create(INT_CAN0, &CanMaster::interrupt_handler, NULL, &eb);

	setup_rx();

	CANIntEnable(CAN0_BASE, CAN_INT_MASTER);
}

CanMaster::~CanMaster()
{
	CANIntDisable(CAN0_BASE, CAN_INT_MASTER);
	ti_sysbios_hal_Hwi_delete(&hwi_);
	CANDisable(CAN0_BASE);
	SysCtlPeripheralDisable(SYSCTL_PERIPH_CAN0);
}


void CanMaster::setup_rx()
{
	tCANMsgObject object = {0};
	object.ui32MsgID = HA_CAN_ReservedBits;
	object.ui32MsgIDMask = HA_CAN_ReservedMask;
	object.ui32Flags = MSG_OBJ_EXTENDED_ID | MSG_OBJ_USE_EXT_FILTER | MSG_OBJ_USE_DIR_FILTER | MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_FIFO;

	for (uint32_t i = RxBegin; i < RxEnd; ++i)
	{
		if (i == RxEnd - 1)
			object.ui32Flags &= ~MSG_OBJ_FIFO;
		CANMessageSet(CAN0_BASE, i + 1, &object, MSG_OBJ_TYPE_RX);
	}
}

bool CanMaster::do_send(uint32_t msg_id, const uint8_t* data, size_t length, bool request)
{
	if (!request && !data)
		return false;

	const uint32_t busy_objects = CANStatusGet(CAN0_BASE, CAN_STS_TXREQUEST);
	uint32_t object_id = 0;

	for (uint32_t i = TxBegin; i < TxEnd; ++i)
	{
		if (busy_objects & (1 << i))
			continue;
		object_id = i+1;
		break;
	}

	if (object_id == 0)
		return false;

	tCANMsgObject object = {0};
	object.ui32MsgID = msg_id;
	object.ui32Flags = MSG_OBJ_EXTENDED_ID;

	if (!request)
	{
		object.pui8MsgData = tx_data_ + (object_id-1)*MsgLen;
		object.ui32MsgLen = length;
		::memcpy(object.pui8MsgData, data, length);
	}

	CANMessageSet(CAN0_BASE, object_id, &object, request ? MSG_OBJ_TYPE_TX : MSG_OBJ_TYPE_TX_REMOTE);
	return true;
}


bool CanMaster::send(const CanPacket& packet)
{
	const uint32_t msg_id = HA_CAN_packet_id_to_number(&packet.id);
	return do_send(msg_id, packet.data, packet.length, false);
}

bool CanMaster::send_request(const HA_CAN_PacketId& packet_id)
{
	const uint32_t msg_id = HA_CAN_packet_id_to_number(&packet_id);
	return do_send(msg_id, 0, 0, true);
}

bool CanMaster::receive(CanPacket& packet)
{
	const uint32_t new_data = CANStatusGet(CAN0_BASE, CAN_STS_NEWDAT);
	if ((new_data & RxMask) == 0)
		return false;

	uint32_t object_id = 0;
	for (uint32_t i = RxBegin; i < RxEnd; ++i)
	{
		if (new_data & (1 << i))
		{
			object_id = i + 1;
			break;
		}
	}

	if (!object_id)
		return false;

	tCANMsgObject object = {0};
	object.pui8MsgData = packet.data;
	object.ui32MsgLen = MsgLen;

	CANMessageGet(CAN0_BASE, object_id, &object, true);

	if ((object.ui32Flags & MSG_OBJ_NEW_DATA) == 0)
		return false;

	packet.length = object.ui32MsgLen;
	HA_CAN_number_to_packet_id(object.ui32MsgID, &packet.id);

	return true;
}
