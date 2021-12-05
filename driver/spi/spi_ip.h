// Author: Sarker Nadir Afridi Azmi

// Includes, Defines

#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "../address_map.h"
#include "spi_regs.h"

#define CS0_AUTO			0x20
#define CS1_AUTO			0x40
#define CS2_AUTO			0x80
#define CS3_AUTO			0x100
#define CS_MANUAL			0x00
#define WORD_SIZE_8BITS		0x07
#define WORD_SIZE_16BITS	0x0F
#define WORD_SIZE_24BITS	0x17
#define WORD_SIZE_32BITS	0x1F

#define CS_SELECT_OFFSET	13
#define SPI_MODE_MASK		0x03
#define SPI_MODE_OFFSET		0x10

bool openSpi();
uint32_t spiReadRegister(uint8_t regOffset);
void spiWriteRegister(uint8_t regOffset, uint32_t data);
void spiWriteData(uint32_t data);
uint32_t spiReadData();
void enableCS(uint8_t n);
void disableCS(uint8_t n);
void csSelect(uint32_t n);
void enableSpi();
void disableSpi();
void spiSetMode(uint8_t n, uint32_t spo_sph);
void spiSetBaudRate(uint32_t baudRate);
