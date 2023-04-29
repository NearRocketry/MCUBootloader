TARGET = hid_bootloader
FLASH  = st-flash
DEVICE = STM32F072xB
DEBUG = 1

VECTOR_TABLE_OFFSET = 0x0000

# High Density STM32F072 devices have 1 kB Flash Page size  
PAGE_SIZE = 1024 

C_SRCS = Src/main.c Src/usb.c Src/hid.c Src/led.c Src/flash.c

# Be silent per default, but 'make V=1' will show all compiler calls.
# If you're insane, V=99 will print out all sorts of things.
V?=0
ifeq ($(V),0)
Q	:= @
NULL	:= 2>/dev/null
endif

INCLUDE_DIRS += -I Inc
INCLUDE_DIRS += -I CMSIS-Device/Include
INCLUDE_DIRS += -I CMSIS/Include

SRCS += $(C_SRCS)
vpath %.c $(sort $(dir $(C_SRCS)))
OBJS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SRCS:.c=.o)))

CFLAGS += -mcpu=cortex-m0 -mthumb -mfloat-abi=soft -Wall
CFLAGS += -std=gnu99
CFLAGS += -fno-strict-aliasing
CFLAGS += -specs=nano.specs -specs=nosys.specs
CFLAGS += -Wextra -Wshadow -Wno-unused-variable -Wimplicit-function-declaration
CFLAGS += -Wredundant-decls -Wstrict-prototypes -Wmissing-prototypes
CFLAGS += $(INCLUDE_DIRS)
CFLAGS += -D$(DEVICE)
CFLAGS += -DVECTOR_TABLE_OFFSET=$(VECTOR_TABLE_OFFSET)
CFLAGS += -nostdlib
CFLAGS += $(TARGETFLAGS)
CFLAGS += -DPAGE_SIZE=$(PAGE_SIZE)

LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,--print-memory-usage
LDFLAGS += -Wl,--no-wchar-size-warning
LDFLAGS += -nostdlib
LDFLAGS += -T$(LINKER_SCRIPT)

ifeq ($(DEBUG),0)
	CFLAGS += -Os
else
	CFLAGS += -DNDEBUG -DDEBUG -g3 -gdwarf-2 -O0
endif

# Tool paths.
PREFIX	?= arm-none-eabi-
CC = $(PREFIX)gcc
LD = $(PREFIX)gcc
OBJDUMP = $(PREFIX)objdump
OBJCOPY = $(PREFIX)objcopy
SIZE = $(PREFIX)size
GDB = $(PREFIX)gdb

ECHO = @echo
CP = cp
MKDIR = mkdir -p
RM = rm
RMDIR = rmdir

BUILD_DIR = build
BIN_DIR = bootloader_only_binaries

program: $(BUILD_DIR)/$(TARGET).hex
	$(FLASH) write $(BUILD_DIR)/$(TARGET).bin 0x8000000
.PHONY: all build output info size clean flash

all: $(SRCS) clean gccversion output info size

generic-pb1: $(SRCS) clean gccversion build_generic-pb1 info size

build_generic-pb1: LINKER_SCRIPT=STM32F072C8U8.ld
build_generic-pb1: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(ECHO) "LD      $@"
	$(Q)$(CC) $(LDFLAGS) $(CFLAGS) $(OBJS) -o "$@"

.SECONDARY: $(OBJS)

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(ECHO) "CC      $<"
	$(Q)$(CC) $(CFLAGS) -c "$<" -o "$@"

output: $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(ECHO) "GENHEX  $@"
	$(Q)$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(ECHO) "GENBIN  $@"
	$(Q)$(OBJCOPY) -O binary $< $@

gccversion:
	@$(CC) --version

info: $(BUILD_DIR)/$(TARGET).elf
	$(ECHO) "INFO    $<"
	$(Q)$(OBJDUMP) -x -S $(BUILD_DIR)/$(TARGET).elf > $(BUILD_DIR)/$(TARGET).lst
	$(Q)$(OBJDUMP) -D $(BUILD_DIR)/$(TARGET).elf > $(BUILD_DIR)/$(TARGET).dis
	$(Q)$(SIZE) $(BUILD_DIR)/$(TARGET).elf > $(BUILD_DIR)/$(TARGET).size

size: $(BUILD_DIR)/$(TARGET).elf
	$(ECHO) "SIZE    $<"
	$(Q)$(SIZE) $(BUILD_DIR)/$(TARGET).elf

$(BUILD_DIR):
	$(ECHO) "MKDIR   $@"
	$(Q)$(MKDIR) $@

clean:
	$(ECHO) "CLEAN"
	$(Q)$(RM) -f $(BUILD_DIR)/$(TARGET).elf
	$(Q)$(RM) -f $(BUILD_DIR)/$(TARGET).bin
	$(Q)$(RM) -f $(BUILD_DIR)/$(TARGET).hex
	$(Q)$(RM) -f $(BUILD_DIR)/$(TARGET).size
	$(Q)$(RM) -f $(BUILD_DIR)/$(TARGET).lst
	$(Q)$(RM) -f $(BUILD_DIR)/$(TARGET).dis
	$(Q)$(RM) -f $(BUILD_DIR)/$(TARGET).map
	$(Q)$(RM) -f $(OBJS)
	-$(Q)$(RMDIR) $(BUILD_DIR)

load: 
	JLinkExe -device STM32F072C8 -if SWD -speed 4000 -CommanderScript load.jlink
