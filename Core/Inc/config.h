#ifndef __CONFIG_H
#define __CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define LINK_LED_PORT       GPIOC
#define LINK_LED_PIN        GPIO_PIN_0

#define BOOT_CTRL_PORT      GPIOA
#define BOOT_CTRL_PIN       GPIO_PIN_4

#define DEBUG_UART          huart4
#define COMMUNICATION_UART  huart5

#ifdef __cplusplus
}
#endif

#endif //__CONFIG_H__