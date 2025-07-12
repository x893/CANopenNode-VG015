#ifndef SYSTEM_K1921VG015_H
#define SYSTEM_K1921VG015_H

#ifdef __cplusplus
extern "C" {
#endif

//-- Includes ------------------------------------------------------------------
#include <stdint.h>
#include "csr.h"
#include "arch.h"
#include "plic.h"

//-- Defines -------------------------------------------------------------------
#ifndef HSECLK_VAL
#define HSECLK_VAL 0
#endif

#define SYSCLK_SWITCH_TIMEOUT	200
#define HSICLK_VAL 1000000
#define LSICLK_VAL 32768

// machine irq handler
#define MCAUSE_INTERRUPT_FLAG           (1 << 31)
#define MCAUSE_EXCEPT_MASK              0xF
#define MCAUSE_EXCEPT_INSTRADDRMISALGN  0x0
#define MCAUSE_EXCEPT_INSTRACCSFAULT    0x1
#define MCAUSE_EXCEPT_INSTRILLEGAL      0x2
#define MCAUSE_EXCEPT_BREAKPNT          0x3
#define MCAUSE_EXCEPT_LOADADDRMISALGN   0x4
#define MCAUSE_EXCEPT_LOADACCSFAULT     0x5
#define MCAUSE_EXCEPT_STAMOADDRMISALGN  0x6
#define MCAUSE_EXCEPT_STAMOACCSFAULT    0x7
#define MCAUSE_EXCEPT_ECALLFRM_M_MODE   0xB

#define MIER_MEIE_BITMASK       (1 << 11)
#define MSTATUSR_MIE_BITMASK    (1 << 3)

extern uint32_t SystemCoreClock; // System Clock Frequency (Core Clock)
extern uint32_t SystemPll0Clock; // System PLL0Clock Frequency
extern uint32_t SystemPll1Clock; // System PLL1Clock Frequency
extern uint32_t USBClock; 		 // USB Clock Frequency (USB PLL Clock)

void InterruptEnable(void);
void InterruptDisable(void);
void SystemInit(void);
void SystemCoreClockUpdate(void);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_K1921VG015_H
