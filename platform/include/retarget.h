#ifndef BSP_INCLUDE_RETARGET_H_
#define BSP_INCLUDE_RETARGET_H_

#include "K1921VG015.h"

#define RETARGET_UART UART0
#define RETARGET_UART_NUM 0
#define RETARGET_UART_PORT GPIOA
#define RETARGET_UART_PIN_TX_POS 1
#define RETARGET_UART_PIN_RX_POS 0
#define RETARGET_UART_RX_IRQHandler UART0_IRQHandler
#define RETARGET_UART_RX_IRQn UART0_IRQn

#define RETARGET_UART_BAUD 115200

//-- Functions -----------------------------------------------------------------
void retarget_init(void);
int retarget_get_char(void);
int retarget_put_char(int ch);

#endif /* BSP_INCLUDE_RETARGET_H_ */
