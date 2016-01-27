#ifndef __STM8_CAN_NODE_ERROR_H_INCLUDED__
#define __STM8_CAN_NODE_ERROR_H_INCLUDED__

#include <stdint.h>

/**
 * Here we define a list of recoverable errors.
 */

enum CAN_Node_ErrorCode
{
	CAN_Node_NoError = 0,
	CAN_Node_RuntimeError
};

typedef uint8_t CAN_Node_Error;

#endif
