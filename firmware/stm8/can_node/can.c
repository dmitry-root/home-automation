#include "can.h"
#include "led.h"
#include "module.h"
#include "sysinfo.h"


static inline
uint8_t get_device_id()
{
	return CAN_Node_Sysinfo_get_device_id();
}

CAN_InitStatus_TypeDef CAN_Node_CAN_init()
{
	CAN_InitStatus_TypeDef status;
	const uint8_t device_id = get_device_id();
	uint8_t fr[4], fm[4];
	HA_CAN_PacketId pid = {0};
	HA_CAN_PacketId pid_mask = {0};
	uint32_t address, mask;

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
	pid.device_id = device_id;
	pid.reserved = 1;
	pid_mask.device_id = 0x7f;
	pid_mask.reserved = 1;
	address = HA_CAN_packet_id_to_number(&pid);
	mask = HA_CAN_packet_id_to_number(&pid_mask);

	fr[0] = (uint8_t)(address >> 21);
	fm[0] = (uint8_t)(mask >> 21);
	fr[1] = ((uint8_t)(address >> 13) & 0xe0) | 0x80 /* IDE */ | ((uint8_t)(address >> 15) & 0x5);
	fm[1] = ((uint8_t)(mask    >> 13) & 0xe0) | 0x80 /* IDE */ | ((uint8_t)(mask    >> 15) & 0x5);
	fr[2] = (uint8_t)(address >> 7);
	fm[2] = (uint8_t)(mask    >> 7);
	fr[3] = (uint8_t)(address << 1);
	fm[3] = (uint8_t)(mask    << 1);

	CAN_FilterInit(
		CAN_FilterNumber_0,
		ENABLE,
		CAN_FilterMode_IdMask,
		CAN_FilterScale_32Bit,
		fr[0], fr[1], fr[2], fr[3],
		fm[0], fm[1], fm[2], fm[3]);

	/* Filter for special packet id */
	CAN_FilterInit(
		CAN_FilterNumber_1,
		ENABLE,
		CAN_FilterMode_IdMask,
		CAN_FilterScale_32Bit,
		0xff, 0xef /* RTR = 0 */, 0xff, 0xfe,
		0xff, 0xff, 0xff, 0xfe);

	CAN_ITConfig(CAN_IT_FMP, ENABLE);

	return status;
}

static HA_CAN_PacketId packet_id;

static void handle_received_data()
{
	uint32_t received_id;
	uint8_t rtr, len, i;
	uint8_t data[8];

	if (CAN_GetReceivedIDE() != CAN_Id_Extended)
		return;

	received_id = CAN_GetReceivedId();
	HA_CAN_number_to_packet_id(received_id, &packet_id);
	rtr = CAN_GetReceivedRTR();
	len = CAN_GetReceivedDLC();

	if (len > 8)
		return;

	for (i = 0; i < len; ++i)
		data[i] = CAN_GetReceivedData(i);

	if (received_id == HA_CAN_SpecialId_Update)
	{
		if (len == 1)
			CAN_Node_Sysinfo_set_device_id(data[0]);
		return;
	}

	CAN_Node_handle_packet(rtr, &packet_id, len, data);
}

void CAN_Node_CAN_handle_packets()
{
	while (CAN_MessagePending() > 0)
	{
		CAN_Node_Led_blink(CAN_Node_Led_Info, 1);
		CAN_Receive();
		handle_received_data();
		CAN_FIFORelease();
	}
}

void CAN_Node_CAN_send_reply(uint8_t length, uint8_t* value)
{
	uint32_t send_id;

	send_id = HA_CAN_packet_id_to_number(&packet_id);

	CAN_Transmit(send_id, CAN_Id_Extended, CAN_RTR_Data, length, value);
}

void CAN_Node_CAN_send_data(uint8_t channel_id, uint8_t index, uint8_t length, uint8_t* value)
{
	HA_CAN_PacketId packet_id;
	uint32_t send_id;

	packet_id.priority = 0; /* TODO */
	packet_id.reserved = 1;
	packet_id.device_id = get_device_id();
	packet_id.address = (((uint16_t)channel_id + 2) << 8) | index;

	send_id = HA_CAN_packet_id_to_number(&packet_id);
	CAN_Transmit(send_id, CAN_Id_Extended, CAN_RTR_Data, length, value);
}
