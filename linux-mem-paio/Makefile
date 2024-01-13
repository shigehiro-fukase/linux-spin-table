#CROSS_COMPILE = aarch64-poky-linux
ifeq ($(CC),"")
CC		= $(CROSS_COMPILE)gcc
endif
CFLAGS		+= -g
CFLAGS		+= -O0
CFLAGS		+= -Wall
#CFLAGS		+= -Wwrite-strings
#CFLAGS		+= -Wstrict-prototypes
#CFLAGS		+= -Wmissing-prototypes
CFLAGS		+= -W
CFLAGS		+= -Wall
#CFLAGS		+= -static
#CFLAGS		+= -nostdinc
#CFLAGS		+= -nostdlib
#CFLAGS		+= -fno-builtin
#CFLAGS		+= -ffreestanding

#CFLAGS		+= -Wl,-O1
#CFLAGS		+= -Wl,--hash-style=gnu
#CFLAGS		+= -Wl,--as-needed

CFLAGS		+= -I$(SDKTARGETSYSROOT)/usr/include
#CFLAGS		+= -I$(PKG_CONFIG_SYSROOT_DIR)/usr/include

#LDFLAGS		+= -L$(SDKTARGETSYSROOT)/lib
#LDFLAGS		= -L$(PKG_CONFIG_SYSROOT_DIR)/lib
#LDFLAGS		+= -lc

DTC		= dtc
DTS2DTB		= $(DTC) -I dts -O dtb
DTB2DTS		= $(DTC) -I dtb -O dts

RM		= rm -f
NULL		=

LIB_SRCS	= 
MAIN_SRCS	= $(wildcard *.c)
PROGRAMS	= $(MAIN_SRCS:.c=.elf)

SRCS		= $(LIB_SRCS) $(MAIN_SRCS)
OBJS		= $(SRCS:.c=.o)

DTS		+= $(wildcard dts/*.dts)
DTSO		+= $(wildcard dts/*.dtso)

DTB		+= $(DTS:.dts=.dtb)
DTBO		+= $(DTSO:.dtso=.dtbo)

all:	$(OBJS) $(PROGRAMS)
all:	$(DTB) $(DTBO)

%.o : %.c
	$(CC) $(CFLAGS) -c $<

%.elf: %.o
	$(CC) -o $@ $< $(LIB_SRCS:.c=.o)
#	$(CC) -o $@ $< $(LIB_SRCS:.c=.o) $(LDFLAGS)
#	$(LD) -o $@ $< $(LIB_SRCS:.c=.o) $(LDFLAGS)

%.dtb: %.dts
	$(DTS2DTB) -o $@ $<

%.dtbo: %.dtso
	$(DTS2DTB) -o $@ $<

clean:
	$(RM)	$(OBJS)
	$(RM)	*~

distclean: clean
	$(RM)	$(PROGRAMS)

