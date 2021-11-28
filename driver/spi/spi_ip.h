// Author: Sarker Nadir Afridi Azmi

// Includes, Defines

#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "../address_map.h"
#include "spi_regs.h"

#define CS_AUTO				0x20
#define CS_MANUAL			0x00
#define WORD_SIZE_8BITS		0x07
#define WORD_SIZE_16BITS	0x0F
#define WORD_SIZE_24BITS	0x17
#define WORD_SIZE_32BITS	0x1F

bool openSpi();
uint32_t spiReadRegister(uint8_t regOffset);
void spiWriteRegister(uint8_t regOffset, uint32_t data);
void spiWriteData(uint32_t data);
uint32_t spiReadData();
void enableCS();
void disableCS();
void enableSpi();
void disableSpi();
