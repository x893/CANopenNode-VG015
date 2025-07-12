#include <string.h>

#include "K1921VG015.h"
#include "system_k1921vg015.h"

#include "301/CO_driver.h"

unsigned int __builtin_disable_interrupts(void)
{
	//disable all interrupts in machine mode
	PLIC_SetThreshold(Plic_Mach_Target, 7); //Disable all interrupts in machine mode

	// disable machine external interrupt
	clear_csr(mie, MIE_MEXTERNAL);

	// disable global interrupts
	uint32_t state = read_csr(mstatus) & MSTATUS_MIE;
	clear_csr(mstatus, MSTATUS_MIE);

	return state;
}

void __builtin_enable_interrupts(void)
{
	//allow all interrupts in machine mode
	PLIC_SetThreshold(Plic_Mach_Target, 0); //allow all interrupts in machine mode
	// disable timer interrupt
	clear_csr(mie, MIE_MTIMER);
	// enable machine external interrupt
	set_csr(mie, MIE_MEXTERNAL);
	// enable global interrupts
	set_csr(mstatus, MSTATUS_MIE);
}
void __enable_irq(void) __attribute((alias("__builtin_enable_interrupts")));

#include "retarget.h"

#define SystemCoreClock_uart	16000000

//-- Functions -----------------------------------------------------------------
void retarget_init(void)
{
#if defined RETARGET
	uint32_t baud_icoef = SystemCoreClock_uart / (16 * RETARGET_UART_BAUD);
	uint32_t baud_fcoef = ((SystemCoreClock_uart / (16.0f * RETARGET_UART_BAUD)
			- baud_icoef) * 64 + 0.5f);
	uint32_t uartclk_ref = 1;	//RCU_UARTCFG_UARTCFG_CLKSEL_OSICLK;

	RCU->CGCFGAHB_bit.GPIOAEN = 1;
	RCU->RSTDISAHB_bit.GPIOAEN = 1;
	RCU->CGCFGAPB_bit.UART0EN = 1;
	RCU->RSTDISAPB_bit.UART0EN = 1;

	RETARGET_UART_PORT->ALTFUNCNUM_bit.PIN0 = 1;
	RETARGET_UART_PORT->ALTFUNCNUM_bit.PIN1 = 1;
	RETARGET_UART_PORT->ALTFUNCSET = (1 << RETARGET_UART_PIN_TX_POS)
			| (1 << RETARGET_UART_PIN_RX_POS);
	RCU->UARTCLKCFG[RETARGET_UART_NUM].UARTCLKCFG = (uartclk_ref
			<< RCU_UARTCLKCFG_CLKSEL_Pos) |
	RCU_UARTCLKCFG_CLKEN_Msk |
	RCU_UARTCLKCFG_RSTDIS_Msk;
	RETARGET_UART->IBRD = baud_icoef;
	RETARGET_UART->FBRD = baud_fcoef;
	RETARGET_UART->LCRH = UART_LCRH_FEN_Msk | (3 << UART_LCRH_WLEN_Pos);
	RETARGET_UART->CR = UART_CR_TXE_Msk | UART_CR_RXE_Msk | UART_CR_UARTEN_Msk;
#endif //RETARGET
}

void timerIsr(void);
volatile uint32_t timerIsrFlag = 0;
uint32_t timerIsrActive = 0;

volatile uint32_t CO_timer_ms;

void TMR32_IRQHandler()
{
	TMR32->IC_bit.CAP0 = 1;
	CO_timer_ms++;
	timerIsrFlag = 1;
	if (timerIsrActive == 0)
	{
		timerIsrActive = 1;
		timerIsr();
	}
}

void timer_enable(int ENABLE)
{
	TMR32->COUNT = 0;
	TMR32->IM_bit.CAP0 = (ENABLE) ? 1 : 0;
}

void timer_init(uint32_t period)
{
	RCU->CGCFGAPB_bit.TMR32EN = 1;
	RCU->RSTDISAPB_bit.TMR32EN = 1;

	period = SystemCoreClock / period;

	TMR32->CAPCOM[0].VAL = period - 1;
	TMR32->CTRL_bit.MODE = TMR32_CTRL_MODE_Up;

	PLIC_SetIrqHandler(Plic_Mach_Target, IsrVect_IRQ_TMR32, TMR32_IRQHandler);
	PLIC_SetPriority(IsrVect_IRQ_TMR32, 5);
	PLIC_IntEnable(Plic_Mach_Target, IsrVect_IRQ_TMR32);
}

#define LEDS_MSK  0xF000
#define LED4_MSK  (1 << 12)
#define LED5_MSK  (1 << 13)
#define LED6_MSK  (1 << 14)
#define LED7_MSK  (1 << 15)

void led_RUN_set(int on)
{
	if (on)
		GPIOA->DATAOUTSET = LED4_MSK;
	else
		GPIOA->DATAOUTCLR = LED4_MSK;
}

void led_ERROR_set(int on)
{
	if (on)
		GPIOA->DATAOUTSET = LED5_MSK;
	else
		GPIOA->DATAOUTCLR = LED5_MSK;
}

void led_USER1_set(int on)
{
	if (on)
		GPIOA->DATAOUTSET = LED6_MSK;
	else
		GPIOA->DATAOUTCLR = LED6_MSK;
}

void led_USER2_set(int on)
{
	if (on)
		GPIOA->DATAOUTSET = LED7_MSK;
	else
		GPIOA->DATAOUTCLR = LED7_MSK;
}

void CO_PERIPHERAL_CONFIG(void)
{
	RCU->CGCFGAHB_bit.GPIOAEN = 1;
	RCU->RSTDISAHB_bit.GPIOAEN = 1;
	GPIOA->OUTENSET = LEDS_MSK;
	GPIOA->DATAOUTCLR = LEDS_MSK;

	SystemInit();
	SystemCoreClockUpdate();
	retarget_init();

	__enable_irq();
}

int retarget_put_char(int ch)
{
#if defined RETARGET
	if (ch == '\n')
		retarget_put_char('\r');
	while (RETARGET_UART->FR_bit.BUSY)
	{
	}
	RETARGET_UART->DR = ch;
#else
	(void) ch;
#endif //RETARGET
	return 0;
}

int putchar(int)
{
	return 0;
}

int _write(int fd, char *str, int len)
{
	(void) fd;
	for (int i = 0; i != len; i++)
	{
		retarget_put_char(*str++);
	}
	return len;
}

void can_enable(int ENABLE)
{
	if (ENABLE)
		PLIC_IntEnable(Plic_Mach_Target, IsrVect_IRQ_CAN0);
	else
		PLIC_IntDisable(Plic_Mach_Target, IsrVect_IRQ_CAN0);
}

typedef struct
{
	uint8_t BRP; /* (1...64) Baud Rate Prescaler */
	uint8_t SJW; /* (1...4) SJW time */
	uint8_t TSeg1; /* (1...8) Phase Segment 1 time */
	uint8_t TSeg2; /* (1...8) Phase Segment 2 time */
	uint16_t bitrate; /* bitrate in kbps */
} CO_CANbitRateData_t;

static const CO_CANbitRateData_t CO_CANbitRateData[] =
{
{ 19, 1, 4, 3, 250 }, };
/******************************************************************************/
bool_t CO_LSSchkBitrateCallback(void *object, uint16_t bitRate)
{
	(void) object;
	for (size_t i = 0;
			i < (sizeof(CO_CANbitRateData) / sizeof(CO_CANbitRateData[0])); i++)
		if (CO_CANbitRateData[i].bitrate == bitRate && bitRate > 0)
			return true;
	return false;
}

/**
 * @brief CAN_Object_Location
 */
void CAN_Object_Location(uint32_t obj_first_num, uint32_t obj_last_num,
		uint32_t list_num)
{
	for (uint32_t x = obj_first_num; x <= obj_last_num; x++)
	{
		// PANCMD_field = 0x02 - static location objects to one of the CAN-lists
		CAN->PANCTR = (0x2 << CAN_PANCTR_PANCMD_Pos)
				| (x << CAN_PANCTR_PANAR1_Pos)
				| (list_num << CAN_PANCTR_PANAR2_Pos);
		while ((CAN->PANCTR_bit.BUSY) | (CAN->PANCTR_bit.RBUSY))
		{
		}
	}
}

#define CAN0_LIST			1
#define CAN0_LOCATION_FIRST	0

#define CAN0_TX_FIRST		CAN0_LOCATION_FIRST
#define CAN0_TX_LAST		(CAN0_TX_FIRST + 3)

#define CAN0_RX_FIRST		(CAN0_TX_LAST  + 1)
#define CAN0_RX_LAST		(CAN0_RX_FIRST + 3)

#define CAN0_LOCATION_LAST	CAN0_RX_LAST

#define CANMSG_Msg_MOCTR_RESALL ( \
		CANMSG_Msg_MOCTR_RESRXPND_Msk | \
		CANMSG_Msg_MOCTR_RESTXPND_Msk | \
		CANMSG_Msg_MOCTR_RESRXUPD_Msk | \
		CANMSG_Msg_MOCTR_RESNEWDAT_Msk | \
		CANMSG_Msg_MOCTR_RESMSGLST_Msk | \
		CANMSG_Msg_MOCTR_RESMSGVAL_Msk | \
		CANMSG_Msg_MOCTR_RESRTSEL_Msk | \
		CANMSG_Msg_MOCTR_RESRXEN_Msk | \
		CANMSG_Msg_MOCTR_RESTXRQ_Msk | \
		CANMSG_Msg_MOCTR_RESTXEN0_Msk | \
		CANMSG_Msg_MOCTR_RESTXEN1_Msk | \
		CANMSG_Msg_MOCTR_RESDIR_Msk \
	)

void CAN0_IRQHandler(void);

/**
 * @brief can_init
 */
void can_init(const CO_CANbitRateData_t *CANbitRateData)
{
	// Clock CAN
	RCU->CGCFGAHB_bit.CANEN = 1;
	// Reset CAN
	RCU->RSTDISAHB_bit.CANEN = 0;
	__NOP();
	RCU->RSTDISAHB_bit.CANEN = 1;
	__NOP();
	// Enable CAN
	CAN->CLC_bit.DISR = 0;
	while ((CAN->CLC_bit.DISS) & (CAN->PANCTR_bit.PANCMD))
	{
	}
	// Normal divider mode
	CAN->FDR = (0x1 << CAN_FDR_DM_Pos) | (0x3FF << CAN_FDR_STEP_Pos);

	_CAN_Node_TypeDef *node = &CAN->Node[0];

	RCU->CGCFGAHB_bit.GPIOBEN = 1;
	RCU->RSTDISAHB_bit.GPIOBEN = 1;

	GPIOB->ALTFUNCNUM_bit.PIN8 = 1;
	GPIOB->ALTFUNCNUM_bit.PIN9 = 1;
	GPIOB->ALTFUNCSET = 0x0300;

	node->NCR =
	CAN_Node_NCR_CCE_Msk |
	CAN_Node_NCR_INIT_Msk;
	node->NBTR = (CANbitRateData->TSeg1 << CAN_Node_NBTR_TSEG1_Pos)
			| (CANbitRateData->TSeg2 << CAN_Node_NBTR_TSEG2_Pos)
			| (CANbitRateData->SJW << CAN_Node_NBTR_SJW_Pos)
			| (CANbitRateData->BRP << CAN_Node_NBTR_BRP_Pos);

	node->NPCR_bit.LBM = 0; // Loopback disable

	// CAN0 is connected with the bus, node's interrupts are enable
	node->NCR = CAN_Node_NCR_TRIE_Msk;

	// choosing number lines for node's interrupts
	node->NIPR = (1 << CAN_Node_NIPR_TRINP_Pos);

	while (CAN->PANCTR_bit.BUSY)
	{
	}

	CAN_Object_Location(CAN0_LOCATION_FIRST, CAN0_LOCATION_LAST, CAN0_LIST);

	_CANMSG_Msg_TypeDef *m;

	for (uint32_t x = CAN0_TX_FIRST; x <= CAN0_TX_LAST; x++)
	{
		m = &CANMSG->Msg[x];
		m->MOFCR = (8 << CANMSG_Msg_MOFCR_DLC_Pos) |
		CANMSG_Msg_MOFCR_STT_Msk;
		m->MOCTR = CANMSG_Msg_MOCTR_RESALL;
		m->MOCTR =
		CANMSG_Msg_MOCTR_SETTXEN0_Msk |
		CANMSG_Msg_MOCTR_SETTXEN1_Msk |
		CANMSG_Msg_MOCTR_SETDIR_Msk;
	}

	for (uint32_t x = CAN0_RX_FIRST; x <= CAN0_RX_LAST; x++)
	{
		m = &CANMSG->Msg[x];
		m->MOAMR = CANMSG_Msg_MOAMR_MIDE_Msk;
		m->MOAR = 0;
		m->MOFCR = (0x8 << CANMSG_Msg_MOFCR_DLC_Pos) |
		CANMSG_Msg_MOFCR_RXIE_Msk |
		CANMSG_Msg_MOFCR_SDT_Msk;
		m->MOIPR = (0x00 << CANMSG_Msg_MOIPR_RXINP_Pos);
		m->MOCTR = CANMSG_Msg_MOCTR_RESALL;
		m->MOCTR =
		CANMSG_Msg_MOCTR_SETMSGVAL_Msk |
		CANMSG_Msg_MOCTR_SETRXEN_Msk;
	}

	// Setup interrupt for CAN Line0
	PLIC_SetIrqHandler(Plic_Mach_Target, IsrVect_IRQ_CAN0, CAN0_IRQHandler);
	PLIC_SetPriority(IsrVect_IRQ_CAN0, 1);
	PLIC_IntEnable(Plic_Mach_Target, IsrVect_IRQ_CAN0);
}

_CANMSG_Msg_TypeDef* can_get_free_tx(void)
{
	static uint16_t tx_index = CAN0_TX_FIRST;

	_CANMSG_Msg_TypeDef *m;
	uint16_t x = tx_index;
	do
	{
		m = &CANMSG->Msg[x];
		if (++x > CAN0_TX_LAST)
			x = CAN0_TX_FIRST;
		if (m->MOSTAT_bit.TXRQ == 0)
		{
			tx_index = x;
			return m;
		}
	} while (x != tx_index);
	return NULL;
}

/******************************************************************************/

#define MOAR_STD_Pos	18

CO_ReturnError_t CO_CANsend(CO_CANmodule_t *CANmodule, CO_CANtx_t *buffer)
{
	CO_ReturnError_t err = CO_ERROR_NO;

	/* Verify overflow */
	if (buffer->bufferFull)
	{
		if (!CANmodule->firstCANtxMessage)
		{
			/* don't set error, if bootup message is still on buffers */
			CANmodule->CANerrorStatus |= CO_CAN_ERRTX_OVERFLOW;
		}
		err = CO_ERROR_TX_OVERFLOW;
	}

	CO_LOCK_CAN_SEND(CANmodule);

	/* if CAN TX buffer is free, copy message to it */
	if (CANmodule->CANtxCount == 0)
	{
		_CANMSG_Msg_TypeDef *m = can_get_free_tx();
		if (m != NULL)
		{
			CANmodule->bufferInhibitFlag = buffer->syncFlag;

			m->MOAR = (0x2 << CANMSG_Msg_MOAR_PRI_Pos)
					| (buffer->ident << MOAR_STD_Pos);

			memcpy((void*) &m->MODATAL, &buffer->data[0], 4);
			memcpy((void*) &m->MODATAH, &buffer->data[3], 4);
			m->MOFCR_bit.DLC = buffer->dlc;
			m->MOCTR = CANMSG_Msg_MOCTR_SETTXRQ_Msk
					| CANMSG_Msg_MOCTR_SETMSGVAL_Msk;
		}
		else
		{
			buffer->bufferFull = true;
			CANmodule->CANtxCount++;
		}
	}
	/* if no buffer is free, message will be sent by interrupt */
	else
	{
		buffer->bufferFull = true;
		CANmodule->CANtxCount++;
	}

	CO_UNLOCK_CAN_SEND(CANmodule);

	return err;
}

/******************************************************************************/
CO_CANtx_t* CO_CANtxBufferInit(CO_CANmodule_t *CANmodule, uint16_t index,
		uint16_t ident, bool_t rtr, uint8_t noOfBytes, bool_t syncFlag)
{
	CO_CANtx_t *buffer = NULL;
	if (rtr)
	{
		__BKPT();
	}
	else if ((CANmodule != NULL) && (index < CANmodule->txSize))
	{
		/* get specific buffer */
		buffer = &CANmodule->txArray[index];

		/* CAN identifier, DLC and rtr, bit aligned with CAN module transmit buffer */
		buffer->ident = ident & 0x07FF;
		buffer->dlc = noOfBytes & 0xF;

		buffer->bufferFull = false;
		buffer->syncFlag = syncFlag;
	}

	return buffer;
}

/******************************************************************************/
CO_ReturnError_t CO_CANrxBufferInit(CO_CANmodule_t *CANmodule, uint16_t index,
		uint16_t ident, uint16_t mask, bool_t rtr, void *object,
		void (*CANrx_callback)(void *object, void *message))
{
	CO_ReturnError_t ret = CO_ERROR_NO;

	if ((CANmodule != NULL) && (object != NULL) && (CANrx_callback != NULL)
			&& (index < CANmodule->rxSize))
	{
		/* buffer, which will be configured */
		CO_CANrx_t *buffer = &CANmodule->rxArray[index];

		/* Configure object variables */
		buffer->object = object;
		buffer->CANrx_callback = CANrx_callback;

		/* CAN identifier and CAN mask, bit aligned with CAN module FIFO buffers (RTR is extra) */
		buffer->ident = ident & 0x07FFU;
		if (rtr)
		{
			__BKPT();
			return ret;
		}
		buffer->mask = (mask & 0x07FFU) | 0x0800U;

		/* Set CAN hardware module filter and mask. */
		if (CANmodule->useCANrxFilters)
		{
			__BKPT();
			ret = CO_ERROR_ILLEGAL_ARGUMENT;
		}
	}
	else
	{
		ret = CO_ERROR_ILLEGAL_ARGUMENT;
	}

	return ret;
}

/******************************************************************************/
void CO_CANsetConfigurationMode(void *CANptr)
{

}

/******************************************************************************/
void CO_CANsetNormalMode(CO_CANmodule_t *CANmodule)
{
	CANmodule->CANnormal = true;
}

/******************************************************************************/
void CO_CANmodule_disable(CO_CANmodule_t *CANmodule)
{
	if (CANmodule != NULL)
	{
		CO_CANsetConfigurationMode(CANmodule->CANptr);
	}
}

/******************************************************************************/
CO_ReturnError_t CO_CANmodule_init(CO_CANmodule_t *CANmodule, void *CANptr,
		CO_CANrx_t rxArray[], uint16_t rxSize, CO_CANtx_t txArray[],
		uint16_t txSize, uint16_t CANbitRate)
{
	uint16_t i;
	const CO_CANbitRateData_t *CANbitRateData = NULL;

	/* verify arguments */
	if (CANmodule == NULL || rxArray == NULL || txArray == NULL)
	{
		return CO_ERROR_ILLEGAL_ARGUMENT;
	}

	/* Configure object variables */
	CANmodule->CANptr = CANptr;
	CANmodule->rxArray = rxArray;
	CANmodule->rxSize = rxSize;
	CANmodule->txArray = txArray;
	CANmodule->txSize = txSize;
	CANmodule->CANerrorStatus = 0;
	CANmodule->CANnormal = false;
	CANmodule->useCANrxFilters = false;
	CANmodule->bufferInhibitFlag = false;
	CANmodule->firstCANtxMessage = true;
	CANmodule->CANtxCount = 0U;
	CANmodule->errOld = 0U;
	CANmodule->interruptStatus = 0;
	CANmodule->interruptDisabler = 0;

	for (i = 0U; i < rxSize; i++)
	{
		rxArray[i].ident = 0U;
		rxArray[i].dlc = 0U;
		rxArray[i].mask = 0xFFFFU;
		rxArray[i].object = NULL;
		rxArray[i].CANrx_callback = NULL;
	}
	for (i = 0U; i < txSize; i++)
	{
		txArray[i].bufferFull = false;
	}

	/* Configure CAN timing */
	for (i = 0; i < (sizeof(CO_CANbitRateData) / sizeof(CO_CANbitRateData[0])); i++)
		if (CO_CANbitRateData[i].bitrate == CANbitRate)
		{
			CANbitRateData = &CO_CANbitRateData[i];
			break;
		}

	if (CANbitRate == 0 || CANbitRateData == NULL)
		return CO_ERROR_ILLEGAL_BAUDRATE;

	can_init(CANbitRateData);

	return CO_ERROR_NO;
}

/******************************************************************************/
void CO_CANmodule_process(CO_CANmodule_t *CANmodule)
{
	uint16_t rxErrors, txErrors, overflow = 0;
	uint32_t err;

	rxErrors = CAN->Node[0].NECNT_bit.REC;
	txErrors = CAN->Node[0].NECNT_bit.TEC;
	if ((CAN->Node[0].NSR & (
	CAN_Node_NSR_EWRN_Msk |
	CAN_Node_NSR_BOFF_Msk |
	CAN_Node_NSR_LLE_Msk |
	CAN_Node_NSR_LOE_Msk)) != 0)
		txErrors = 256; // bus off
	// overflow = (CAN_REG(CANmodule->CANptr, C_FIFOINT) & 0x8) ? 1 : 0;
	err = ((uint32_t) txErrors << 16) | ((uint32_t) rxErrors << 8) | overflow;

	if (CANmodule->errOld != err)
	{
		uint16_t status = CANmodule->CANerrorStatus;
		CANmodule->errOld = err;

		if (txErrors >= 256U)
		{
			/* bus off */
			status |= CO_CAN_ERRTX_BUS_OFF;
		}
		else
		{
			/* recalculate CANerrorStatus, first clear some flags */
			status &= 0xFFFF ^ (CO_CAN_ERRTX_BUS_OFF |
			CO_CAN_ERRRX_WARNING | CO_CAN_ERRRX_PASSIVE |
			CO_CAN_ERRTX_WARNING | CO_CAN_ERRTX_PASSIVE);

			/* rx bus warning or passive */
			if (rxErrors >= 128)
			{
				status |= CO_CAN_ERRRX_WARNING | CO_CAN_ERRRX_PASSIVE;
			}
			else if (rxErrors >= 96)
			{
				status |= CO_CAN_ERRRX_WARNING;
			}

			/* tx bus warning or passive */
			if (txErrors >= 128)
			{
				status |= CO_CAN_ERRTX_WARNING | CO_CAN_ERRTX_PASSIVE;
			}
			else if (rxErrors >= 96)
			{
				status |= CO_CAN_ERRTX_WARNING;
			}

			/* if not tx passive clear also overflow */
			if ((status & CO_CAN_ERRTX_PASSIVE) == 0)
			{
				status &= 0xFFFF ^ CO_CAN_ERRTX_OVERFLOW;
			}
		}

		if (overflow != 0)
		{
			/* CAN RX bus overflow */
			status |= CO_CAN_ERRRX_OVERFLOW;
		}

		CANmodule->CANerrorStatus = status;
	}
}

/******************************************************************************/
void CO_CANclearPendingSyncPDOs(CO_CANmodule_t *CANmodule)
{
	uint32_t tpdoDeleted = 0U;

	CO_LOCK_CAN_SEND(CANmodule);
	/* Abort message from CAN module, if there is synchronous TPDO.
	 * Take special care with this functionality. */
	if (CANmodule->bufferInhibitFlag)
	{
		CANmodule->bufferInhibitFlag = false;
		tpdoDeleted = 1U;
	}
	/* delete also pending synchronous TPDOs in TX buffers */
	if (CANmodule->CANtxCount != 0U)
	{
		uint16_t i;
		CO_CANtx_t *buffer = &CANmodule->txArray[0];
		for (i = CANmodule->txSize; i > 0U; i--)
		{
			if (buffer->bufferFull)
			{
				if (buffer->syncFlag)
				{
					buffer->bufferFull = false;
					CANmodule->CANtxCount--;
					tpdoDeleted = 2U;
				}
			}
			buffer++;
		}
	}
	CO_UNLOCK_CAN_SEND(CANmodule);

	if (tpdoDeleted != 0U)
	{
		CANmodule->CANerrorStatus |= CO_CAN_ERRTX_PDO_LATE;
	}
}

/******************************************************************************/
void CO_CANinterrupt(CO_CANmodule_t *CANmodule)
{
	/* receive interrupt (New CAN message is available in RX FIFO 0 buffer) */
	CO_CANrxMsg_t rcvMsg; /* pointer to received message in CAN module */
	uint16_t index; /* index of received message */
	uint16_t rcvMsgIdent; /* identifier of the received message */
	CO_CANrx_t *buffer = NULL; /* receive message buffer from CO_CANmodule_t object. */
	bool_t msgMatched = false;

	_CANMSG_Msg_TypeDef *m = &CANMSG->Msg[CAN0_RX_FIRST];
	for (uint32_t x = CAN0_RX_FIRST; x <= CAN0_RX_LAST; x++, m++)
	{
		if (m->MOSTAT_bit.MSGVAL != 0)
			continue;

		rcvMsgIdent = (m->MOAR >> MOAR_STD_Pos) & 0x7FF;
		if (CANmodule->useCANrxFilters)
		{
			__BKPT();
		}
		else
		{
			/* CAN module filters are not used, message with any standard 11-bit identifier */
			/* has been received. Search rxArray form CANmodule for the same CAN-ID. */
			buffer = &CANmodule->rxArray[0];
			for (index = CANmodule->rxSize; index > 0U; index--)
			{
				if (((rcvMsgIdent ^ buffer->ident) & buffer->mask) == 0U)
				{
					msgMatched = true;
					break;
				}
				buffer++;
			}
		}

		/* Call specific function, which will process the message */
		if (msgMatched && (buffer != NULL) && (buffer->CANrx_callback != NULL))
		{
			rcvMsg.ident = rcvMsgIdent;
			rcvMsg.dlc = m->MOFCR_bit.DLC;
			memcpy(&rcvMsg.data[0], (void *)&m->MODATAL, 4);
			memcpy(&rcvMsg.data[4], (void *)&m->MODATAH, 4);
			buffer->CANrx_callback(buffer->object, (void *)&rcvMsg);
		}

		m->MOCTR = CANMSG_Msg_MOCTR_SETMSGVAL_Msk;
	}
}
