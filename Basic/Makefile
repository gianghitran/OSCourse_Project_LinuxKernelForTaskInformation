obj-m += TaskInformation_Basic.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

.PHONY: all

all:
	make -C $(KDIR) M=$(PWD) modules
	sudo insmod TaskInformation_Basic.ko || echo "Module TaskInformation_Basic already loaded or error occurred."

