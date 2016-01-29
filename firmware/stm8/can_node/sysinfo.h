#pragma once

#include "module.h"

/**
 * System information for internal use and for CAN reporting.
 */

CAN_Node_Module* CAN_Node_Sysinfo_get_module();

uint8_t CAN_Node_Sysinfo_get_device_id();
void CAN_Node_Sysinfo_set_device_id(uint8_t value);
