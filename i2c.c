#include "K1921VG015.h"
#include "system_k1921vg015.h"

#include "CO_driver_target.h"

#define SMBUS_SYSTEM_FREQ		16000000
#define SMBUS_BLOCK_BYTES_MAX	32

#define FSFreq	100000
#define HSFreq	3400000

volatile uint32_t i2c_int_cnt;
uint8_t i2c_rx_data[8];
uint8_t i2c_rx_data_count;

void I2C_IRQHandler()
{
	uint8_t I2C0_Mode = I2C->ST_bit.MODE;

	switch (I2C0_Mode)
	{
		case I2C_ST_MODE_STDONE:
			if (i2c_int_cnt == 0)
			{
				I2C->SDA = 0x15 << 1;	// Send to slave address 0x15 { address, R/W = 0 }
				i2c_int_cnt++;
			}
			else
				I2C->SDA = (0x15 << 1) | 1;	// Receive from slave address 0x15 { address, R/W = 1 }
			break;
		case I2C_ST_MODE_MTADPA:
			I2C->SDA = 0xAA;
			break;
		case I2C_ST_MODE_MTDAPA:
			I2C->CTL0_bit.STOP = 1;
			break;
		case I2C_ST_MODE_MRADPA:
			break;
		case I2C_ST_MODE_MRDAPA:
			I2C->CTL0_bit.ACK = 1; // Not ACK from Master
			break;
		case I2C_ST_MODE_MRDANA:
			I2C->CTL0_bit.STOP = 1;
			break;
		default:
			break;
	}
	I2C->CTL0 = (I2C_CTL0_CLRST_Msk | I2C_CTL0_INTEN_Msk);		//������� ��� ����������, ��������� ����������
}

void i2c_init(void)
{
	uint32_t freq_calc;

	/* I2C: SCL - PC.12, SDA - PC.13 */
	RCU->CGCFGAHB_bit.GPIOCEN  = 1;
	RCU->RSTDISAHB_bit.GPIOCEN = 1;

	GPIOC->PULLMODE |= (GPIO_PULLMODE_PIN12_Msk | GPIO_PULLMODE_PIN13_Msk);
	GPIOC->OUTMODE_bit.PIN12 = GPIO_OUTMODE_PIN12_OD;
	GPIOC->OUTMODE_bit.PIN13 = GPIO_OUTMODE_PIN13_OD;
	GPIOC->ALTFUNCNUM_bit.PIN12 = GPIO_ALTFUNCNUM_PIN12_AF1;
	GPIOC->ALTFUNCNUM_bit.PIN13 = GPIO_ALTFUNCNUM_PIN13_AF1;
	GPIOC->ALTFUNCSET = GPIO_ALTFUNCSET_PIN12_Msk | GPIO_ALTFUNCSET_PIN13_Msk;

	RCU->CGCFGAPB_bit.I2CEN  = 1;
    RCU->RSTDISAPB_bit.I2CEN = 1;

    freq_calc = SMBUS_SYSTEM_FREQ / (4 * FSFreq);
    I2C->CTL1_bit.SCLFRQ = freq_calc & 0x7F;
    I2C->CTL3_bit.SCLFRQ = freq_calc >> 7;

    freq_calc = SMBUS_SYSTEM_FREQ / ( 3 * HSFreq );
    I2C->CTL2_bit.HSDIV = freq_calc & 0x0F;
    I2C->CTL4_bit.HSDIV = freq_calc >> 4;

    I2C->CTL1_bit.ENABLE = 1;
    I2C->CTL0_bit.INTEN  = 1;

    PLIC_SetIrqHandler (Plic_Mach_Target, IsrVect_IRQ_I2C, I2C_IRQHandler);
    PLIC_SetPriority   (IsrVect_IRQ_I2C, 0x1);
    PLIC_IntEnable     (Plic_Mach_Target, IsrVect_IRQ_I2C);
}
