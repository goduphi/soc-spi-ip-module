#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "spi_ip.h"

// mcp23s08 register map
#define MCP23S08_ADDRESS		0x40
#define DIR_REG					0x00
#define DATA_REG				0x09
#define GPPU_REG				0x06

#define ALL_OUTPUTS				0x00
#define ALL_INPUTS				0xFF

#define RED_LED_MASK			0x01
#define GREEN_LED_MASK			0x02

void initSpi(uint32_t control)
{
	// Set a baud rate of 5 MHz
	spiSetBaudRate(5e6);
	spiWriteRegister(OFS_CONTROL, control);

	if (!(control & CS0_AUTO))
		enableCS(0);
	if (!(control & CS1_AUTO))
		enableCS(1);
	if (!(control & CS2_AUTO))
		enableCS(2);
	if (!(control & CS3_AUTO))
		enableCS(3);

	enableSpi();
}

// This function makes use of auto CS
void writeRegisterMcp23s08(uint8_t address, uint8_t data)
{
	uint32_t tmp = MCP23S08_ADDRESS;
	tmp = (tmp << 8) | address;
	tmp = (tmp << 8) | data;
	spiWriteData(tmp);
	while (!(spiReadRegister(OFS_STATUS) & 0x20));
	spiReadData();
}

uint32_t readRegisterMcp23s08(uint8_t address)
{
	uint32_t tmp = MCP23S08_ADDRESS | 1;
	tmp = (tmp << 8) | address;
	tmp = (tmp << 8) | 0xFF;
	spiWriteData(tmp);
	while (!(spiReadRegister(OFS_STATUS) & 0x20));
	return spiReadData();
}

void writeRegisterMcp23s08CsMan(uint8_t address, uint8_t data, uint8_t cs)
{
	uint32_t tmp = MCP23S08_ADDRESS;
	tmp = (tmp << 8) | address;
	tmp = (tmp << 8) | data;
	disableCS(cs);
	spiWriteData(tmp);
	while (!(spiReadRegister(OFS_STATUS) & 0x20));
	spiReadData();
	enableCS(cs);
}

void help(const char* programName)
{
	printf("%s stop_go [mode]\n", programName);
}

int main(int argc, char* argv[])
{
	if (argc == 2 && strcmp(argv[1], "help") == 0)
	{
		help(argv[0]);
		return EXIT_FAILURE;
	}

	if (!openSpi())
	{
		printf("Error opening /dev/mem for SPI IP module @0x%8x\n", LW_BRIDGE_BASE, SPI_BASE_OFFSET);
		return EXIT_FAILURE;
	}
	else
		printf("Mapped physical memory 0x%8x with %d bytes of span!\n", LW_BRIDGE_BASE + SPI_BASE_OFFSET, SPAN_IN_BYTES);

	if (argc == 6 && strcmp(argv[1], "cs_auto") == 0)
	{
		initSpi(CS0_AUTO | CS1_AUTO | CS2_AUTO | CS3_AUTO | WORD_SIZE_24BITS);

		uint32_t cs = atoi(argv[4]);
		if (cs > 3)
			cs = 0;
		uint32_t mode = atoi(argv[5]);
		if (mode > 3)
			mode = 0;

		csSelect(cs);
		spiSetMode(cs, mode);

		writeRegisterMcp23s08(DIR_REG, ALL_OUTPUTS);

		if (strcmp(argv[2], "on") == 0)
		{
			printf("Turning on pin %s...\n", argv[3]);
			writeRegisterMcp23s08(DATA_REG, strtol(argv[3], NULL, 16) & 0xFF);
		}
		else if (strcmp(argv[2], "off") == 0)
		{
			printf("Turning off all pins...\n");
			writeRegisterMcp23s08(DATA_REG, 0x00);
		}
	}

	if (argc == 6 && strcmp(argv[1], "cs_man") == 0)
	{
		initSpi(WORD_SIZE_24BITS);

		uint32_t cs = atoi(argv[4]);
		if (cs > 3)
			cs = 0;
		uint32_t mode = atoi(argv[5]);
		if (mode > 3)
			mode = 0;

		csSelect(cs);
		spiSetMode(cs, mode);
		
		writeRegisterMcp23s08CsMan(DIR_REG, ALL_OUTPUTS, cs);

		if(strcmp(argv[2], "on") == 0)
		{
			writeRegisterMcp23s08CsMan(DATA_REG, strtol(argv[3], NULL, 16) & 0xFF, cs);
		}
		else if(strcmp(argv[2], "off") == 0)
		{
			writeRegisterMcp23s08CsMan(DATA_REG, 0x00, cs);
		}
	}

	if (argc >= 2 && strcmp(argv[1], "stop_go") == 0)
	{
		initSpi(CS0_AUTO | CS1_AUTO | CS2_AUTO | CS3_AUTO | WORD_SIZE_24BITS);

		uint32_t mode = (argc == 3) ? atoi(argv[2]) : 0;
		mode = (mode > 3) ? 0 : mode;
		printf("Running in mode %hhu, %hhu\n", (mode >> 1) & 1, mode & 1);

		csSelect(0);
		spiSetMode(0, mode);
		// Set pins 0 and 1 as outputs and the rest as inputs
		// 0 represents an output and 1 an input
		writeRegisterMcp23s08(GPPU_REG, 0xFF);
		writeRegisterMcp23s08(DIR_REG, 0xF8);
		writeRegisterMcp23s08(DATA_REG, 0x04);
		printf("Press dat button!!!\n");
		while ((readRegisterMcp23s08(DATA_REG)) & 0x08);
		writeRegisterMcp23s08(DATA_REG, 0x02);
	}

	disableSpi();
	return EXIT_SUCCESS;
}
