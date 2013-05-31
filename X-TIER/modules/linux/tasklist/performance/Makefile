EXTRA_CFLAGS += -mcmodel=large -fno-stack-protector -I../../performance
obj-m = tasklist_performance.o
tasklist_performance-objs = ../../performance/performance.o tasklist_perf.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean

