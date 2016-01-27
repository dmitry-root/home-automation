#ifndef __HA_CAN_PROTO_H_INCLUDED__
#define __HA_CAN_PROTO_H_INCLUDED__

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 *
 * The CAN bus in Home Automation network always have one master device (Bus Master)
 * and up to 127 slave devices (sensors, actors, etc). Note that each master device
 * controls it's own bus; the overall setup can include more than one such masters, but
 * their CAN network may not overlap. In the other words, each slave device always talks
 * with exacly one master, but the master can talk to up to 127 slaves.
 *
 * The CAN packets in HA network has the following structure.
 *
 * 1. Packet identifier.
 *
 *    Identifiers are always extended (29 bits) and has the following structure:
 *
 *      Bits        | Name              | Description
 *    --------------+-------------------+---------------------------------------
 *     15:0         | Address           | Device-specific address of value to read/write
 *     22:16        | Device ID         | Slave device identifier (0-127)
 *     23           | Direction         | 0: slave -> master, 1: master -> slave
 *     27:24        | Priority          | Message priority bits (0 => highest, 15 => lowest)
 *     28           | Update ID         | Special bit for programming device ID
 *
 *
 * 2. RTR bit
 *
 * The CAN RTR bit is used as described in CAN protocol:
 *
 *      RTR     | Direction       | Meaning
 *    ----------+-----------------+-------------------------------
 *     0 (data) | 0 (s->m)        | Write value to slave at specified address
 *     0 (data) | 1 (m->s)        | Reply with requested value at specified address
 *     1 (req)  | 0 (s->m)        | Meaningless
 *     1 (req)  | 1 (m->s)        | Request a value at specified address
 *
 *
 * 3. Data (0-8 bytes)
 *
 * CAN data field is used to store a device-specific value at selected address. Data size and
 * meaning of the value is device-specific.
 *
 *
 * 4. Programming device ID
 *
 * The special 28'th bit has special meaning. It's used to setup a new device into the network.
 * The device must be the only one connected to the network. All other bits of packet ID must
 * be set to 1. The RTR bit is set to 0, and packet data contains 1 byte with value in range
 * 0-127. The value of that byte sets a new device id for the device being programmed.
 *
 * The device must implement a special hardware-specific check to verify that this command is allowed.
 * For example, some special button may be pressed, or a jumper is set, etc. If it's not possible,
 * the device must not implement this command at all. The device ID is then programmed by some other
 * protocol, e.g. the microcontroller debug interface.
 *
 */


typedef struct HA_CAN_PacketId_s
{
	uint8_t priority;
	uint8_t direction : 1;
	uint8_t device_id : 7;
	uint16_t address;
} HA_CAN_PacketId;

static inline
uint32_t HA_CAN_packet_id_to_number(const HA_CAN_PacketId* packet_id)
{
	return
		((uint32_t)(packet_id->priority & 0x0f) << 24) |
		((uint32_t)packet_id->direction << 23) |
		((uint32_t)packet_id->device_id << 16) |
		packet_id->address;
}

static inline
void HA_CAN_number_to_packet_id(uint32_t value, HA_CAN_PacketId* result)
{
	result->priority = (value >> 24) & 0x0f;
	result->direction = (value >> 23) & 1;
	result->device_id = (value >> 16) & 0x7f;
	result->address = value & 0xffff;
}


enum HA_CAN_SpecialId
{
	HA_CAN_SpecialId_Update = 0x1fffffff
};



/**
 *
 * Address space common for all slave devices.
 *
 * The reserved address space is 0x0000 .. 0x00ff.
 *
 * All numbers are transferred with big-endian byte order.
 *
 */
enum HA_CAN_CommonMap
{
	/* == Device information, 0x00 .. 0x0f. Read only. == */
	HA_CAN_Common_DeviceType = 0x00,        /*!< Type of the device, 4 bytes (hi word: vendor id, lo word: device id) */
	HA_CAN_Common_DeviceRevision = 0x01,    /*!< Revision number, 2 bytes */
	HA_CAN_Common_SerialHi = 0x02,          /*!< Hi part of 128-bit serial, 8 bytes */
	HA_CAN_Common_SerialLo = 0x03,          /*!< Lo part of 128-bit serial, 8 bytes */
};


#ifdef __cplusplus
}
#endif

#endif
