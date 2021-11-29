#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "spi_ip.h"

#define MCP23S08_ADDRESS		0x40

void initSpi(uint32_t control)
{
	// Set a baud rate of 5 MHz
	spiWriteRegister(OFS_BRD, 640);
	spiWriteRegister(OFS_CONTROL, control);
	if (!(CS_AUTO & control))
	{
		enableCS(0);
		enableCS(1);
		enableCS(2);
		enableCS(3);
	}
	enableSpi();
}

void writeRegisterMcp23s08(uint8_t address, uint8_t data)
{
	uint32_t tmp = MCP23S08_ADDRESS;
	tmp = (tmp << 8) | address;
	tmp = (tmp << 8) | data;
	spiWriteData(tmp);
	while (!(spiReadRegister(OFS_STATUS) & 0x20));
}

void writeRegisterMcp23s08CsMan(uint8_t address, uint8_t data)
{
	uint32_t tmp = MCP23S08_ADDRESS;
	tmp = (tmp << 8) | address;
	tmp = (tmp << 8) | data;
	disableCS(0);
	spiWriteData(tmp);
	while (!(spiReadRegister(OFS_STATUS) & 0x20));
	enableCS(0);
}

int main(int argc, char* argv[])
{
	if (!openSpi())
	{
		printf("Error opening /dev/mem for SPI IP module @0x%8x\n", LW_BRIDGE_BASE, SPI_BASE_OFFSET);
		return EXIT_FAILURE;
	}
	else
		printf("Mapped physical memory 0x%8x with %d bytes of span!\n", LW_BRIDGE_BASE + SPI_BASE_OFFSET, SPAN_IN_BYTES);

	if (argc >= 3 && strcmp(argv[1], "cs_auto") == 0)
	{
		initSpi(CS_AUTO | WORD_SIZE_24BITS);

		writeRegisterMcp23s08(0x00, 0x00);

		if (argc == 4 && strcmp(argv[2], "on") == 0)
		{
			printf("Turning LED on...\n");
			writeRegisterMcp23s08(0x09, strtol(argv[3], NULL, 16) & 0xFF);
		}
		else if (strcmp(argv[2], "off") == 0)
		{
			printf("Turning LED off...\n");
			writeRegisterMcp23s08(0x09, 0x00);
		}
	}

	if (argc >= 3 && strcmp(argv[1], "cs_man") == 0)
	{
		initSpi(WORD_SIZE_24BITS);
		writeRegisterMcp23s08CsMan(0x00, 0x00);
		
		if(strcmp(argv[2], "on") == 0)
		{
			writeRegisterMcp23s08CsMan(0x09, strtol(argv[3], NULL, 16) & 0xFF);
		}
		else if(strcmp(argv[2], "off") == 0)
		{
			writeRegisterMcp23s08CsMan(0x09, 0x00);
		}
	}

	disableSpi();
	return EXIT_SUCCESS;
}
