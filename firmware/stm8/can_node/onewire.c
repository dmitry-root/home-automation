#include "onewire.h"
#include "stm8s.h"
#include "stm8s_tim1.h"
#include "stm8s_gpio.h"


static GPIO_TypeDef* const    OW_PORT = GPIOC;
static const GPIO_Pin_TypeDef OW_IN   = GPIO_PIN_1;
static const GPIO_Pin_TypeDef OW_OUT  = GPIO_PIN_2;

enum
{
	RESET_TIME = 100,
	WRITE1_TIME = 5,
	READ_PULSE = 2,
	READ_TIME = 7
};


void CAN_Node_OW_init(void)
{
	GPIO_Init(OW_PORT, OW_IN, GPIO_MODE_IN_FL_NO_IT); // external pull up
	GPIO_Init(OW_PORT, OW_OUT, GPIO_MODE_OUT_PP_LOW_FAST);
	GPIO_WriteLow(OW_PORT, OW_OUT);

	// Configure TIM1 to run at 1MHz frequency.
	// NOTE assuming master clock frequency is 8MHz.
	TIM1_DeInit();
	TIM1_TimeBaseInit(7, TIM1_COUNTERMODE_UP, 60, 0);
	TIM1_SelectOnePulseMode(TIM1_OPMODE_SINGLE);
}


void CAN_Node_OW_deinit(void)
{
	GPIO_Init(OW_PORT, OW_OUT, GPIO_MODE_IN_FL_NO_IT);
	TIM1_DeInit();
}


/**
 * Reset 1-wire bus (~1 ms).
 */
static inline uint8_t reset(void)
{
	uint8_t tmp = 0;
	uint8_t result = 0;

	TIM1_SetAutoreload(485);
	TIM1->CR1 |= TIM1_CR1_CEN;
	OW_PORT->ODR |= OW_OUT;
	while (TIM1->CR1 & TIM1_CR1_CEN);
	OW_PORT->ODR &= ~OW_OUT;
	TIM1_ClearFlag(TIM1_FLAG_UPDATE);

	// Wait once again, sample within 100 mks
	TIM1->CR1 |= TIM1_CR1_CEN;
	while (TIM1->CR1 & TIM1_CR1_CEN)
	{
		tmp = TIM1->CNTRH;
		if (TIM1->CNTRL >= RESET_TIME)
			break;
	}
	result = (OW_PORT->IDR & OW_IN) == 0;

	// Wait for the end of reset period.
	while (TIM1->CR1 & TIM1_CR1_CEN);
	TIM1_ClearFlag(TIM1_FLAG_UPDATE);

	TIM1_SetAutoreload(60);
	return result;
}


/**
 * 1-wire write 0 cycle (60 mks).
 * Drive line low for the whole cycle.
 *
 * \pre TIM1 is configured to 1 MHz, 60 mks period.
 * \pre TIM1 counter is set to 0.
 */
static inline void write_0(void)
{
	TIM1->CR1 |= TIM1_CR1_CEN;
	OW_PORT->ODR |= OW_OUT; // Drives line low.
	while (TIM1->CR1 & TIM1_CR1_CEN);
	OW_PORT->ODR &= ~OW_OUT; // Release line.
	TIM1_ClearFlag(TIM1_FLAG_UPDATE);
}


/**
 * 1-wire write 1 cycle (60 mks).
 * Drive line low for ~5 mks, then drive it high.
 *
 * \pre TIM1 is configured to 1 MHz, 60 mks period.
 * \pre TIM1 counter is set to 0.
 */
static inline void write_1(void)
{
	uint8_t tmp;

	TIM1->CR1 |= TIM1_CR1_CEN;
	OW_PORT->ODR |= OW_OUT; // Drives line low.
	while (TIM1->CR1 & TIM1_CR1_CEN)
	{
		tmp = TIM1->CNTRH;
		if (TIM1->CNTRL >= WRITE1_TIME)
		{
			OW_PORT->ODR &= ~OW_OUT;
			break;
		}
	}
	while (TIM1->CR1 & TIM1_CR1_CEN); // Wait until the end of time slot.
	TIM1_ClearFlag(TIM1_FLAG_UPDATE);
}


/**
 * 1-wire read cycle.
 * Drive line low for 1 mks, then release line, wait until remote device drives it and measure release time.
 *
 * \pre TIM1 is configured to 1 MHz, 60 mks period.
 * \pre TIM1 counter is set to 0.
 * \pre TIM1 CC1 is configured to input capture.
 * \returns 0x80 if 1 was read
 */
static inline uint8_t read(void)
{
	uint8_t tmp = 0;
	uint8_t result = 0;

	TIM1->CR1 |= TIM1_CR1_CEN;
	OW_PORT->ODR |= OW_OUT;
	for (;;)
	{
		tmp = TIM1->CNTRH;
		if (TIM1->CNTRL >= READ_PULSE)
			break;
	}
	OW_PORT->ODR &= ~OW_OUT;

	for (;;)
	{
		tmp = TIM1->CNTRH;
		if (TIM1->CNTRL >= READ_TIME)
			break;
	}
	result = (OW_PORT->IDR & OW_IN) ? 0x80 : 0;

	while (TIM1->CR1 & TIM1_CR1_CEN); // Wait until the end of time slot.
	TIM1_ClearFlag(TIM1_FLAG_UPDATE);

	return result;
}


static void write_byte(uint8_t byte)
{
	uint8_t bit;

	for (bit = 0; bit < 8; ++bit)
	{
		if (byte & 1)
			write_1();
		else
			write_0();
		byte >>= 1;
	}
}

static uint8_t read_byte()
{
	uint8_t byte = 0;
	uint8_t bit;

	for (bit = 0; bit < 8; ++bit)
	{
		byte >>= 1;
		byte |= read();
	}

	return byte;
}


CAN_Node_OW_Presence CAN_Node_OW_reset(void)
{
	return reset() ? CAN_Node_OW_Present : CAN_Node_OW_Obsent;
}

void CAN_Node_OW_write_byte(uint8_t byte)
{
	write_byte(byte);
}

void CAN_Node_OW_write_bytes(uint8_t count, const uint8_t* bytes)
{
	uint8_t i;
	for (i = 0; i < count; ++i)
		write_byte(bytes[i]);
}

uint8_t CAN_Node_OW_read_byte(void)
{
	return read_byte();
}

void CAN_Node_OW_read_bytes(uint8_t count, uint8_t* bytes)
{
	uint8_t i;
	for (i = 0; i < count; ++i)
		bytes[i] = read_byte();
}

