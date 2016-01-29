#pragma once

#include "stm8s.h"

extern volatile uint8_t g_lock_count;


/**
 * Recursive lock of interrupts handling.
 */
static inline
void int_lock()
{
	sim(); // disable interrupts
	++g_lock_count;
}

/**
 * Recursive unlock of interrupts handling.
 */
static inline
void int_unlock()
{
	if (--g_lock_count == 0)
		rim();
}


/* Atomic access to integer value */
static inline
void atomic_increment(uint8_t* value)
{
	int_lock();
	++(*value);
	int_unlock();
}

static inline
void atomic_decrement(uint8_t* value)
{
	int_lock();
	--(*value);
	int_unlock();
}
