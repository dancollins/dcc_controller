# Dan Collins 2014

#
# PROJECT
#

# Project Name
PROJECT = dcc_controller

# Add project sources
SRCS_C = \
	main.c \
	hal/retarget.c \
	hal/systick.c \
	hal/dcc_hal.c \
	driver/ringbuf.c \
	driver/dcc.c \
	system_stm32f10x.c

SRCS_H =

SRCS_S = \
	startup_stm32f10x_md.s

# Add project include directories here
INCLUDE = src src/hal src/driver


#
# VENDOR
#

# Include the vendor code
ifndef STM_DIR
$(error "STM_DIR not defined.")
endif

STM_SRCS_C = \
	STM32F10x_StdPeriph_Lib_V3.5.0/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c \
	STM32F10x_StdPeriph_Lib_V3.5.0/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c \
	STM32F10x_StdPeriph_Lib_V3.5.0/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_usart.c \
	STM32F10x_StdPeriph_Lib_V3.5.0/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_tim.c \
	STM32F10x_StdPeriph_Lib_V3.5.0/Libraries/STM32F10x_StdPeriph_Driver/src/misc.c

STM_INCLUDE = \
	STM32F10x_StdPeriph_Lib_V3.5.0/Libraries/STM32F10x_StdPeriph_Driver/inc	\
	STM32F10x_StdPeriph_Lib_V3.5.0/Libraries/CMSIS/CM3/CoreSupport \
	STM32F10x_StdPeriph_Lib_V3.5.0/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x

VENDOR_DEFS = \
	-DUSE_STDPERIPH_DRIVER \
	-DSTM32F10X_MD


#
# COMPILER FLAGS
#
ARCH_FLAGS=-mthumb -mcpu=cortex-m3 -mfloat-abi=soft

STARTUP_DEFS=-D__STARTUP_CLEAR_BSS -D__START=main

GC=-Wl,--gc-sections
MAP=-Wl,-Map=output/$(PROJECT).map

CFLAGS=$(ARCH_FLAGS) $(STARTUP_DEFS) $(VENDOR_DEFS)
CFLAGS+=-Wall -Werror -g -std=gnu99
CFLAGS+=-O0 -flto -ffunction-sections -fdata-sections
CFLAGS+=-fno-builtin

CFLAGS += $(addprefix -I, $(INCLUDE))
CFLAGS += $(addprefix -I$(STM_DIR)/, $(STM_INCLUDE))

LDSCRIPTS=-Lsrc -T stm32_flash.ld
LFLAGS=--specs=nano.specs -lc -lnosys $(LDSCRIPTS) $(GC) $(MAP)


OBJS = 	$(addprefix build/, $(SRCS_C:.c=.o)) \
	$(addprefix build/, $(SRCS_S:.s=.o)) \
	$(addprefix build/vendor/, $(STM_SRCS_C:.c=.o)) \
	$(addprefix build/vendor/, $(STM_SRCS_S:.s=.o))


#
# BUILD RULES
#
.PHONY: all clean prog

all: output/$(PROJECT).elf

build/%.o: src/%.c
	$(MKDIR) -p $(dir $@)
	$(CC) $< $(CFLAGS) -c -o $@

build/%.o: src/%.s
	$(MKDIR) -p $(dir $@)
	$(CC) $< $(CFLAGS) -c -o $@

build/vendor/%.o: $(STM_DIR)/%.c
	$(MKDIR) -p $(dir $@)
	$(CC) $< $(CFLAGS) -c -o $@

build/vendor/%.o: $(STM_DIR)/%.s
	$(MKDIR) -p $(dir $@)
	$(CC) $< $(CFLAGS) -c -o $@

output/$(PROJECT).elf: $(OBJS)
	$(MKDIR) -p output
	$(CC) $^ $(CFLAGS) $(LFLAGS) -o $@
	$(OBJCOPY) -O binary output/$(PROJECT).elf output/$(PROJECT).bin
	$(OBJCOPY) -O ihex output/$(PROJECT).elf output/$(PROJECT).hex
	$(OBJDUMP) -St -marm output/$(PROJECT).elf > output/$(PROJECT).lst
	$(SIZE) output/$(PROJECT).elf

clean:
	rm -rf build
	rm -rf output

prog: output/$(PROJECT).elf
	JLinkExe -device stm32f103rb -commanderscript flash.jlink


#
# TOOLCHAIN
#
CC=arm-none-eabi-gcc
GDB=arm-none-eabi-gdb
OBJCOPY=arm-none-eabi-objcopy
OBJDUMP=arm-none-eabi-objdump
SIZE=arm-none-eabi-size
MKDIR=mkdir
