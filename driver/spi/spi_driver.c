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

//-----------------------------------------------------------------------------
// Kernel module information
//-----------------------------------------------------------------------------

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sarker Nadir Afridi Azmi");
MODULE_DESCRIPTION("SPI IP Driver");

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
	int data = ioread32(base + OFS_DATA);
	return data;
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

//-----------------------------------------------------------------------------
// Kernel Objects
//-----------------------------------------------------------------------------

// SPI Enable
static bool spi_enable = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(spi_enable, bool, S_IRUGO);
MODULE_PARM_DESC(spi_enable, " Enables the SPI engine");

static ssize_t spiEnableStore(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	if (strncmp(buffer, "true", count - 1) == 0)
	{
		spiEnable();
		spi_enable = true;
	}
	else
		if (strncmp(buffer, "false", count - 1) == 0)
		{
			spiDisable();
			spi_enable = false;
		}
	return count;
}

static ssize_t spiEnableShow(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	spi_enable = isCsAutoEnabled(1);
	if (spi_enable)
		strcpy(buffer, "true\n");
	else
		strcpy(buffer, "false\n");
	return strlen(buffer);
}

static struct kobj_attribute spiEnableAttr = __ATTR(spi_enable, 0664, spiEnableShow, spiEnableStore);

// Baud Rate
static unsigned int baud_rate = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(baud_rate, uint, S_IRUGO);
MODULE_PARM_DESC(baud_rate, " Set the Baud Rate");

static ssize_t baudRateStore(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &baud_rate);
	if (result == 0)
		spiSetBaudRate(baud_rate);
	return count;
}

static ssize_t baudRateShow(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	return sprintf(buffer, "%u\n", baud_rate);
}

static struct kobj_attribute baudRateAttr = __ATTR(baud_rate, 0664, baudRateShow, baudRateStore);

// Word Size
static unsigned int word_size = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(word_size, uint, S_IRUGO);
MODULE_PARM_DESC(word_size, " Set the Word Size");

static ssize_t wordSizeStore(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &word_size);
	if (result == 0)
		setWordSize(word_size - 1);
	return count;
}

static ssize_t wordSizeShow(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	return sprintf(buffer, "%u\n", word_size);
}

static struct kobj_attribute wordSizeAttr = __ATTR(word_size, 0664, wordSizeShow, wordSizeStore);

// CS Select
static unsigned int cs_select = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(cs_select, uint, S_IRUGO);
MODULE_PARM_DESC(cs_select, " Set the Chipselect");

static ssize_t csSelectStore(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &cs_select);
	if (result == 0)
		spiCsSelect(cs_select);
	return count;
}

static ssize_t csSelectShow(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	return sprintf(buffer, "%u\n", cs_select);
}

static struct kobj_attribute csSelectAttr = __ATTR(cs_select, 0664, csSelectShow, csSelectStore);

// Device 0
// Mode 0
static unsigned int mode0 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(mode0, uint, S_IRUGO);
MODULE_PARM_DESC(mode0, " Sets the SPI Mode");

static ssize_t mode0Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &mode0);
	if (result == 0)
		spiSetMode(0, mode0);
	return count;
}

static ssize_t mode0Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	return sprintf(buffer, "%u\n", mode0);
}

static struct kobj_attribute mode0Attr = __ATTR(mode, 0664, mode0Show, mode0Store);

// CS Auto 0
static bool cs_auto0 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(cs_auto0, bool, S_IRUGO);
MODULE_PARM_DESC(cs_auto0, " Sets the SPI CS Auto Mode");

static ssize_t csAuto0Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	if (strncmp(buffer, "true", count - 1) == 0)
	{
		spiCsAutoEnable(0);
		cs_auto0 = true;
	}
	else
		if (strncmp(buffer, "false", count - 1) == 0)
		{
			spiCsAutoDisable(0);
			cs_auto0 = false;
		}
	return count;
}

static ssize_t csAuto0Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	cs_auto0 = isCsAutoEnabled(1);
	if (cs_auto0)
		strcpy(buffer, "true\n");
	else
		strcpy(buffer, "false\n");
	return strlen(buffer);
}

static struct kobj_attribute csAuto0Attr = __ATTR(cs_auto, 0664, csAuto0Show, csAuto0Store);

// CS Manual 0
static bool cs_man0 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(cs_man0, bool, S_IRUGO);
MODULE_PARM_DESC(cs_man0, " Sets the SPI CS Manual Mode");

static ssize_t csMan0Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	if (strncmp(buffer, "true", count - 1) == 0)
	{
		spiEnableCS(0);
		cs_man0 = true;
	}
	else
		if (strncmp(buffer, "false", count - 1) == 0)
		{
			spiDisableCS(0);
			cs_man0 = false;
		}
	return count;
}

static ssize_t csMan0Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	cs_man0 = isCsManualEnabled(0);
	if (cs_man0)
		strcpy(buffer, "true\n");
	else
		strcpy(buffer, "false\n");
	return strlen(buffer);
}

static struct kobj_attribute csMan0Attr = __ATTR(cs_man, 0664, csMan0Show, csMan0Store);

// Device 1
// Mode 1
static unsigned int mode1 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(mode1, uint, S_IRUGO);
MODULE_PARM_DESC(mode1, " Sets the SPI Mode");

static ssize_t mode1Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &mode1);
	if (result == 0)
		spiSetMode(1, mode1);
	return count;
}

static ssize_t mode1Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	return sprintf(buffer, "%u\n", mode1);
}

static struct kobj_attribute mode1Attr = __ATTR(mode, 0664, mode1Show, mode1Store);

// CS Auto 1
static bool cs_auto1 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(cs_auto1, bool, S_IRUGO);
MODULE_PARM_DESC(cs_auto1, " Sets the SPI CS Auto Mode");

static ssize_t csAuto1Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	if (strncmp(buffer, "true", count - 1) == 0)
	{
		spiCsAutoEnable(1);
		cs_auto1 = true;
	}
	else
		if (strncmp(buffer, "false", count - 1) == 0)
		{
			spiCsAutoDisable(1);
			cs_auto1 = false;
		}
	return count;
}

static ssize_t csAuto1Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	cs_auto1 = isCsAutoEnabled(1);
	if (cs_auto1)
		strcpy(buffer, "true\n");
	else
		strcpy(buffer, "false\n");
	return strlen(buffer);
}

static struct kobj_attribute csAuto1Attr = __ATTR(cs_auto, 0664, csAuto1Show, csAuto1Store);

// CS Manual 1
static bool cs_man1 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(cs_man1, bool, S_IRUGO);
MODULE_PARM_DESC(cs_man1, " Sets the SPI CS Manual Mode");

static ssize_t csMan1Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	if (strncmp(buffer, "true", count - 1) == 0)
	{
		spiEnableCS(1);
		cs_man1 = true;
	}
	else
		if (strncmp(buffer, "false", count - 1) == 0)
		{
			spiDisableCS(1);
			cs_man1 = false;
		}
	return count;
}

static ssize_t csMan1Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	cs_man1 = isCsManualEnabled(1);
	if (cs_man1)
		strcpy(buffer, "true\n");
	else
		strcpy(buffer, "false\n");
	return strlen(buffer);
}

static struct kobj_attribute csMan1Attr = __ATTR(cs_man, 0664, csMan1Show, csMan1Store);

// Device 2
// Mode 2
static unsigned int mode2 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(mode2, uint, S_IRUGO);
MODULE_PARM_DESC(mode2, " Sets the SPI Mode");

static ssize_t mode2Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &mode2);
	if (result == 0)
		spiSetMode(2, mode2);
	return count;
}

static ssize_t mode2Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	return sprintf(buffer, "%u\n", mode2);
}

static struct kobj_attribute mode2Attr = __ATTR(mode, 0664, mode2Show, mode2Store);

// CS Auto 2
static bool cs_auto2 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(cs_auto2, bool, S_IRUGO);
MODULE_PARM_DESC(cs_auto2, " Sets the SPI CS Auto Mode");

static ssize_t csAuto2Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	if (strncmp(buffer, "true", count - 1) == 0)
	{
		spiCsAutoEnable(2);
		cs_auto2 = true;
	}
	else
		if (strncmp(buffer, "false", count - 1) == 0)
		{
			spiCsAutoDisable(2);
			cs_auto2 = false;
		}
	return count;
}

static ssize_t csAuto2Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	cs_auto2 = isCsAutoEnabled(2);
	if (cs_auto2)
		strcpy(buffer, "true\n");
	else
		strcpy(buffer, "false\n");
	return strlen(buffer);
}

static struct kobj_attribute csAuto2Attr = __ATTR(cs_auto, 0664, csAuto2Show, csAuto2Store);

// CS Manual 2
static bool cs_man2 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(cs_man2, bool, S_IRUGO);
MODULE_PARM_DESC(cs_man2, " Sets the SPI CS Manual Mode");

static ssize_t csMan2Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	if (strncmp(buffer, "true", count - 1) == 0)
	{
		spiEnableCS(2);
		cs_man2 = true;
	}
	else
		if (strncmp(buffer, "false", count - 1) == 0)
		{
			spiDisableCS(2);
			cs_man2 = false;
		}
	return count;
}

static ssize_t csMan2Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	cs_man2 = isCsManualEnabled(2);
	if (cs_man2)
		strcpy(buffer, "true\n");
	else
		strcpy(buffer, "false\n");
	return strlen(buffer);
}

static struct kobj_attribute csMan2Attr = __ATTR(cs_man, 0664, csMan2Show, csMan2Store);

// Device 3
// Mode 3
static unsigned int mode3 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(mode3, uint, S_IRUGO);
MODULE_PARM_DESC(mode3, " Sets the SPI Mode");

static ssize_t mode3Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &mode3);
	if (result == 0)
		spiSetMode(3, mode3);
	return count;
}

static ssize_t mode3Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	return sprintf(buffer, "%u\n", mode3);
}

static struct kobj_attribute mode3Attr = __ATTR(mode, 0664, mode3Show, mode3Store);

// CS Auto 3
static bool cs_auto3 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(cs_auto3, bool, S_IRUGO);
MODULE_PARM_DESC(cs_auto3, " Sets the SPI CS Auto Mode");

static ssize_t csAuto3Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	if (strncmp(buffer, "true", count - 1) == 0)
	{
		spiCsAutoEnable(3);
		cs_auto3 = true;
	}
	else
		if (strncmp(buffer, "false", count - 1) == 0)
		{
			spiCsAutoDisable(3);
			cs_auto3 = false;
		}
	return count;
}

static ssize_t csAuto3Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	cs_auto3 = isCsAutoEnabled(3);
	if (cs_auto3)
		strcpy(buffer, "true\n");
	else
		strcpy(buffer, "false\n");
	return strlen(buffer);
}

static struct kobj_attribute csAuto3Attr = __ATTR(cs_auto, 0664, csAuto3Show, csAuto3Store);

// CS Manual 3
static bool cs_man3 = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(cs_man3, bool, S_IRUGO);
MODULE_PARM_DESC(cs_man3, " Sets the SPI CS Manual Mode");

static ssize_t csMan3Store(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	if (strncmp(buffer, "true", count - 1) == 0)
	{
		spiEnableCS(3);
		cs_man3 = true;
	}
	else
		if (strncmp(buffer, "false", count - 1) == 0)
		{
			spiDisableCS(3);
			cs_man3 = false;
		}
	return count;
}

static ssize_t csMan3Show(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	cs_man3 = isCsManualEnabled(3);
	if (cs_man3)
		strcpy(buffer, "true\n");
	else
		strcpy(buffer, "false\n");
	return strlen(buffer);
}

static struct kobj_attribute csMan3Attr = __ATTR(cs_man, 0664, csMan3Show, csMan3Store);

// TX Data
static unsigned int tx_data = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(tx_data, uint, S_IRUGO);
MODULE_PARM_DESC(tx_data, " Enqueues new data into the TX FIFO");

static ssize_t txDataStore(struct kobject* kobj, struct kobj_attribute* attr, const char* buffer, size_t count)
{
	unsigned int result = kstrtouint(buffer, 0, &tx_data);
	if (result == 0)
		spiWriteData(tx_data);
	return count;
}

static struct kobj_attribute txDataAttr = __ATTR(tx_data, 0664, NULL, txDataStore);

// RX Data
static unsigned int rx_data = 0;
// Root, Registered User, Guest - S_IRUGO
module_param(rx_data, uint, S_IRUGO);
MODULE_PARM_DESC(rx_data, " Dequeues new data into the RX FIFO");

static ssize_t rxDataShow(struct kobject* kobj, struct kobj_attribute* attr, char* buffer)
{
	bool empty = rxFifoIsEmpty();
	if(!empty)
		rx_data = spiReadData();
	return sprintf(buffer, empty ? "%d\n" : "%u\n", empty ? -1 : rx_data);
}

static struct kobj_attribute rxDataAttr = __ATTR(rx_data, 0664, rxDataShow, NULL);

// Attributes
static struct attribute* attrs[] = { &baudRateAttr.attr, &wordSizeAttr.attr, &csSelectAttr.attr, &txDataAttr.attr, &rxDataAttr.attr, &spiEnableAttr.attr, NULL };
static struct attribute* dev0Attrs[] = { &mode0Attr.attr, &csAuto0Attr.attr, &csMan0Attr.attr, NULL };
static struct attribute* dev1Attrs[] = { &mode1Attr.attr, &csAuto1Attr.attr, &csMan1Attr.attr, NULL };
static struct attribute* dev2Attrs[] = { &mode2Attr.attr, &csAuto2Attr.attr, &csMan2Attr.attr, NULL };
static struct attribute* dev3Attrs[] = { &mode3Attr.attr, &csAuto3Attr.attr, &csMan3Attr.attr, NULL };

static struct attribute_group group0 =
{
	.name = "device_0",
	.attrs = dev0Attrs
};

static struct attribute_group group1 =
{
	.name = "device_1",
	.attrs = dev1Attrs
};

static struct attribute_group group2 =
{
	.name = "device_2",
	.attrs = dev2Attrs
};

static struct attribute_group group3 =
{
	.name = "device_3",
	.attrs = dev3Attrs
};

static struct kobject* kobj;

//-----------------------------------------------------------------------------
// Initialization and Exit
//-----------------------------------------------------------------------------

static int __init initialize_module(void)
{
	int result;
	uint8_t i = 0;

	printk(KERN_INFO "SPI driver: starting\n");

	// Create qe directory under /sys/kernel
	kobj = kobject_create_and_add("spi", kernel_kobj);
	if (!kobj)
	{
		printk(KERN_ALERT "SPI driver: failed to create and add kobj\n");
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

	// Create a file for each attribute
	for (; i < 6; i++)
	{
		result = sysfs_create_file(kobj, attrs[i]);
		if (result != 0)
			return result;
	}

	// Physical to virtual memory map to access spi registers
	base = (unsigned int*)ioremap_nocache(LW_BRIDGE_BASE + SPI_BASE_OFFSET, SPAN_IN_BYTES);
	if (base == NULL)
		return -ENODEV;

	printk(KERN_INFO "SPI driver: initialized\n");

	return 0;
}

static void __exit exit_module(void)
{
	kobject_put(kobj);
	printk(KERN_INFO "SPI driver: exit\n");
}

module_init(initialize_module);
module_exit(exit_module);
