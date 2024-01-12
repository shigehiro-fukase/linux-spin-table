# KERNEL_HEADERS=/home/kmc/work/rz_v2l/linux/build
# KERNEL_HEADERS=/home/kmc/work/h3/linux/build
ifeq ("$(KERNEL_HEADERS)","")
PATH1=/usr/src/linux-headers-$(shell uname -r)
PATH2=/lib/modules/$(shell uname -r)/build
ifneq ("$(wildcard $(PATH1))","")
KERNEL_HEADERS=$(PATH1)
else
KERNEL_HEADERS=$(PATH2)
endif
endif

CFILES = driver.c
obj-m := mod-paio.o
$(obj-m:.o=-objs) := $(CFILES:.c=.o)
obj-mm := $(subst -,_,$(obj-m:.o=))
obj-ko := $(obj-m:.o=.ko)

ccflags-y += -std=gnu99 -Wall -Wno-declaration-after-statement
ccflags-y += -DDRIVER_NAME=\"$(obj-m:.o=)\"

all:
	@echo "KERNEL_HEADERS=$(KERNEL_HEADERS)"
	$(MAKE) -C $(KERNEL_HEADERS) M=$(PWD) modules
	-modinfo ./*.ko
	-file ./*.ko
clean:
	$(MAKE) -C $(KERNEL_HEADERS) M=$(PWD) clean

