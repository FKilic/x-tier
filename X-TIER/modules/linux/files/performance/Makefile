EXTRA_CFLAGS += -mcmodel=large -fno-stack-protector -I../../performance
obj-m = files_performance.o
files_performance-objs = ../../performance/performance.o files_perf.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean

