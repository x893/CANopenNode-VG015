#ifndef CO_DRIVER_TARGET_H
#define CO_DRIVER_TARGET_H

/* This file contains device and application specific definitions.
 * It is included from CO_driver.h, which contains documentation
 * for common definitions below. */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef CO_CANRX_ENABLE
void can_enable(int ENABLE);
#define CO_CANRX_ENABLE(ENABLE)	can_enable(ENABLE)
#endif

static inline void __BKPT(void)
{
	asm("ebreak \n"
			"nop    \n");
}

static inline void __NOP(void)
{
	asm("nop    \n");
}

#ifndef CO_clearWDT
#define CO_clearWDT() __NOP()
#endif

/* Interval of the realtime thread */
#ifndef CO_RT_THREAD_INTERVAL_US
#define CO_RT_THREAD_INTERVAL_US 1000
#endif

#ifndef CO_RT_THREAD_CONFIG
#define CO_RT_THREAD_CONFIG()				\
	void timer_init(unsigned int interval); \
	timer_init(CO_RT_THREAD_INTERVAL_US);
#endif

#ifndef CO_RT_THREAD_ENABLE
void timer_enable(int ENABLE);
#define CO_RT_THREAD_ENABLE(ENABLE) timer_enable(ENABLE)
#endif

#ifndef CO_CANRX_CONFIG
#define CO_CANRX_CONFIG()
#endif

unsigned int __builtin_disable_interrupts(void);
void __builtin_enable_interrupts(void);

#ifdef CO_CONFIG_STORAGE
#undef  CO_CONFIG_STORAGE
#endif
#define CO_CONFIG_STORAGE	(0)
#define CO_STORAGE_APPLICATION

void CO_PERIPHERAL_CONFIG(void);
#ifndef MSTATUS_MIE
#define MSTATUS_MIE   (1 << 3)
#endif

/* Stack configuration override from CO_driver.h.
 * For more information see file CO_config.h. */
#ifndef CO_CONFIG_NMT
#define CO_CONFIG_NMT	CO_CONFIG_NMT_MASTER
#endif

/* Add full SDO_SRV_BLOCK transfer with CRC16 */
#ifndef CO_CONFIG_SDO_SRV
#define CO_CONFIG_SDO_SRV (CO_CONFIG_SDO_SRV_SEGMENTED | \
                           CO_CONFIG_SDO_SRV_BLOCK)
#endif
#ifndef CO_CONFIG_SDO_SRV_BUFFER_SIZE
#define CO_CONFIG_SDO_SRV_BUFFER_SIZE 900
#endif
#ifndef CO_CONFIG_CRC16
#define CO_CONFIG_CRC16 (CO_CONFIG_CRC16_ENABLE)
#endif

/* default system clock configuration */
#ifndef CO_FSYS
#define CO_FSYS 64000 /* (8MHz Quartz used) */
#endif

/* default peripheral bus clock configuration */
#ifndef CO_PBCLK
#define CO_PBCLK 32000
#endif

/* Basic definitions */
#define CO_LITTLE_ENDIAN
#define CO_SWAP_16(x) x
#define CO_SWAP_32(x) x
#define CO_SWAP_64(x) x
/* NULL is defined in stddef.h */
/* true and false are defined in stdbool.h */
/* int8_t to uint64_t are defined in stdint.h */
typedef unsigned char bool_t;
typedef float float32_t;
typedef long double float64_t;

/* CAN receive message structure as aligned in CAN module. */
typedef struct
{
	uint16_t ident; /* Standard Identifier */
	uint8_t data[8];/* 8 data bytes */
	uint8_t dlc; /* Data length code (bits 0...3) */
} CO_CANrxMsg_t;

/* Access to received CAN message */
#define CO_CANrxMsg_readIdent(msg) ((uint16_t)(((CO_CANrxMsg_t *) (msg))->ident))
#define CO_CANrxMsg_readDLC(msg)   ((uint8_t)(((CO_CANrxMsg_t *)  (msg))->dlc))
#define CO_CANrxMsg_readData(msg)  ((uint8_t *)(((CO_CANrxMsg_t *)(msg))->data))

/* Received message object */
typedef struct
{
	uint16_t ident;
	uint16_t mask;
	void *object;
	void (*CANrx_callback)(void *object, void *message);
	uint8_t dlc;
} CO_CANrx_t;

/* Transmit message object */
typedef struct
{
	uint32_t ident;
	uint8_t data[8];
	volatile bool_t bufferFull;
	volatile bool_t syncFlag;
	uint8_t dlc;
} CO_CANtx_t;

/* CAN module object */
typedef struct
{
	void *CANptr;

	CO_CANrx_t *rxArray;
	uint16_t rxSize;

	CO_CANtx_t *txArray;
	uint16_t txSize;

	uint16_t CANerrorStatus;
	volatile bool_t CANnormal;
	volatile bool_t useCANrxFilters;
	volatile bool_t bufferInhibitFlag;
	volatile bool_t firstCANtxMessage;
	volatile uint16_t CANtxCount;
	uint32_t errOld;
	unsigned int interruptStatus; /* for enabling/disabling interrupts */
	unsigned int interruptDisabler; /* for enabling/disabling interrupts */
} CO_CANmodule_t;

/* Data storage object for one entry */
typedef struct
{
	void *addr;
	size_t len;
	uint8_t subIndexOD;
	uint8_t attr;
	void *storageModule;
	uint16_t crc;
	size_t eepromAddrSignature;
	size_t eepromAddr;
	size_t offset;
} CO_storage_entry_t;

/* (un)lock critical section in CO_CANsend() */
#define CO_LOCK_CAN_SEND(CAN_MODULE) { \
    if ((CAN_MODULE)->interruptDisabler == 0) { \
        (CAN_MODULE)->interruptStatus = __builtin_disable_interrupts(); \
        (CAN_MODULE)->interruptDisabler = 1; \
    } \
}

#define CO_UNLOCK_CAN_SEND(CAN_MODULE) { \
    if ((CAN_MODULE)->interruptDisabler == 1) { \
        if(((CAN_MODULE)->interruptStatus & MSTATUS_MIE) != 0) { \
            __builtin_enable_interrupts(); \
        } \
        (CAN_MODULE)->interruptDisabler = 0; \
    } \
}

/* (un)lock critical section in CO_errorReport() or CO_errorReset() */
#define CO_LOCK_EMCY(CAN_MODULE) { \
    if ((CAN_MODULE)->interruptDisabler == 0) { \
        (CAN_MODULE)->interruptStatus = __builtin_disable_interrupts(); \
        (CAN_MODULE)->interruptDisabler = 2; \
    } \
}

#define CO_UNLOCK_EMCY(CAN_MODULE) { \
    if ((CAN_MODULE)->interruptDisabler == 2) { \
        if(((CAN_MODULE)->interruptStatus & MSTATUS_MIE) != 0) { \
            __builtin_enable_interrupts(); \
        } \
        (CAN_MODULE)->interruptDisabler = 0; \
    } \
}

/* (un)lock critical section when accessing Object Dictionary */
#define CO_LOCK_OD(CAN_MODULE) { \
    if ((CAN_MODULE)->interruptDisabler == 0) { \
        (CAN_MODULE)->interruptStatus = __builtin_disable_interrupts(); \
        (CAN_MODULE)->interruptDisabler = 3; \
    } \
}
#define CO_UNLOCK_OD(CAN_MODULE) { \
    if ((CAN_MODULE)->interruptDisabler == 3) { \
        if(((CAN_MODULE)->interruptStatus & MSTATUS_MIE) != 0) { \
            __builtin_enable_interrupts(); \
        } \
        (CAN_MODULE)->interruptDisabler = 0; \
    } \
}

/* Synchronization between CAN receive and message processing threads. */
#define CO_MemoryBarrier()
#define CO_FLAG_READ(rxNew)		((rxNew) != NULL)
#define CO_FLAG_SET(rxNew)		{ CO_MemoryBarrier(); rxNew = (void*)1L; }
#define CO_FLAG_CLEAR(rxNew)	{CO_MemoryBarrier(); rxNew = NULL;}

/* Translate a kernel virtual address in KSEG0 or KSEG1 to a real
 * physical address and back. */
typedef unsigned long CO_paddr_t; /* a physical address */
typedef unsigned long CO_vaddr_t; /* a virtual address */

#define CO_KVA_TO_PA(v)         ((CO_paddr_t)(v) & 0x1fffffff)
#define CO_PA_TO_KVA0(pa)       ((void *) ((pa) | 0x80000000))
#define CO_PA_TO_KVA1(pa)       ((void *) ((pa) | 0xa0000000))

/* Callback for checking bitrate, needed by LSS slave */
bool_t CO_LSSchkBitrateCallback(void *object, uint16_t bitRate);

/* Function called from CAN receive interrupt handler */
void CO_CANinterrupt(CO_CANmodule_t *CANmodule);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CO_DRIVER_TARGET_H */
