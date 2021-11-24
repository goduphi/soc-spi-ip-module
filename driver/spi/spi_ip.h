// Author: Sarker Nadir Afridi Azmi

// Includes, Defines

#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "../address_map.h"
#include "spi_regs.h"

bool initSpi();
uint32_t spiReadRegister(uint8_t regOffset);
void spiWriteRegister(uint8_t regOffset, uint32_t data);
void spiWriteData(uint32_t data);
uint32_t spiReadData();
void enableCS();
void disableCS();
void enableSpi();
void disableSpi();
