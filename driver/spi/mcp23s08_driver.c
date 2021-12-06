// SPI IP
// QE Driver (qe_driver.c)
// Sarker Nadir Afridi Azmi

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: DE1-SoC Board

// Hardware configuration:
// SPI:
//   GPIO_0[7,9,11,13,15,17,19] maps to The SPI_TX, SPI_RX, SPI_CLK, and ~SPI_CS0 - 3
// HPS interface:
//   Mapped to offset of 0x8000 in light-weight MM interface aperature

// Load kernel module with insmod spi_driver.ko [param=___]

//-----------------------------------------------------------------------------

#include <linux/kernel.h>     // kstrtouint
#include <linux/module.h>     // MODULE_ macros
#include <linux/init.h>       // __init
#include <linux/kobject.h>    // kobject, kobject_atribute,
							  // kobject_create_and_add, kobject_put
#include <asm/io.h>           // iowrite, ioread, ioremap_nocache (platform specific)
#include "spi_regs.h"         // register offsets in SPI IP
#include "../address_map.h"   // overall memory map

#define CS0_OFFSET				0x200
#define CS_SELECT_OFFSET		0x00D
#define CS_ENABLE				0x00008000
#define SPI_MODE_MASK			0x03
#define SPI_MODE_OFFSET			0x10
#define WORD_SIZE_MASK			0x1F

#define MCP23S08_ADDRESS		0x40
#define DIR_REG					0x00
#define DATA_REG				0x09
#define GPPU_REG				0x06

//-----------------------------------------------------------------------------
// Kernel module information
//-----------------------------------------------------------------------------

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sarker Nadir Afridi Azmi");
MODULE_DESCRIPTION("MCP23S08 IP Driver");

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

static unsigned int* base = NULL;

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void spiWriteData(uint32_t data)
{
	iowrite32(data, base + OFS_DATA);
}

int spiReadData(void)
{
	return ioread32(base + OFS_DATA);
}

uint8_t spiReadRegister(uint8_t reg)
{
	return ioread32(base + reg);
}

bool rxFifoIsEmpty(void)
{
	return ioread32(base + OFS_STATUS) & 4;
}

bool isCsManualEnabled(uint8_t n)
{
	return ioread32(base + OFS_CONTROL) & (CS0_OFFSET << n);
}

void spiEnableCS(uint8_t n)
{
	int cs = ioread32(base + OFS_CONTROL);
	iowrite32(cs | (CS0_OFFSET << n), base + OFS_CONTROL);
}

void spiDisableCS(uint8_t n)
{
	int cs = ioread32(base + OFS_CONTROL);
	iowrite32(cs & ~(CS0_OFFSET << n), base + OFS_CONTROL);
}

void spiCsSelect(uint32_t n)
{
	int cs = ioread32(base + OFS_CONTROL) & ~(0x00006000);
	iowrite32(cs | (n << CS_SELECT_OFFSET), base + OFS_CONTROL);
}

bool isCsAutoEnabled(uint8_t n)
{
	return ioread32(base + OFS_CONTROL) & (0x00000020 << n);
}

void spiCsAutoEnable(uint8_t n)
{
	int cs = ioread32(base + OFS_CONTROL) & ~(0x00000020 << n);
	iowrite32(cs | (0x00000020 << n), base + OFS_CONTROL);
}

void spiCsAutoDisable(uint8_t n)
{
	int cs = ioread32(base + OFS_CONTROL) & ~(0x00000020 << n);
	iowrite32(cs, base + OFS_CONTROL);
}

void spiClearCsSelect(void)
{
	int cs = ioread32(base + OFS_CONTROL) & ~(0x00006000);
	iowrite32(cs, base + OFS_CONTROL);
}

void spiEnable(void)
{
	int cs = ioread32(base + OFS_CONTROL) | CS_ENABLE;
	iowrite32(cs, base + OFS_CONTROL);
}

void spiDisable(void)
{
	int cs = ioread32(base + OFS_CONTROL) & ~CS_ENABLE;
	iowrite32(cs, base + OFS_CONTROL);
}

void spiSetMode(uint8_t n, uint32_t spoSph)
{
	int mode = ioread32(base + OFS_CONTROL) & ~(SPI_MODE_MASK << (SPI_MODE_OFFSET + (n << 1)));
	iowrite32(mode | (spoSph << (SPI_MODE_OFFSET + (n << 1))), base + OFS_CONTROL);
}

// The cycle is fixed to 50 MHz
void spiSetBaudRate(uint32_t baudRate)
{
	// Baud Rate = fcycle / divisor
	unsigned int divisor = (50000000 / 2) / baudRate;
	iowrite32(divisor << 7, base + OFS_BRD);
}

void setWordSize(uint8_t wordSize)
{
	int readWordSize = ioread32(base + OFS_CONTROL) & ~WORD_SIZE_MASK;
	iowrite32(readWordSize | wordSize, base + OFS_CONTROL);
}

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

//-----------------------------------------------------------------------------
// Kernel Objects
//-----------------------------------------------------------------------------

// Pin 0
static unsigned int dir_0 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(dir_0, uint, S_IRUGO);
MODULE_PARM_DESC(dir_0, " Set the pin as an input or output");

static ssize_t direction0Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &dir_0);
	uint32_t dir = 0;
	if (result == 0)
	{
		dir = readRegisterMcp23s08(DIR_REG);
		if (dir_0 == 0)
			dir &= ~(1 << 0);
		else if(dir_0 == 1)
			dir |= (1 << 0);
		writeRegisterMcp23s08(DIR_REG, (dir & 0xFF));
	}
	return count;
}

static ssize_t direction0Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	return sprintf(buffer, "%hhx\n", dir_0);
}

static struct kobj_attribute dir0Attr = __ATTR(dir, 0664, direction0Show, direction0Store);

static unsigned int data_0 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(data_0, uint, S_IRUGO);
MODULE_PARM_DESC(data_0, " Set the pin low or high");

static ssize_t data0Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &data_0);
	uint32_t data = 0;
	if (result == 0)
	{
		data = readRegisterMcp23s08(DATA_REG);
		if (data_0 == 0)
			data &= ~(1 << 0);
		else if (data_0 == 1)
			data |= (1 << 0);
		writeRegisterMcp23s08(DATA_REG, (data & 0xFF));
	}
	return count;
}

static ssize_t data0Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	return sprintf(buffer, "%hhx\n", data_0);
}

static struct kobj_attribute data0Attr = __ATTR(data, 0664, data0Show, data0Store);

static unsigned int pullup_0 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(pullup_0, uint, S_IRUGO);
MODULE_PARM_DESC(pullup_0, " Set the pin to have a pull up resistor internally");

static ssize_t pullup0Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &pullup_0);
	uint32_t pullup = 0;
	if (result == 0)
	{
		pullup = readRegisterMcp23s08(GPPU_REG);
		if (pullup_0 == 0)
			pullup &= ~(1 << 0);
		else if (pullup_0 == 1)
			pullup |= (1 << 0);
		writeRegisterMcp23s08(GPPU_REG, (pullup & 0xFF));
	}
	return count;
}

static ssize_t pullup0Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	return sprintf(buffer, "%hhx\n", pullup_0);
}

static struct kobj_attribute pullup0Attr = __ATTR(pullup, 0664, pullup0Show, pullup0Store);

// Pin 1
static unsigned int dir_1 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(dir_1, uint, S_IRUGO);
MODULE_PARM_DESC(dir_1, " Set the pin as an input or output");

static ssize_t direction1Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &dir_1);
	uint32_t dir = 0;
	if (result == 0)
	{
		dir = readRegisterMcp23s08(DIR_REG);
		if (dir_1 == 0)
			dir &= ~(1 << 1);
		else if (dir_1 == 1)
			dir |= (1 << 1);
		writeRegisterMcp23s08(DIR_REG, (dir & 0xFF));
	}
	return count;
}

static ssize_t direction1Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	return sprintf(buffer, "%hhx\n", dir_1);
}

static struct kobj_attribute dir1Attr = __ATTR(dir, 0664, direction1Show, direction1Store);

static unsigned int data_1 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(data_1, uint, S_IRUGO);
MODULE_PARM_DESC(data_1, " Set the pin low or high");

static ssize_t data1Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &data_1);
	uint32_t data = 0;
	if (result == 0)
	{
		data = readRegisterMcp23s08(DATA_REG);
		if (data_1 == 0)
			data &= ~(1 << 1);
		else if (data_1 == 1)
			data |= (1 << 1);
		writeRegisterMcp23s08(DATA_REG, (data & 0xFF));
	}
	return count;
}

static ssize_t data1Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	return sprintf(buffer, "%hhx\n", data_1);
}

static struct kobj_attribute data1Attr = __ATTR(data, 0664, data1Show, data1Store);

static unsigned int pullup_1 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(pullup_1, uint, S_IRUGO);
MODULE_PARM_DESC(pullup_1, " Set the pin to have a pull up resistor internally");

static ssize_t pullup1Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &pullup_1);
	uint32_t pullup = 0;
	if (result == 0)
	{
		pullup = readRegisterMcp23s08(GPPU_REG);
		if (pullup_1 == 0)
			pullup &= ~(1 << 1);
		else if (pullup_1 == 1)
			pullup |= (1 << 1);
		writeRegisterMcp23s08(GPPU_REG, (pullup & 0xFF));
	}
	return count;
}

static ssize_t pullup1Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	return sprintf(buffer, "%hhx\n", pullup_1);
}

static struct kobj_attribute pullup1Attr = __ATTR(pullup, 0664, pullup1Show, pullup1Store);

// Pin 2
static unsigned int dir_2 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(dir_2, uint, S_IRUGO);
MODULE_PARM_DESC(dir_2, " Set the pin as an input or output");

static ssize_t direction2Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &dir_2);
	uint32_t dir = 0;
	if (result == 0)
	{
		dir = readRegisterMcp23s08(DIR_REG);
		if (dir_2 == 0)
			dir &= ~(1 << 2);
		else if (dir_2 == 1)
			dir |= (1 << 2);
		writeRegisterMcp23s08(DIR_REG, (dir & 0xFF));
	}
	return count;
}

static ssize_t direction2Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	return sprintf(buffer, "%hhx\n", dir_2);
}

static struct kobj_attribute dir2Attr = __ATTR(dir, 0664, direction2Show, direction2Store);

static unsigned int data_2 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(data_2, uint, S_IRUGO);
MODULE_PARM_DESC(data_2, " Set the pin low or high");

static ssize_t data2Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &data_2);
	uint32_t data = 0;
	if (result == 0)
	{
		data = readRegisterMcp23s08(DATA_REG);
		if (data_2 == 0)
			data &= ~(1 << 2);
		else if (data_2 == 1)
			data |= (1 << 2);
		writeRegisterMcp23s08(DATA_REG, (data & 0xFF));
	}
	return count;
}

static ssize_t data2Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	return sprintf(buffer, "%hhx\n", data_2);
}

static struct kobj_attribute data2Attr = __ATTR(data, 0664, data2Show, data2Store);

static unsigned int pullup_2 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(pullup_2, uint, S_IRUGO);
MODULE_PARM_DESC(pullup_2, " Set the pin to have a pull up resistor internally");

static ssize_t pullup2Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &pullup_2);
	uint32_t pullup = 0;
	if (result == 0)
	{
		pullup = readRegisterMcp23s08(GPPU_REG);
		if (pullup_2 == 0)
			pullup &= ~(1 << 2);
		else if (pullup_2 == 1)
			pullup |= (1 << 2);
		writeRegisterMcp23s08(GPPU_REG, (pullup & 0xFF));
	}
	return count;
}

static ssize_t pullup2Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	return sprintf(buffer, "%hhx\n", pullup_2);
}

static struct kobj_attribute pullup2Attr = __ATTR(pullup, 0664, pullup2Show, pullup2Store);

// Pin 3
static unsigned int dir_3 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(dir_3, uint, S_IRUGO);
MODULE_PARM_DESC(dir_3, " Set the pin as an input or output");

static ssize_t direction3Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &dir_3);
	uint32_t dir = 0;
	if (result == 0)
	{
		dir = readRegisterMcp23s08(DIR_REG);
		if (dir_3 == 0)
			dir &= ~(1 << 3);
		else if (dir_3 == 1)
			dir |= (1 << 3);
		writeRegisterMcp23s08(DIR_REG, (dir & 0xFF));
	}
	return count;
}

static ssize_t direction3Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	dir_3 = readRegisterMcp23s08(DIR_REG);
	return sprintf(buffer, "%hhx\n", (dir_3 & 8) ? 1 : 0);
}

static struct kobj_attribute dir3Attr = __ATTR(dir, 0664, direction3Show, direction3Store);

static unsigned int data_3 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(data_3, uint, S_IRUGO);
MODULE_PARM_DESC(data_3, " Set the pin low or high");

static ssize_t data3Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &data_3);
	uint32_t data = 0;
	if (result == 0)
	{
		data = readRegisterMcp23s08(DATA_REG);
		if (data_3 == 0)
			data &= ~(1 << 3);
		else if (data_3 == 1)
			data |= (1 << 3);
		writeRegisterMcp23s08(DATA_REG, (data & 0xFF));
	}
	return count;
}

static ssize_t data3Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	data_3 = readRegisterMcp23s08(DATA_REG);
	return sprintf(buffer, "%hhx\n", (data_3 & 8) ? 1 : 0);
}

static struct kobj_attribute data3Attr = __ATTR(data, 0664, data3Show, data3Store);

static unsigned int pullup_3 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(pullup_3, uint, S_IRUGO);
MODULE_PARM_DESC(pullup_3, " Set the pin to have a pull up resistor internally");

static ssize_t pullup3Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &pullup_3);
	uint32_t pullup = 0;
	if (result == 0)
	{
		pullup = readRegisterMcp23s08(GPPU_REG);
		if (pullup_3 == 0)
			pullup &= ~(1 << 3);
		else if (pullup_3 == 1)
			pullup |= (1 << 3);
		writeRegisterMcp23s08(GPPU_REG, (pullup & 0xFF));
	}
	return count;
}

static ssize_t pullup3Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	pullup_3 = readRegisterMcp23s08(GPPU_REG);
	return sprintf(buffer, "%hhx\n", (pullup_3 & 8) ? 1 : 0);
}

static struct kobj_attribute pullup3Attr = __ATTR(pullup, 0664, pullup3Show, pullup3Store);

// Pin 4
static unsigned int dir_4 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(dir_4, uint, S_IRUGO);
MODULE_PARM_DESC(dir_4, " Set the pin as an input or output");

static ssize_t direction4Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &dir_4);
	uint32_t dir = 0;
	if (result == 0)
	{
		dir = readRegisterMcp23s08(DIR_REG);
		if (dir_4 == 0)
			dir &= ~(1 << 4);
		else if (dir_4 == 1)
			dir |= (1 << 4);
		writeRegisterMcp23s08(DIR_REG, (dir & 0xFF));
	}
	return count;
}

static ssize_t direction4Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	dir_4 = readRegisterMcp23s08(DIR_REG);
	return sprintf(buffer, "%hhx\n", (dir_4 & 16) ? 1 : 0);
}

static struct kobj_attribute dir4Attr = __ATTR(dir, 0664, direction4Show, direction4Store);

static unsigned int data_4 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(data_4, uint, S_IRUGO);
MODULE_PARM_DESC(data_4, " Set the pin low or high");

static ssize_t data4Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &data_4);
	uint32_t data = 0;
	if (result == 0)
	{
		data = readRegisterMcp23s08(DATA_REG);
		if (data_4 == 0)
			data &= ~(1 << 4);
		else if (data_4 == 1)
			data |= (1 << 4);
		writeRegisterMcp23s08(DATA_REG, (data & 0xFF));
	}
	return count;
}

static ssize_t data4Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	data_4 = readRegisterMcp23s08(DATA_REG);
	return sprintf(buffer, "%hhx\n", (data_4 & 16) ? 1 : 0);
}

static struct kobj_attribute data4Attr = __ATTR(data, 0664, data4Show, data4Store);

static unsigned int pullup_4 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(pullup_4, uint, S_IRUGO);
MODULE_PARM_DESC(pullup_4, " Set the pin to have a pull up resistor internally");

static ssize_t pullup4Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &pullup_4);
	uint32_t pullup = 0;
	if (result == 0)
	{
		pullup = readRegisterMcp23s08(GPPU_REG);
		if (pullup_4 == 0)
			pullup &= ~(1 << 4);
		else if (pullup_4 == 1)
			pullup |= (1 << 4);
		writeRegisterMcp23s08(GPPU_REG, (pullup & 0xFF));
	}
	return count;
}

static ssize_t pullup4Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	pullup_4 = readRegisterMcp23s08(GPPU_REG);
	return sprintf(buffer, "%hhx\n", (pullup_4 & 16) ? 1 : 0);
}

static struct kobj_attribute pullup4Attr = __ATTR(pullup, 0664, pullup4Show, pullup4Store);

// Pin 5
static unsigned int dir_5 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(dir_5, uint, S_IRUGO);
MODULE_PARM_DESC(dir_5, " Set the pin as an input or output");

static ssize_t direction5Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &dir_5);
	uint32_t dir = 0;
	if (result == 0)
	{
		dir = readRegisterMcp23s08(DIR_REG);
		if (dir_5 == 0)
			dir &= ~(1 << 5);
		else if (dir_5 == 1)
			dir |= (1 << 5);
		writeRegisterMcp23s08(DIR_REG, (dir & 0xFF));
	}
	return count;
}

static ssize_t direction5Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	dir_5 = readRegisterMcp23s08(DIR_REG);
	return sprintf(buffer, "%hhx\n", (dir_5 & 32) ? 1 : 0);
}

static struct kobj_attribute dir5Attr = __ATTR(dir, 0664, direction5Show, direction5Store);

static unsigned int data_5 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(data_5, uint, S_IRUGO);
MODULE_PARM_DESC(data_5, " Set the pin low or high");

static ssize_t data5Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &data_5);
	uint32_t data = 0;
	if (result == 0)
	{
		data = readRegisterMcp23s08(DATA_REG);
		if (data_5 == 0)
			data &= ~(1 << 5);
		else if (data_5 == 1)
			data |= (1 << 5);
		writeRegisterMcp23s08(DATA_REG, (data & 0xFF));
	}
	return count;
}

static ssize_t data5Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	data_5 = readRegisterMcp23s08(DATA_REG);
	return sprintf(buffer, "%hhx\n", (data_5 & 32) ? 1 : 0);
}

static struct kobj_attribute data5Attr = __ATTR(data, 0664, data5Show, data5Store);

static unsigned int pullup_5 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(pullup_5, uint, S_IRUGO);
MODULE_PARM_DESC(pullup_5, " Set the pin to have a pull up resistor internally");

static ssize_t pullup5Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &pullup_5);
	uint32_t pullup = 0;
	if (result == 0)
	{
		pullup = readRegisterMcp23s08(GPPU_REG);
		if (pullup_5 == 0)
			pullup &= ~(1 << 5);
		else if (pullup_5 == 1)
			pullup |= (1 << 5);
		writeRegisterMcp23s08(GPPU_REG, (pullup & 0xFF));
	}
	return count;
}

static ssize_t pullup5Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	pullup_5 = readRegisterMcp23s08(GPPU_REG);
	return sprintf(buffer, "%hhx\n", (pullup_5 & 32) ? 1 : 0);
}

static struct kobj_attribute pullup5Attr = __ATTR(pullup, 0664, pullup5Show, pullup5Store);

// Pin 6
static unsigned int dir_6 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(dir_6, uint, S_IRUGO);
MODULE_PARM_DESC(dir_6, " Set the pin as an input or output");

static ssize_t direction6Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &dir_6);
	uint32_t dir = 0;
	if (result == 0)
	{
		dir = readRegisterMcp23s08(DIR_REG);
		if (dir_6 == 0)
			dir &= ~(1 << 7);
		else if (dir_6 == 1)
			dir |= (1 << 7);
		writeRegisterMcp23s08(DIR_REG, (dir & 0xFF));
	}
	return count;
}

static ssize_t direction6Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	dir_6 = readRegisterMcp23s08(DIR_REG);
	return sprintf(buffer, "%hhx\n", (dir_6 & 64) ? 1 : 0);
}

static struct kobj_attribute dir6Attr = __ATTR(dir, 0664, direction6Show, direction6Store);

static unsigned int data_6 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(data_6, uint, S_IRUGO);
MODULE_PARM_DESC(data_6, " Set the pin low or high");

static ssize_t data6Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &data_6);
	uint32_t data = 0;
	if (result == 0)
	{
		data = readRegisterMcp23s08(DATA_REG);
		if (data_6 == 0)
			data &= ~(1 << 6);
		else if (data_6 == 1)
			data |= (1 << 6);
		writeRegisterMcp23s08(DATA_REG, (data & 0xFF));
	}
	return count;
}

static ssize_t data6Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	data_6 = readRegisterMcp23s08(DATA_REG);
	return sprintf(buffer, "%hhx\n", (data_6 & 64) ? 1 : 0);
}

static struct kobj_attribute data6Attr = __ATTR(data, 0664, data6Show, data6Store);

static unsigned int pullup_6 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(pullup_6, uint, S_IRUGO);
MODULE_PARM_DESC(pullup_6, " Set the pin to have a pull up resistor internally");

static ssize_t pullup6Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &pullup_6);
	uint32_t pullup = 0;
	if (result == 0)
	{
		pullup = readRegisterMcp23s08(GPPU_REG);
		if (pullup_6 == 0)
			pullup &= ~(1 << 6);
		else if (pullup_6 == 1)
			pullup |= (1 << 6);
		writeRegisterMcp23s08(GPPU_REG, (pullup & 0xFF));
	}
	return count;
}

static ssize_t pullup6Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	pullup_6 = readRegisterMcp23s08(GPPU_REG);
	return sprintf(buffer, "%hhx\n", (pullup_6 & 64) ? 1 : 0);
}

static struct kobj_attribute pullup6Attr = __ATTR(pullup, 0664, pullup6Show, pullup6Store);

// Pin 7
static unsigned int dir_7 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(dir_7, uint, S_IRUGO);
MODULE_PARM_DESC(dir_7, " Set the pin as an input or output");

static ssize_t direction7Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &dir_7);
	uint32_t dir = 0;
	if (result == 0)
	{
		dir = readRegisterMcp23s08(DIR_REG);
		if (dir_7 == 0)
			dir &= ~(1 << 7);
		else if (dir_7 == 1)
			dir |= (1 << 7);
		writeRegisterMcp23s08(DIR_REG, (dir & 0xFF));
	}
	return count;
}

static ssize_t direction7Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	dir_7 = readRegisterMcp23s08(DIR_REG);
	return sprintf(buffer, "%hhx\n", (dir_7 & 128) ? 1 : 0);
}

static struct kobj_attribute dir7Attr = __ATTR(dir, 0664, direction7Show, direction7Store);

static unsigned int data_7 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(data_7, uint, S_IRUGO);
MODULE_PARM_DESC(data_7, " Set the pin low or high");

static ssize_t data7Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &data_7);
	uint32_t data = 0;
	if (result == 0)
	{
		data = readRegisterMcp23s08(DATA_REG);
		if (data_7 == 0)
			data &= ~(1 << 7);
		else if (data_7 == 1)
			data |= (1 << 7);
		writeRegisterMcp23s08(DATA_REG, (data & 0xFF));
	}
	return count;
}

static ssize_t data7Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	data_7 = readRegisterMcp23s08(DATA_REG);
	return sprintf(buffer, "%hhx\n", (data_7 & 128) ? 1 : 0);
}

static struct kobj_attribute data7Attr = __ATTR(data, 0664, data7Show, data7Store);

static unsigned int pullup_7 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(pullup_7, uint, S_IRUGO);
MODULE_PARM_DESC(pullup_7, " Set the pin to have a pull up resistor internally");

static ssize_t pullup7Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &pullup_7);
	uint32_t pullup = 0;
	if (result == 0)
	{
		pullup = readRegisterMcp23s08(GPPU_REG);
		if (pullup_7 == 0)
			pullup &= ~(1 << 7);
		else if (pullup_7 == 1)
			pullup |= (1 << 7);
		writeRegisterMcp23s08(GPPU_REG, (pullup & 0xFF));
	}
	return count;
}

static ssize_t pullup7Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	pullup_7 = readRegisterMcp23s08(GPPU_REG);
	return sprintf(buffer, "%hhx\n", (pullup_7 & 128) ? 1 : 0);
}

static struct kobj_attribute pullup7Attr = __ATTR(pullup, 0664, pullup7Show, pullup7Store);

// Attributes
static struct attribute* dev0Attrs[] = { &dir0Attr.attr, &data0Attr.attr, &pullup0Attr.attr, NULL };
static struct attribute* dev1Attrs[] = { &dir1Attr.attr, &data1Attr.attr, &pullup1Attr.attr, NULL };
static struct attribute* dev2Attrs[] = { &dir2Attr.attr, &data2Attr.attr, &pullup2Attr.attr, NULL };
static struct attribute* dev3Attrs[] = { &dir3Attr.attr, &data3Attr.attr, &pullup3Attr.attr, NULL };
static struct attribute* dev4Attrs[] = { &dir4Attr.attr, &data4Attr.attr, &pullup4Attr.attr, NULL };
static struct attribute* dev5Attrs[] = { &dir5Attr.attr, &data5Attr.attr, &pullup5Attr.attr, NULL };
static struct attribute* dev6Attrs[] = { &dir6Attr.attr, &data6Attr.attr, &pullup6Attr.attr, NULL };
static struct attribute* dev7Attrs[] = { &dir7Attr.attr, &data7Attr.attr, &pullup7Attr.attr, NULL };

static struct attribute_group group0 =
{
	.name = "0",
	.attrs = dev0Attrs
};

static struct attribute_group group1 =
{
	.name = "1",
	.attrs = dev1Attrs
};

static struct attribute_group group2 =
{
	.name = "2",
	.attrs = dev2Attrs
};

static struct attribute_group group3 =
{
	.name = "3",
	.attrs = dev3Attrs
};

static struct attribute_group group4 =
{
	.name = "4",
	.attrs = dev4Attrs
};

static struct attribute_group group5 =
{
	.name = "5",
	.attrs = dev5Attrs
};

static struct attribute_group group6 =
{
	.name = "6",
	.attrs = dev6Attrs
};

static struct attribute_group group7 =
{
	.name = "7",
	.attrs = dev7Attrs
};

static struct kobject* kobj;

//-----------------------------------------------------------------------------
// Initialization and Exit
//-----------------------------------------------------------------------------

static int __init initialize_module(void)
{
	int result;

	printk(KERN_INFO "MCP23S08 driver: starting\n");

	// Create qe directory under /sys/kernel
	kobj = kobject_create_and_add("spi_expander", kernel_kobj);
	if (!kobj)
	{
		printk(KERN_ALERT "MCP23S08 Driver: failed to create and add kobj\n");
		return -ENOENT;
	}

	result = sysfs_create_group(kobj, &group0);
	if (result != 0)
		return result;

	result = sysfs_create_group(kobj, &group1);
	if (result != 0)
		return result;

	result = sysfs_create_group(kobj, &group2);
	if (result != 0)
		return result;

	result = sysfs_create_group(kobj, &group3);
	if (result != 0)
		return result;

	result = sysfs_create_group(kobj, &group4);
	if (result != 0)
		return result;

	result = sysfs_create_group(kobj, &group5);
	if (result != 0)
		return result;

	result = sysfs_create_group(kobj, &group6);
	if (result != 0)
		return result;

	result = sysfs_create_group(kobj, &group7);
	if (result != 0)
		return result;

	// Physical to virtual memory map to access spi registers
	base = (unsigned int*)ioremap_nocache(LW_BRIDGE_BASE + SPI_BASE_OFFSET, SPAN_IN_BYTES);
	if (base == NULL)
		return -ENODEV;

	// Baud Rate = 5MHz
	spiSetBaudRate(5e6);
	// Set device 0 in SPI mode 0, 0
	spiCsSelect(0);
	spiSetMode(0, 0);
	spiCsAutoEnable(0);
	// Set the word size as 24 bits
	setWordSize(23);
	spiEnable();

	printk(KERN_INFO "MCP23S08 driver: initialized\n");

	return 0;
}

static void __exit exit_module(void)
{
	spiDisable();
	kobject_put(kobj);
	printk(KERN_INFO "MCP23S08 driver: exit\n");
}

module_init(initialize_module);
module_exit(exit_module);
