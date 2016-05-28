#include "can.h"
#include "led.h"
#include "module.h"
#include "sysinfo.h"


static inline
uint8_t get_device_id(void)
{
	return CAN_Node_Sysinfo_get_device_id();
}

CAN_InitStatus_TypeDef CAN_Node_CAN_init(void)
{
	CAN_InitStatus_TypeDef status;
	const uint8_t device_id = get_device_id();
	const uint8_t dev_hi = ((device_id >> 5) & 0x03) | 0 /* reserved */;
	const uint8_t dev_lo = ((device_id << 3) & 0xe0) | ((device_id << 1) & 6) | 0 /* IDE */;

	CAN_DeInit();

	/* The system clock is set to 8 MHz. We setup CAN to work with 125 kbit/s speed */
	status = CAN_Init(
		CAN_MasterCtrl_AllDisabled,
		CAN_Mode_Normal,
		CAN_SynJumpWidth_1TimeQuantum,
		CAN_BitSeg1_8TimeQuantum,
		CAN_BitSeg2_7TimeQuantum,
		4);   /* prescaler = 4: baud rate = 8 MHz / 4 / 16 = 125 kbps */

	if (status != CAN_InitStatus_Success)
		return status;

	/* create packet filter */

	CAN_FilterInit(
		CAN_FilterNumber_0,
		ENABLE,
		CAN_FilterMode_IdMask,
		CAN_FilterScale_16Bit,
		dev_hi, dev_lo, 0xff, 0xef,
		0x03  , 0xe6  , 0xff, 0xef);

/*
	CAN_FilterInit(
		CAN_FilterNumber_0,
		ENABLE,
		CAN_FilterMode_IdMask,
		CAN_FilterScale_32Bit,
		0, 0, 0, 0,
		0, 0, 0, 0);
*/

	return status;
}

static uint32_t received_id;
static HA_CAN_PacketId packet_id;

static void handle_received_data(void)
{
	uint8_t rtr, len, i;
	uint8_t device_id = CAN_Node_Sysinfo_get_device_id();
	uint8_t data[8] = {0};

	if (CAN_GetReceivedIDE() != CAN_Id_Extended)
		return;

	received_id = CAN_GetReceivedId();
	HA_CAN_number_to_packet_id(received_id, &packet_id);
	rtr = CAN_GetReceivedRTR();
	len = CAN_GetReceivedDLC();

	if (!rtr)
	{
		if (len > 8)
			return;
		for (i = 0; i < len; ++i)
			data[i] = CAN_GetReceivedData(i);
	}

	if (received_id == HA_CAN_SpecialId_Update)
	{
		if (rtr)
			CAN_Node_CAN_send_reply(1, &device_id);
		else if (len == 2 && data[0] == device_id)
			CAN_Node_Sysinfo_set_device_id(data[1]);

		return;
	}

	if (device_id != packet_id.device_id)
		return;

	CAN_Node_handle_packet(rtr, &packet_id, len, data);
	//CAN_Node_CAN_send_reply(len, data);
}

void CAN_Node_CAN_handle_packets(void)
{
	while (CAN_MessagePending() > 0)
	{
		CAN_Node_Led_blink(CAN_Node_Led_Info, 1);
		CAN_Receive();
		handle_received_data();
		//CAN_FIFORelease();
	}
}

void CAN_Node_CAN_send_reply(uint8_t length, uint8_t* value)
{
	if (CAN_Transmit(received_id, CAN_Id_Extended, CAN_RTR_Data, length, value) == CAN_TxStatus_NoMailBox)
		CAN_Node_Led_blink(CAN_Node_Led_Error, 4);
}

void CAN_Node_CAN_send_data(uint8_t channel_id, uint8_t index, uint8_t length, uint8_t* value)
{
	HA_CAN_PacketId packet_id;
	uint32_t send_id;

	packet_id.priority = 0; /* TODO */
	packet_id.reserved = 1;
	packet_id.device_id = get_device_id();
	packet_id.address = (((uint16_t)channel_id | 0x80) << 8) | index;

	send_id = HA_CAN_packet_id_to_number(&packet_id);
	CAN_Transmit(send_id, CAN_Id_Extended, CAN_RTR_Data, length, value);
}
