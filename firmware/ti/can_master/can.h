#pragma once

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/hal/Hwi.h>

#include "ha/can_proto.h"
#include "util/NonCopyable.h"


struct CanPacket
{
	HA_CAN_PacketId id = {0};
	uint8_t data[8] = {0};
	size_t length = 0;
};

class CanMaster : util::NonCopyable
{
public:
	~CanMaster();

	static CanMaster& instance();

	bool send(const CanPacket& packet);
	bool receive(CanPacket& packet);

	bool send_request(const HA_CAN_PacketId& packet_id);
	bool receive_response(CanPacket& packet);
	void clear_request();

private:
	CanMaster();

	static void interrupt_handler(UArg arg);

private:
	void setup_rx();
	bool do_send(uint32_t msg_id, const uint8_t* data, size_t length, bool request);

private:
	ti_sysbios_hal_Hwi_Handle hwi_;
};
