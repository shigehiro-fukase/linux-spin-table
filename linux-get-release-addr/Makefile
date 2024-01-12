#CROSS_COMPILE = aarch64-poky-linux
ifeq ($(CC),"")
CC		= $(CROSS_COMPILE)gcc
endif
RM		= rm -f
NULL		=

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

LIB_SRCS	= 
MAIN_SRCS	= $(wildcard *.c)
PROGRAMS	= $(MAIN_SRCS:.c=.elf)

SRCS		= $(LIB_SRCS) $(MAIN_SRCS)
OBJS		= $(SRCS:.c=.o)

all:	$(OBJS) $(PROGRAMS)

%.o : %.c
	$(CC) $(CFLAGS) -c $<

%.elf: %.o
	$(CC) -o $@ $< $(LIB_SRCS:.c=.o)
#	$(LD) -o $@ $< $(LIB_SRCS:.c=.o) $(LDFLAGS)
	@- file $@

clean:
	$(RM)	$(OBJS)
	$(RM)	*~

distclean: clean
	$(RM)	$(PROGRAMS)

