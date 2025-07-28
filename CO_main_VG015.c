#include "CO_driver_target.h"
#include "CANopen.h"
#include "storage/CO_storageEeprom.h"
#include "storage/CO_storage.h"

#include "OD.h"
#include "CO_application.h"
#include "CO_Flash.h"

/* Default values for CO_CANopenInit() */
#ifndef NMT_CONTROL
#define NMT_CONTROL	(					\
    CO_NMT_STARTUP_TO_OPERATIONAL	|	\
	CO_ERR_REG_GENERIC_ERR			|	\
	CO_ERR_REG_COMMUNICATION			\
	)
#endif

#ifndef SDO_SRV_TIMEOUT_TIME
#define SDO_SRV_TIMEOUT_TIME	1000
#endif

#ifndef SDO_CLI_TIMEOUT_TIME
#define SDO_CLI_TIMEOUT_TIME	500
#endif

#ifndef FIRST_HB_TIME
#define FIRST_HB_TIME	500
#endif

#ifndef SDO_CLI_BLOCK
#define SDO_CLI_BLOCK	false
#endif

#ifndef OD_STATUS_BITS
#define OD_STATUS_BITS	NULL
#endif

/* CANopen object */
CO_t *CO = NULL;

/* Active node-id, copied from pendingNodeId in the communication reset */
static uint8_t CO_activeNodeId = 0x7E; // CO_LSS_NODE_ID_ASSIGNMENT;

/* Timer for time measurement */
volatile uint32_t CO_timer_us = 0;

/* Data block for mainline data, which can be stored to non-volatile memory */
typedef struct
{
	uint16_t pendingBitRate;	/* Pending CAN bit rate, can be set by switch or LSS slave. */
	uint8_t pendingNodeId;		/* Pending CANopen NodeId, can be set by switch or LSS slave. */
} mainlineStorage_t;
mainlineStorage_t mlStorage;

/* callback for storing node id and bitrate */
static bool_t LSScfgStoreCallback(void *object, uint8_t id, uint16_t bitRate)
{
	mainlineStorage_t *mainlineStorage = object;

	mainlineStorage->pendingBitRate = bitRate;
	OD_PERSIST_COMM.x2102_bitrate = bitRate;

	mainlineStorage->pendingNodeId = id;
	OD_PERSIST_COMM.x2101_nodeID = id;

	return true;
}

#include <stdlib.h>

/* main ***********************************************************************/
int main(void)
{
	CO_ReturnError_t err;

	/* Configure peripherals */
	CO_PERIPHERAL_CONFIG();

	/* Allocate memory for CANopen objects */
	uint32_t heapMemoryUsed = 0;
	CO = CO_new(NULL, &heapMemoryUsed);
	if (CO == NULL)
		Error_Handler();

	__enable_irq();
	OD_INFO(("CANOpenNode VG015 " __DATE__ " " __TIME__ "\n"));

#if (CO_CONFIG_STORAGE) & CO_CONFIG_STORAGE_ENABLE
	uint32_t storageInitError = 0;
	if (CO_Flash_init(CO, &storageInitError) != CO_ERROR_NO)
		Error_Handler();
#endif

	/* Execute external application code */
	uint32_t errInfo_app_programStart = 0;
	if (app_programStart(
			&mlStorage.pendingBitRate,
			&mlStorage.pendingNodeId,
			&errInfo_app_programStart
			) != CO_ERROR_NO)
		Error_Handler();

	/* verify stored values */
	if (!CO_LSSchkBitrateCallback(NULL, mlStorage.pendingBitRate))
		mlStorage.pendingBitRate = 250;

	if (mlStorage.pendingNodeId < 1 || mlStorage.pendingNodeId > 127)
		mlStorage.pendingNodeId = CO_LSS_NODE_ID_ASSIGNMENT;

	// void trng_init(void);
	// trng_init();

	void mongoose_init(void);
	void mongoose_poll(void);
	mongoose_init();

	uint32_t errInfo;
	uint32_t timeDifference_us;
	uint32_t CO_timer_ms_last = CO_timer_ms;
	CO_NMT_reset_cmd_t reset = CO_RESET_NOT;
	bool_t firstRun = true;

	while (reset != CO_RESET_APP)
	{
		/* CANopen communication reset - initialize CANopen objects *******************/

		/* disable CAN receive interrupts */
		CO_CANRX_ENABLE(0);

		/* initialize CANopen */
		if (CO_CANinit(CO, NULL, mlStorage.pendingBitRate) != CO_ERROR_NO)
			Error_Handler();

		CO_LSS_address_t lssAddress =
		{ .identity =
			{ .vendorID = OD_PERSIST_COMM.x1018_identity.vendor_ID, .productCode =
				OD_PERSIST_COMM.x1018_identity.productCode, .revisionNumber =
				OD_PERSIST_COMM.x1018_identity.revisionNumber, .serialNumber =
				OD_PERSIST_COMM.x1018_identity.serialNumber
			}
		};

		if (CO_LSSinit(CO, &lssAddress, &mlStorage.pendingNodeId, &mlStorage.pendingBitRate) != CO_ERROR_NO)
			Error_Handler();

		CO_activeNodeId = mlStorage.pendingNodeId;
		errInfo = 0;

		err = CO_CANopenInit(
							CO, /* CANopen object */
							NULL, /* alternate NMT */
							NULL, /* alternate em */
							OD, /* Object dictionary */
							OD_STATUS_BITS, /* Optional OD_statusBits */
							NMT_CONTROL, /* CO_NMT_control_t */
							FIRST_HB_TIME, /* firstHBTime_ms */
							SDO_SRV_TIMEOUT_TIME, /* SDOserverTimeoutTime_ms */
							SDO_CLI_TIMEOUT_TIME, /* SDOclientTimeoutTime_ms */
							SDO_CLI_BLOCK, /* SDOclientBlockTransfer */
							CO_activeNodeId, &errInfo
							);

		if (err != CO_ERROR_NO && err != CO_ERROR_NODE_ID_UNCONFIGURED_LSS)
			Error_Handler();

		/* Emergency messages in case of errors */
		if (!CO->nodeIdUnconfigured)
		{
			if (errInfo == 0)
				errInfo = errInfo_app_programStart;
			if (errInfo != 0)
			{
				CO_errorReport(
						CO->em,
						CO_EM_INCONSISTENT_OBJECT_DICT,
						CO_EMC_DATA_SET, errInfo);
			}
#if (CO_CONFIG_STORAGE) & CO_CONFIG_STORAGE_ENABLE
            if (storageInitError != 0) {
                CO_errorReport(
                		CO->em,
						CO_EM_NON_VOLATILE_MEMORY,
						CO_EMC_HARDWARE,
						storageInitError);
            }
#endif
		}

		/* initialize callbacks */
		CO_LSSslave_initCkBitRateCall(CO->LSSslave, NULL, CO_LSSchkBitrateCallback);
		CO_LSSslave_initCfgStoreCall(CO->LSSslave, &mlStorage, LSScfgStoreCallback);

		/* First time only initialization. */
		if (firstRun)
		{
			firstRun = false;

			/* Configure real time thread and CAN receive interrupt */
			CO_RT_THREAD_CONFIG();
			CO_CANRX_CONFIG();
			CO_timer_ms_last = CO_timer_ms;
		} /* if(firstRun) */

		/* Execute external application code */
		app_communicationReset(CO);

		errInfo = 0;
		err = CO_CANopenInitPDO(
							CO, /* CANopen object */
							CO->em, /* emergency object */
							OD, /* Object dictionary */
							CO_activeNodeId, &errInfo
							);
		if (err != CO_ERROR_NO && err != CO_ERROR_NODE_ID_UNCONFIGURED_LSS)
			Error_Handler();

		/* start CAN and enable interrupts */
		CO_CANsetNormalMode(CO->CANmodule);
		CO_RT_THREAD_ENABLE(1);
		CO_CANRX_ENABLE(1);
		reset = CO_RESET_NOT;

		while (reset == CO_RESET_NOT)
		{
			mongoose_poll();

			/* loop for normal program execution ******************************************/

			/* calculate time difference since last cycle */
			if (CO_timer_ms_last != CO_timer_ms)
			{
				timeDifference_us = (CO_timer_ms - CO_timer_ms_last) * CO_RT_THREAD_INTERVAL_US;
				CO_timer_ms_last = CO_timer_ms;
				CO_clearWDT();
#ifdef CO_RT_THREAD_ISR_DEFAULT
				CO_RT_THREAD_ISR();
#endif
			}

			/* process CANopen objects */
			reset = CO_process(CO, false, timeDifference_us, NULL);
			CO_clearWDT();

			/* Execute external application code */
			app_programAsync(CO, timeDifference_us);
			CO_clearWDT();

#if (CO_CONFIG_STORAGE) & CO_CONFIG_STORAGE_ENABLE
			CO_Flash_auto_process(false);
#endif
			timeDifference_us = 0;
		}
	} /* while(reset != CO_RESET_APP */

	/* program exit ***************************************************************/
	CO_RT_THREAD_ENABLE(0);
	CO_CANRX_ENABLE(0);

	/* Execute external application code */
	app_programEnd();

#if (CO_CONFIG_STORAGE) & CO_CONFIG_STORAGE_ENABLE
	CO_Flash_auto_process(true);
#endif

	/* delete objects from memory */
	CO_CANsetConfigurationMode(CO->CANmodule->CANptr);
	CO_delete(CO);

	Error_Handler();
}

/* timer interrupt function executes every millisecond ************************/
#ifdef CO_RT_THREAD_ISR_DEFAULT
void CO_RT_THREAD_ISR()
{
	CO_timer_us += CO_RT_THREAD_INTERVAL_US;

	/* Execute external application code */
	app_peripheralRead(CO, CO_RT_THREAD_INTERVAL_US);

	CO_RT_THREAD_ISR_FLAG = 0;

	/* No need to CO_LOCK_OD(co->CANmodule); this is interrupt */
	if (!CO->nodeIdUnconfigured && CO->CANmodule->CANnormal)
	{
		bool_t syncWas = false;

#if (CO_CONFIG_SYNC) & CO_CONFIG_SYNC_ENABLE
		syncWas = CO_process_SYNC(CO, CO_RT_THREAD_INTERVAL_US, NULL);
#endif
#if (CO_CONFIG_PDO) & CO_CONFIG_RPDO_ENABLE
		CO_process_RPDO(CO, syncWas, CO_RT_THREAD_INTERVAL_US, NULL);
#endif

		/* Execute external application code */
		app_programRt(CO, CO_RT_THREAD_INTERVAL_US);

#if (CO_CONFIG_PDO) & CO_CONFIG_TPDO_ENABLE
		CO_process_TPDO(CO, syncWas, CO_RT_THREAD_INTERVAL_US, NULL);
#endif

		/* verify timer overflow */
		if (CO_RT_THREAD_ISR_FLAG == 1)
		{
			CO_errorReport(CO->em, CO_EM_ISR_TIMER_OVERFLOW,
					CO_EMC_SOFTWARE_INTERNAL, 0);
			CO_RT_THREAD_ISR_FLAG = 0;
		}

		(void) syncWas;
	}

	/* Execute external application code */
	app_peripheralWrite(CO, CO_RT_THREAD_INTERVAL_US);
}
#endif /* CO_RT_THREAD_ISR_DEFAULT */

#ifdef CO_CANRX_ISR_DEFAULT
CO_CANRX_ISR()
{
	CO_CANinterrupt(CO->CANmodule);
}
#endif

void Error_Handler(void)
{
	__disable_irq();
	led_reset(LEDS);
	while (1)
	{
		led_toggle(LEDS);
		for(uint32_t i = 0; i < 4000000UL; i++)
			__NOP();
	}
}
