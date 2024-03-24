#include <string.h>
#include "boot.h"
#include "gpio.h"
#include "config.h"

#define DEBUG
#include "debug.h"

void bootPinInit() {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    // Change this if the boot pin is not GPIOA
	__HAL_RCC_GPIOA_CLK_ENABLE();
	GPIO_InitStruct.Pin = BOOT_CTRL_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(BOOT_CTRL_PORT, &GPIO_InitStruct);
}

bool bootPinDeInit() {
    HAL_GPIO_DeInit(BOOT_CTRL_PORT, BOOT_CTRL_PIN);
    // Change this if the boot pin is not GPIOA
    __HAL_RCC_GPIOA_CLK_DISABLE();
    return true;
}

bool startFirmware() {
    // If the boot pin is low, start the firmware
    return HAL_GPIO_ReadPin(BOOT_CTRL_PORT, BOOT_CTRL_PIN) == GPIO_PIN_RESET;
}

// Update this if the target is not H7
static const uint32_t sector_offset[] = {
    [0]  = 0x00000000,
    [1]  = 0x00020000,
    [2]  = 0x00040000,
    [3]  = 0x00060000,
    [4]  = 0x00080000,
    [5]  = 0x000A0000,
    [6]  = 0x000C0000,
    [7]  = 0x000E0000,
};

uint8_t boot_buffer[LOCAL_BUFFER_PAGE * FIRMWARE_PAGE_SIZE];

typedef struct {
    uint16_t bufferPage;
    uint16_t flashPage;
    uint16_t numPages;
} __attribute__((__packed__)) WriteFlash;

typedef struct {
    uint16_t bufferPage;
    uint16_t offset;
} __attribute__((__packed__)) LoadBuffer;

int count = 0;
bool bootLoad(PodtpPacket *packet) {
    uint8_t len = packet->length - 1;
    packet->length = 1;
    LoadBuffer *lb = (LoadBuffer *)packet->data;
    uint8_t data_size = len - sizeof(LoadBuffer);
    if (lb->bufferPage >= LOCAL_BUFFER_PAGE || lb->offset >= FIRMWARE_PAGE_SIZE
        || lb->bufferPage * FIRMWARE_PAGE_SIZE + lb->offset + data_size
            > LOCAL_BUFFER_PAGE * FIRMWARE_PAGE_SIZE) {
        packet->type = PODTP_TYPE_ACK;
        packet->port = PODTP_PORT_ERROR;
        return true;
    }
    
    memcpy(boot_buffer + (lb->bufferPage * FIRMWARE_PAGE_SIZE) + lb->offset, packet->data + sizeof(LoadBuffer), data_size);
    count++;
    packet->type = PODTP_TYPE_ACK;
    return true;
}

bool bootWrite(PodtpPacket *packet) {
    WriteFlash *wf = (WriteFlash *)packet->data;

    packet->length = 1;
    if (wf->bufferPage >= LOCAL_BUFFER_PAGE || wf->flashPage < FIRMWARE_START_PAGE
        || wf->flashPage + wf->numPages > FIRMWARE_END_PAGE) {
        packet->type = PODTP_TYPE_ACK;
        packet->port = PODTP_PORT_ERROR;
        return true;
    }

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR |FLASH_FLAG_WRPERR |
            FLASH_FLAG_PGSERR | FLASH_FLAG_ALL_BANK1);
    __disable_irq();

    // Erase the page(s) by sector
    bool error = false;
    // this would take a long time, around 1 second for each sector
    for (uint16_t i = 0; i < wf->numPages; i++) {
        for (int j = 0; j < 12; j++) {
            if (((uint32_t)(wf->flashPage + i)) * FIRMWARE_PAGE_SIZE == sector_offset[j]) {
                FLASH_EraseInitTypeDef EraseInitStruct;
                EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
                EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
                EraseInitStruct.NbSectors = 1;
                EraseInitStruct.Sector = j;
                EraseInitStruct.Banks = FLASH_BANK_1; // for H7, this is essential
                uint32_t SectorError = 0;
                // SPL: 
                // FLASH_EraseSector(j << 3, VoltageRange_3);
                // HAL:
                if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK) {
                    error = true;
                    break;
                }
            }
        }
        if (error) {
            break;
        }
    }

    if (!error) {
        // Write the data, long per long
        uint32_t flashAddress = FLASH_BASE + (wf->flashPage * FIRMWARE_PAGE_SIZE);
        uint8_t *buffer = boot_buffer + (wf->bufferPage * FIRMWARE_PAGE_SIZE);

        int addr_inc = 256 / 8; // HAL_FLASH_Program for H7 writes 256 bits at a time
        for (int i = 0; i < wf->numPages * FIRMWARE_PAGE_SIZE; i += addr_inc, flashAddress += addr_inc) {
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, flashAddress, (uint32_t) (buffer + i)) != HAL_OK) {
                error = true;
                DEBUG_PRINT("ERROR 0x%lx\n", HAL_FLASH_GetError());
                break;
            }
        }
    }
    
    HAL_FLASH_Lock();
    __enable_irq();

    // enable UART5 RXNE interrupt, otherwise the bootloader will not receive the next packet
    __HAL_UART_ENABLE_IT(&huart5, UART_IT_RXNE);

    packet->type = PODTP_TYPE_ACK;
    packet->port = error ? PODTP_PORT_ERROR : PODTP_PORT_OK;
    return true;
}