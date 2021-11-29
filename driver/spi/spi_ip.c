// Author: Sarker Nadir Afridi Azmi

// Includes, Defines

#include "spi_ip.h"

// Global variables

uint32_t* base = NULL;

// Subroutines

bool openSpi()
{
	// Open /dev/mem
	int file = open("/dev/mem", O_RDWR | O_SYNC);
	bool ok = (file >= 0);
	if(ok)
	{
		// Create a map from the physical memory location of
		// /dev/mem at an offset to LW avalon interface
		// with an aperature of SPAN_IN_BYTES bytes
		// to any location in the virtual 32-bit memory space of the proce
		base = mmap(NULL, SPAN_IN_BYTES, PROT_READ | PROT_WRITE, MAP_SHARED, file, LW_BRIDGE_BASE + SPI_BASE_OFFSET);
		ok = (base != MAP_FAILED);
		
		// Close /dev/mem
		close(file);
	}
	return ok;
}

uint32_t spiReadRegister(uint8_t regOffset)
{
	return *(base + regOffset);
}

void spiWriteRegister(uint8_t regOffset, uint32_t data)
{
	*(base + regOffset) = data;
}

void spiWriteData(uint32_t data)
{
	*(base + OFS_DATA) = data;
}

uint32_t spiReadData()
{
	return *(base + OFS_DATA);
}

void enableCS(uint8_t n)
{
	if(n > 3)
		return;
	uint32_t cs = spiReadRegister(OFS_CONTROL);
	spiWriteRegister(OFS_CONTROL, cs | (0x200 << n));
}

void disableCS(uint8_t n)
{
	if(n > 3)
		return;
	uint32_t cs = spiReadRegister(OFS_CONTROL);
	spiWriteRegister(OFS_CONTROL, cs & ~(0x200 << n));
}

void enableSpi()
{
	*(base + OFS_CONTROL) |= 0x00008000;
}

void disableSpi()
{
	*(base + OFS_CONTROL) &= ~0x00008000;
} 

// The cycle is fixed to 50 MHz
void setBaudRateSpi(uint32_t baudRate)
{
	// Baud Rate = fcycle / divisor
	uint32_t divisor = (50000000 / 2) / baudRate;
	spiWriteRegister(OFS_BRD, divisor << 7);
}
