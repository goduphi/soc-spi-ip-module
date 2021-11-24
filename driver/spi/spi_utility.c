// Author: Sarker Nadir Afridi Azmi

// Includes, Defines

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "spi_ip.h"

int main(int argc, char* argv[])
{
	if(argc < 3)
	{
		printf("Usage: %s <r/w> <register offset> <data>\n", argv[0]);
		return EXIT_FAILURE;
	}

	bool ok = true;
	ok = (argv[1][0] == 'r' || argv[1][0] == 'w');
	uint8_t offset = atoi(argv[2]) & 0xFF;
	ok &= offset >= OFS_DATA && offset <= OFS_BRD;
	if(!ok)
	{
		printf("Usage %s <r/w> <register offset> <data>\n", argv[0]);
		return EXIT_FAILURE;
	} 

	if(!initSpi())
	{
		printf("Error opening /dev/mem for SPI IP module @0x%8x\n", LW_BRIDGE_BASE, SPI_BASE_OFFSET);
		return EXIT_FAILURE;
	}
	else
		printf("Mapped physical memory 0x%8x with %d bytes of span!\n", LW_BRIDGE_BASE + SPI_BASE_OFFSET, SPAN_IN_BYTES);	

	if(argv[1][0] == 'r')
		if(offset == OFS_DATA)
			printf("Value at offset %hhu = 0x%x\n", offset, spiReadData());
		else
			printf("Value at offset %hhu = 0x%x\n", offset, spiReadRegister(offset));
	if(argv[1][0] == 'w')
	{
		if(argv[2][0] == 's')
		{
			/*uint32_t data = strtol(argv[3], NULL, 16);
			uint32_t wordSize = (argc == 5) ? strtol(argv[4], NULL, 10) : 7;*/
			spiWriteRegister(OFS_BRD, 640);
			spiWriteRegister(OFS_CONTROL, 0x20 | 23);
			enableSpi();
			spiWriteData(0x400000);
			while(!(spiReadRegister(OFS_STATUS) & 0x20));
			if(argv[3][0] == 'f')
				spiWriteData(0x400900);
			else
				spiWriteData(0x4009ff);
			while(!(spiReadRegister(OFS_STATUS) & 0x20));
			disableSpi();
		}
		else if(offset == OFS_DATA)	
			spiWriteData(strtol(argv[3], NULL, 16));
		else
			spiWriteRegister(offset, atoi(argv[3]));
	}
	return EXIT_SUCCESS;
}
