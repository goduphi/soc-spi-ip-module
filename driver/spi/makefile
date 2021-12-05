obj-m += spi_driver.o

DIR=/lib/modules/$(shell uname -r)/build

all:
	make -C $(DIR) M=$(shell pwd) modules
	gcc -o spi spi_ip.c spi_utility.c
	gcc -o mcp23s08 mcp23s08.c spi_ip.c

clean:
	make -C $(DIR) M=$(shell pwd) clean
