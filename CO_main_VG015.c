#include "CO_driver_target.h"
#include "CANopen.h"
#include "storage/CO_storageEeprom.h"
#include "OD.h"
#include "CO_application.h"

/* Default values for CO_CANopenInit() */
#ifndef NMT_CONTROL
#define NMT_CONTROL \
            CO_NMT_STARTUP_TO_OPERATIONAL \
          | CO_NMT_ERR_ON_ERR_REG \
          | CO_ERR_REG_GENERIC_ERR \
          | CO_ERR_REG_COMMUNICATION
#endif

#ifndef FIRST_HB_TIME
#define FIRST_HB_TIME 500
#endif

#ifndef SDO_SRV_TIMEOUT_TIME
#define SDO_SRV_TIMEOUT_TIME 1000
#endif

#ifndef SDO_CLI_TIMEOUT_TIME
#define SDO_CLI_TIMEOUT_TIME 500
#endif

#ifndef SDO_CLI_BLOCK
#define SDO_CLI_BLOCK false
#endif

#ifndef OD_STATUS_BITS
#define OD_STATUS_BITS NULL
#endif

#ifndef CO_RT_THREAD_ISR
#define CO_RT_THREAD_ISR_DEFAULT
#define CO_RT_THREAD_ISR() void timerIsr(void)
#endif

#ifndef CO_RT_THREAD_ISR_FLAG
extern volatile uint32_t timerIsrFlag;
#define CO_RT_THREAD_ISR_FLAG timerIsrFlag
#endif

#ifndef CO_CANRX_ISR
#define CO_CANRX_ISR_DEFAULT
#define CO_CANRX_ISR() void CAN0_IRQHandler(void)
#endif

/* CANopen object */
CO_t *CO = NULL;

/* Active node-id, copied from pendingNodeId in the communication reset */
static uint8_t CO_activeNodeId = 0x7E; // CO_LSS_NODE_ID_ASSIGNMENT;

/* Timer for time measurement */
volatile uint32_t CO_timer_us = 0;

/* Data block for mainline data, which can be stored to non-volatile memory */
typedef struct {
	/* Pending CAN bit rate, can be set by switch or LSS slave. */
	uint16_t pendingBitRate;
	/* Pending CANopen NodeId, can be set by switch or LSS slave. */
	uint8_t pendingNodeId;
} mainlineStorage_t;

static mainlineStorage_t mlStorage = { 0 };

/* callback for storing node id and bitrate */
static bool_t LSScfgStoreCallback(void *object, uint8_t id, uint16_t bitRate) {
	mainlineStorage_t *mainlineStorage = object;
	mainlineStorage->pendingBitRate = bitRate;
	mainlineStorage->pendingNodeId = id;
	return true;
}

/* main ***********************************************************************/
int main(void) {
	CO_ReturnError_t err;
	CO_NMT_reset_cmd_t reset = CO_RESET_NOT;
	bool_t firstRun = true;

#if (CO_CONFIG_STORAGE) & CO_CONFIG_STORAGE_ENABLE
    CO_storage_t storage;
    CO_storage_entry_t storageEntries[] = {
        {
            .addr = &OD_PERSIST_COMM,
            .len = sizeof(OD_PERSIST_COMM),
            .subIndexOD = 2,
            .attr = CO_storage_cmd | CO_storage_restore
        },
        {
            .addr = &mlStorage,
            .len = sizeof(mlStorage),
            .subIndexOD = 4,
            .attr = CO_storage_cmd | CO_storage_auto | CO_storage_restore
        },
        CO_STORAGE_APPLICATION
    };
    uint8_t storageEntriesCount = sizeof(storageEntries)
                                / sizeof(storageEntries[0]);
    uint32_t storageInitError = 0;
#endif

	/* Allocate memory for CANopen objects */
	uint32_t heapMemoryUsed = 0;
	CO = CO_new(NULL, &heapMemoryUsed);
	if (CO == NULL) {
		while (1)
			;
	}

#if (CO_CONFIG_STORAGE) & CO_CONFIG_STORAGE_ENABLE
    err = CO_storageEeprom_init(&storage,
                                CO->CANmodule,
                                NULL,
                                OD_ENTRY_H1010_storeParameters,
                                OD_ENTRY_H1011_restoreDefaultParameters,
                                storageEntries,
                                storageEntriesCount,
                                &storageInitError);

    if (err != CO_ERROR_NO && err != CO_ERROR_DATA_CORRUPT) {
        while (1);
    }
#endif

	/* Configure peripherals */
	CO_PERIPHERAL_CONFIG();

	/* Execute external application code */
	uint32_t errInfo_app_programStart = 0;
	err = app_programStart(&mlStorage.pendingBitRate, &mlStorage.pendingNodeId,
			&errInfo_app_programStart);
	if (err != CO_ERROR_NO) {
		while (1)
			;
	}

	/* verify stored values */
	if (!CO_LSSchkBitrateCallback(NULL, mlStorage.pendingBitRate)) {
		mlStorage.pendingBitRate = 125;
	}
	if (mlStorage.pendingNodeId < 1 || mlStorage.pendingNodeId > 127) {
		mlStorage.pendingNodeId = CO_LSS_NODE_ID_ASSIGNMENT;
	}

	while (reset != CO_RESET_APP) {
		/* CANopen communication reset - initialize CANopen objects *******************/
		uint32_t errInfo;
		static uint32_t CO_timer_us_previous = 0;

		/* disable CAN receive interrupts */
		CO_CANRX_ENABLE(0);

		/* initialize CANopen */
		err = CO_CANinit(CO, NULL, mlStorage.pendingBitRate);
		if (err != CO_ERROR_NO) {
			while (1)
				CO_clearWDT();
		}

		CO_LSS_address_t lssAddress = { .identity = { .vendorID =
				OD_PERSIST_COMM.x1018_identity.vendor_ID, .productCode =
				OD_PERSIST_COMM.x1018_identity.productCode, .revisionNumber =
				OD_PERSIST_COMM.x1018_identity.revisionNumber, .serialNumber =
				OD_PERSIST_COMM.x1018_identity.serialNumber } };
		err = CO_LSSinit(CO, &lssAddress, &mlStorage.pendingNodeId,
				&mlStorage.pendingBitRate);
		if (err != CO_ERROR_NO) {
			while (1)
				CO_clearWDT();
		}

		CO_activeNodeId = mlStorage.pendingNodeId;
		errInfo = 0;

		err = CO_CANopenInit(CO, /* CANopen object */
		NULL, /* alternate NMT */
		NULL, /* alternate em */
		OD, /* Object dictionary */
		OD_STATUS_BITS, /* Optional OD_statusBits */
		NMT_CONTROL, /* CO_NMT_control_t */
		FIRST_HB_TIME, /* firstHBTime_ms */
		SDO_SRV_TIMEOUT_TIME, /* SDOserverTimeoutTime_ms */
		SDO_CLI_TIMEOUT_TIME, /* SDOclientTimeoutTime_ms */
		SDO_CLI_BLOCK, /* SDOclientBlockTransfer */
		CO_activeNodeId, &errInfo);
		if (err != CO_ERROR_NO && err != CO_ERROR_NODE_ID_UNCONFIGURED_LSS) {
			while (1)
				CO_clearWDT();
		}

		/* Emergency messages in case of errors */
		if (!CO->nodeIdUnconfigured) {
			if (errInfo == 0)
				errInfo = errInfo_app_programStart;
			if (errInfo != 0) {
				CO_errorReport(CO->em, CO_EM_INCONSISTENT_OBJECT_DICT,
						CO_EMC_DATA_SET, errInfo);
			}
#if (CO_CONFIG_STORAGE) & CO_CONFIG_STORAGE_ENABLE
            if (storageInitError != 0) {
                CO_errorReport(CO->em, CO_EM_NON_VOLATILE_MEMORY,
                               CO_EMC_HARDWARE, storageInitError);
            }
#endif
		}

		/* initialize callbacks */
		CO_LSSslave_initCkBitRateCall(CO->LSSslave, NULL,
				CO_LSSchkBitrateCallback);
		CO_LSSslave_initCfgStoreCall(CO->LSSslave, &mlStorage,
				LSScfgStoreCallback);

		/* First time only initialization. */
		if (firstRun) {
			firstRun = false;

			/* Configure real time thread and CAN receive interrupt */
			CO_RT_THREAD_CONFIG();CO_CANRX_CONFIG();

			CO_timer_us_previous = CO_timer_us;
		} /* if(firstRun) */

		/* Execute external application code */
		app_communicationReset(CO);

		errInfo = 0;
		err = CO_CANopenInitPDO(CO, /* CANopen object */
		CO->em, /* emergency object */
		OD, /* Object dictionary */
		CO_activeNodeId, &errInfo);
		if (err != CO_ERROR_NO && err != CO_ERROR_NODE_ID_UNCONFIGURED_LSS) {
			while (1)
				CO_clearWDT();
		}

		/* start CAN and enable interrupts */
		CO_CANsetNormalMode(CO->CANmodule);
		CO_RT_THREAD_ENABLE(1);
		CO_CANRX_ENABLE(1);
		reset = CO_RESET_NOT;

		while (reset == CO_RESET_NOT) {
			/* loop for normal program execution ******************************************/

			/* calculate time difference since last cycle */
			uint32_t timer_us_copy = CO_timer_us;
			uint32_t timeDifference_us = timer_us_copy - CO_timer_us_previous;
			CO_timer_us_previous = timer_us_copy;

			CO_clearWDT();

			/* process CANopen objects */
			reset = CO_process(CO, false, timeDifference_us, NULL);

			CO_clearWDT();

			/* Execute external application code */
			app_programAsync(CO, timeDifference_us);

			CO_clearWDT();

#if (CO_CONFIG_STORAGE) & CO_CONFIG_STORAGE_ENABLE
            CO_storageEeprom_auto_process(&storage, false);
#endif
		}
	} /* while(reset != CO_RESET_APP */

	/* program exit ***************************************************************/
	CO_RT_THREAD_ENABLE(0);
	CO_CANRX_ENABLE(0);

	/* Execute external application code */
	app_programEnd();

#if (CO_CONFIG_STORAGE) & CO_CONFIG_STORAGE_ENABLE
    CO_storageEeprom_auto_process(&storage, true);
#endif

	/* delete objects from memory */
	CO_CANsetConfigurationMode(CO->CANmodule->CANptr);
	CO_delete(CO);

	while (1)
	{
		__BKPT();
	}
}

/* timer interrupt function executes every millisecond ************************/
#ifdef CO_RT_THREAD_ISR_DEFAULT
CO_RT_THREAD_ISR() {
	CO_timer_us += CO_RT_THREAD_INTERVAL_US;

	/* Execute external application code */
	app_peripheralRead(CO, CO_RT_THREAD_INTERVAL_US);

	CO_RT_THREAD_ISR_FLAG = 0;

	/* No need to CO_LOCK_OD(co->CANmodule); this is interrupt */
	if (!CO->nodeIdUnconfigured && CO->CANmodule->CANnormal) {
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
		if (CO_RT_THREAD_ISR_FLAG == 1) {
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
CO_CANRX_ISR() {
	CO_CANinterrupt(CO->CANmodule);
}
#endif
