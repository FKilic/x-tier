EXTRA_CFLAGS += -mcmodel=large
obj-m = lsmod.o
KDIR := /lib/modules/2.6.38-8-server/build
PWD := $(shell pwd)
all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean
