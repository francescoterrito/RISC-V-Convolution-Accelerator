############################################
#          CONFIGURAZIONE PROG             #
############################################

# Application name passed from top-level
PROG ?= example

# Application directories
APPDIR     := applications/$(PROG)
DRIVERDIR  := drivers/$(PROG)
CTRLREGDIR := control_reg/$(PROG)

# Library directories
LIBDIR     := lib/src

# Common system files
CRT0 := crt0.S
LINK := link.ld

############################################
#              TOOLCHAIN                   #
############################################

RISCV_XLEN    ?= 32
RISCV_MARCH   ?= rv$(RISCV_XLEN)i_zicsr
RISCV_MABI    ?= ilp32
RISCV_PREFIX  ?= riscv64-unknown-elf-

RISCV_CC      ?= $(RISCV_PREFIX)gcc
RISCV_OBJDUMP ?= $(RISCV_PREFIX)objdump
RISCV_OBJCOPY ?= $(RISCV_PREFIX)objcopy

RISCV_FLAGS    := -march=$(RISCV_MARCH) -mabi=$(RISCV_MABI) -mcmodel=medany -static -std=gnu99 -Os -nostdlib -fno-builtin -ffreestanding
RISCV_CCFLAGS  := $(RISCV_FLAGS) -Iinclude -Ilib/inc -I. -I$(APPDIR) -I$(DRIVERDIR) -I$(CTRLREGDIR)
RISCV_LDFLAGS  := -static -nostartfiles $(RISCV_FLAGS)

############################################
#             FILE LISTS                   #
############################################

# Application sources
APP_SRCS := $(wildcard $(APPDIR)/*.c) $(wildcard $(APPDIR)/*.S)

# Driver sources (optional per app)
DRV_SRCS := $(wildcard $(DRIVERDIR)/*.c) $(wildcard $(DRIVERDIR)/*.S)

# Library sources
LIB_SRCS := $(wildcard $(LIBDIR)/*.[cS])

# Convert to objects
APP_OBJS := $(APP_SRCS:.c=.c.o)
APP_OBJS := $(APP_OBJS:.S=.S.o)

DRV_OBJS := $(DRV_SRCS:.c=.c.o)
DRV_OBJS := $(DRV_OBJS:.S=.S.o)

LIB_OBJS := $(LIB_SRCS:.c=.c.o)
LIB_OBJS := $(LIB_OBJS:.S=.S.o)

CRT0_OBJ := $(CRT0:.S=.S.o)

OBJS := $(APP_OBJS) $(DRV_OBJS) $(LIB_OBJS) $(CRT0_OBJ)

############################################
#               OUTPUT                     #
############################################

BINDIR := bin
OUT_ELF := $(BINDIR)/main.elf
OUT_HEX := $(BINDIR)/main.hex

############################################
#                RULES                     #
############################################

all: compile

$(BINDIR):
	mkdir -p $(BINDIR)

%.c.o: %.c
	$(RISCV_CC) $(RISCV_CCFLAGS) -c $< -o $@

%.S.o: %.S
	$(RISCV_CC) $(RISCV_CCFLAGS) -c $< -o $@

$(OUT_ELF): $(OBJS) $(LINK) | $(BINDIR)
	$(RISCV_CC) $(RISCV_LDFLAGS) -T$(LINK) -o $@ $(OBJS) -lm -lgcc -lc

$(OUT_HEX): $(OUT_ELF)
	$(RISCV_OBJCOPY) -O verilog $< $@

compile: $(OUT_HEX)

clean:
	rm -rf $(BINDIR)
	find . -name "*.o" -delete

.PHONY: all clean compile
