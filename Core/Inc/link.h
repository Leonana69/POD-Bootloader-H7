#ifndef __LINK_H__
#define __LINK_H__

#include <stdint.h>
#include <stdbool.h>
#include "podtp.h"

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
bool linkProcessPacket(PodtpPacket *packet);

#endif // __LINK_H__