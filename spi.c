#include "K1921VG015.h"
#include "system_k1921vg015.h"

#include "CO_driver_target.h"

void SPI0_IRQHandler()
{
	__BKPT();
	SPI0->ICR = (SPI_ICR_RORIC_Msk | SPI_ICR_RTIC_Msk);
}

void spi0_init(void)
{
	RCU->CGCFGAHB_bit.GPIOBEN	= 1;
	RCU->RSTDISAHB_bit.GPIOBEN	= 1;
	RCU->CGCFGAHB_bit.SPI0EN	= 1;
	RCU->RSTDISAHB_bit.SPI0EN	= 0;
	__NOP();
	RCU->RSTDISAHB_bit.SPI0EN	= 1;

	RCU->SPICLKCFG[0].SPICLKCFG_bit.CLKSEL	= RCU_SPICLKCFG_CLKSEL_HSE;	// Select clock for SPI0
	RCU->SPICLKCFG[0].SPICLKCFG_bit.CLKEN	= 1;	// Enable clock SPI0
	RCU->SPICLKCFG[0].SPICLKCFG_bit.RSTDIS	= 1;	// Enable SPI0

	SPI0->CPSR_bit.CPSDVSR	= 8;	// First divider

	SPI0->CR0 = (
			(1 << SPI_CR0_SCR_Pos) |
			// SPI_CR0_SPO_Msk |
			// SPI_CR0_SPH_Msk |
			(SPI_CR0_FRF_SPI  << SPI_CR0_FRF_Pos) |	/* SPI of Motorola */
			(SPI_CR0_DSS_8bit << SPI_CR0_DSS_Pos)
			);
	SPI0->CR1_bit.MS	= 0;		// Master mode

	// PB0 CLK
	// PB1 CS
	// PB2 MISO
	// PB3 MOSI
	GPIOB->ALTFUNCNUM_bit.PIN0 = GPIO_ALTFUNCNUM_PIN0_AF1;
//	GPIOB->ALTFUNCNUM_bit.PIN1 = GPIO_ALTFUNCNUM_PIN1_AF1;
	GPIOA->DATAOUTSET		= GPIO_DATAOUTSET_PIN1_Msk;
	GPIOB->OUTMODE_bit.PIN1	= GPIO_OUTMODE_PIN1_PP;
	GPIOB->OUTENSET			= GPIO_OUTENSET_PIN1_Msk;
//
	GPIOB->ALTFUNCNUM_bit.PIN2 = GPIO_ALTFUNCNUM_PIN2_AF1;
	GPIOB->ALTFUNCNUM_bit.PIN3 = GPIO_ALTFUNCNUM_PIN3_AF1;
	GPIOB->ALTFUNCSET = GPIO_ALTFUNCSET_PIN0_Msk |
//						GPIO_ALTFUNCSET_PIN1_Msk |
						GPIO_ALTFUNCSET_PIN2_Msk |
						GPIO_ALTFUNCSET_PIN3_Msk;
	/*
	SPI0->IMSC = SPI_IMSC_RORIM_Msk;// Enable interrupt on receive overflow

	PLIC_SetIrqHandler (Plic_Mach_Target, IsrVect_IRQ_SPI0, SPI0_IRQHandler);
	PLIC_SetPriority   (IsrVect_IRQ_SPI0, 0x3);
	PLIC_IntEnable     (Plic_Mach_Target, IsrVect_IRQ_SPI0);
	 */
	SPI0->CR1_bit.SSE = 1;	// Enable SPI0 transceiver
}

int spi_send_receive(uint8_t *tx_buffer, uint16_t tx_len, uint8_t *rx_buffer, uint16_t rx_len)
{
	return 0;
}

