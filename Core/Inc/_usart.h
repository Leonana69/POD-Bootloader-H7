#ifndef ___USART_H
#define ___USART_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include "usart.h"

void esp32UartPutChar(uint8_t c);
void debugUartPutChar(int c);

#ifdef __cplusplus
}
#endif

#endif //___USART_H__