EXTRA_CFLAGS += -mcmodel=large -fno-stack-protector -I../../performance
obj-m = lsmod_performance.o
lsmod_performance-objs = ../../performance/performance.o lsmod_perf.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean

