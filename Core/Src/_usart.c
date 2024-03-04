#include "_usart.h"
#include "config.h"

void esp32UartPutChar(uint8_t c) {
  HAL_UART_Transmit(&COMMUNICATION_UART, &c, 1, 100);
  while (__HAL_UART_GET_FLAG(&huart5, UART_FLAG_TC) == 0);
}

void debugUartPutChar(int c) {
  HAL_UART_Transmit(&DEBUG_UART, (uint8_t *) &c, 1, 100);
}