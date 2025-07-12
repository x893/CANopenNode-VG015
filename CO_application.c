#include "CO_application.h"
#include "OD.h"

void led_RUN_set(int on);
void led_ERROR_set(int on);
void led_USER1_set(int on);
void led_USER2_set(int on);

/******************************************************************************/
CO_ReturnError_t app_programStart(uint16_t *bitRate, uint8_t *nodeId, uint32_t *errInfo)
{
	led_RUN_set(1);
	led_ERROR_set(0);

	/* Place for peripheral or any other startup configuration. See main_PIC32.c
	 * for defaults. */

	/* Set initial CAN bitRate and CANopen nodeId. May be configured by LSS. */
	if (*bitRate == 0)
		*bitRate = 250;
	if (*nodeId == 0)
		*nodeId = 0x7E;

	return CO_ERROR_NO;
}

/******************************************************************************/
void app_communicationReset(CO_t *co)
{
	if (!co->nodeIdUnconfigured)
	{

	}
}

/******************************************************************************/
void app_programEnd()
{
	led_RUN_set(0);
	led_ERROR_set(0);
}

/******************************************************************************/
void app_programAsync(CO_t *co, uint32_t timer1usDiff)
{
	/* Here can be slower code, all must be non-blocking. Mind race conditions
	 * between this functions and following three functions, which all run from
	 * realtime timer interrupt */
}

/******************************************************************************/
void app_programRt(CO_t *co, uint32_t timer1usDiff)
{

}

/******************************************************************************/
void app_peripheralRead(CO_t *co, uint32_t timer1usDiff)
{
	static uint16_t count = 0;
	OD_RAM.x6401_readAnalogInput16_bit[0] = count++;

	/* Read digital inputs */
	static uint8_t digIn = 0;
	OD_RAM.x6000_readDigitalInput8_bit[0] = digIn++;
}

/******************************************************************************/
void app_peripheralWrite(CO_t *co, uint32_t timer1usDiff)
{
	led_RUN_set(CO_LED_GREEN(co->LEDs, CO_LED_CANopen));
	led_ERROR_set(CO_LED_RED(co->LEDs, CO_LED_CANopen));

	/* Write to digital outputs */
	uint8_t digOut = OD_RAM.x6200_writeDigitalOutput8_bit[0];

	led_USER1_set(digOut & 0x01);
	led_USER2_set(digOut & 0x02);

}
