ifeq ($(KERNELRELEASE),)

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

.PHONY: all build clean

all: transact.ko mktransact

transact.ko: transact.c | transact.h
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

mktransact: mktransact.c | transact.h
	gcc -O2 -o $@ $^

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c *.order *.symvers
else

$(info Building with KERNELRELEASE = ${KERNELRELEASE})
obj-m := transact.o

endif
