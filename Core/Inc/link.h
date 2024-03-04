#ifndef __LINK_H__
#define __LINK_H__

#include <stdint.h>
#include <stdbool.h>
#include "podtp.h"
#include "config.h"


#define LINK_LED_ON()   HAL_GPIO_WritePin(LINK_LED_PORT, LINK_LED_PIN, GPIO_PIN_SET)
#define LINK_LED_OFF()  HAL_GPIO_WritePin(LINK_LED_PORT, LINK_LED_PIN, GPIO_PIN_RESET)

#define LINK_BUFFER_MAX_LEN (2 * (PODTP_MAX_DATA_LEN + 1) + 1)
#define LINK_BUFFER_INC_INDEX(index) ((index + 1) % LINK_BUFFER_MAX_LEN)

typedef struct {
    uint8_t data[LINK_BUFFER_MAX_LEN];
    uint16_t head;
    uint16_t tail;
} LinkBuffer;

void linkBufferPutChar(uint8_t c);
uint8_t linkBufferGetChar();
bool linkBufferIsEmpty();

bool linkGetPacket(PodtpPacket *packet);
void linkSendPacket(PodtpPacket *packet);
bool linkProcessPacket(PodtpPacket *packet);

#endif // __LINK_H__