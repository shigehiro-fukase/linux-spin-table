DTC	= dtc
DTS2DTB	= $(DTC) -I dts -O dtb
DTB2DTS	= $(DTC) -I dtb -O dts
RM	= rm -f
NULL	=

DTS	+= $(wildcard *.dts)
DTB	+= $(DTS:.dts=.dtb)

DTSO	+= $(wildcard *.dtso)
DTBO	+= $(DTSO:.dtso=.dtbo)

define INSTALL
	set -x; \
	dtbo=$(1); \
	pid=$(2); \
	sudo mkdir -p /sys/kernel/config/device-tree/overlays/$${pid}; \
	ls -l /sys/kernel/config/device-tree/overlays/$${pid}; \
	sudo sh -c "cat $${dtbo} > /sys/kernel/config/device-tree/overlays/$${pid}/dtbo"; \
	cat /sys/kernel/config/device-tree/overlays/$${pid}/status; \
	ls /proc/device-tree/; \
	sudo modprobe uio_pdrv_genirq of_id="generic-uio"; \
	ls /dev/uio*; \
	$(NULL)
endef

rpi_ver = $(shell grep "^Model" /proc/cpuinfo | awk '{print $$5;}')


all:	$(DTB) $(DTBO)

%.dtb: %.dts
	$(DTS2DTB) -o $@ $<

%.dtbo: %.dtso
	$(DTS2DTB) -o $@ $<

install-rpi: rpi$(rpi_ver)-uio.dtbo
	$(call INSTALL,$<,$$$$)

clean:
	$(RM)	$(DTB) $(DTBO)
	$(RM)	*~

distclean: clean
