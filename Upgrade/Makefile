obj-m += TaskInformation_Upgrade.o 
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
.PHONY: all
all:
	make -C $(KDIR) M=$(PWD) modules
	sudo insmod TaskInformation_Upgrade.ko || echo "Module TaskInformation_Upgrade already loaded or error occurred."
