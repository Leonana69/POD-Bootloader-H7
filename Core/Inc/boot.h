#ifndef __BOOT_H__
#define __BOOT_H__

#include <stdbool.h>
#include "podtp.h"

#define FIRMWARE_START_PAGE 16
#define FIRMWARE_END_PAGE 1024
#define FIRMWARE_PAGE_SIZE 1024
#define FIRMWARE_START_ADDRESS 0x08004000

#define LOCAL_BUFFER_PAGE 10

#define PORT_LOAD_BUFFER 0x01
#define PORT_WRITE_FLASH 0x02

void bootPinInit();
bool bootPinDeInit();
bool startFirmware();

bool bootLoad(PodtpPacket *packet);
bool bootWrite(PodtpPacket *packet);

#endif // __BOOT_H__