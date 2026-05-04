###############################################################################
# Makefile for Wolfenduino Uzebox C port
#
# Place/build from:
#   demos/Wolfenduino/default/
#
# Source layout expected:
#   ../Game/*.c
#   ../Game/*.h
#   ../DataHeaders/*.h
###############################################################################

## General Flags
PROJECT = Wolfenduino
MCU = atmega644
TARGET = $(PROJECT).elf
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
INFO = ../gameinfo.properties

## Escape spaces in mixer path (due to custom sounds.inc)
EMPTY:=
SPACE:= $(EMPTY) $(EMPTY)
SPACE_ESC:= \\$(SPACE)
MIX_PATH:= $(realpath ../DataHeaders/sounds.inc)
MIX_PATH_ESC:= $(subst $(SPACE),$(SPACE_ESC),$(MIX_PATH))


## Kernel settings
KERNEL_DIR = ../../../kernel
UZEBIN_DIR = ../../../bin
KERNEL_OPTIONS  = -DVIDEO_MODE=23 -DINTRO_LOGO=0
KERNEL_OPTIONS += -DPLATFORM_UZEBOX=1 -DPLATFORM_UZEBOX_MODE23=1
KERNEL_OPTIONS += -DWOLF3D_ENABLE_BOSS=$(WOLF3D_BOSS)
WOLF3D_SOUND ?= 1
WOLF3D_BOSS ?= 0
ifeq ($(WOLF3D_SOUND),1)
KERNEL_OPTIONS += -DSOUND_MIXER=1 -DMUSIC_ENGINE=STREAM -DSOUND_CHANNEL_5_ENABLE=1 -DMIXER_WAVES=\"$(MIX_PATH_ESC)\"
else
KERNEL_OPTIONS += -DWOLF3D_ENABLE_UZEBOX_SFX=0
endif

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -Wextra -Wno-unused-parameter -gdwarf-2 -std=gnu99
CFLAGS += -DF_CPU=28636360UL -Os -fsigned-char
CFLAGS += -ffunction-sections -fdata-sections -fno-toplevel-reorder
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d
CFLAGS += $(KERNEL_OPTIONS)

## Assembly specific flags
ASMFLAGS = $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS += -Wl,-Map=$(PROJECT).map
LDFLAGS += -Wl,-gc-sections

## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings

## Kernel objects
KERNEL_OBJECTS = \
	uzeboxVideoEngineCore.o \
	uzeboxCore.o \
	uzeboxSoundEngine.o \
	uzeboxSoundEngineCore.o \
	uzeboxVideoEngine.o

## Game objects
GAME_OBJECTS = \
	WolfenduinoUzebox.o \
	Platform.o \
	Engine.o \
	FixedMath.o \
	Menu.o \
	Data_GUI.o \
	Map.o \
	Actor.o \
	Player.o \
	Renderer.o

## Objects that must be built in order to link
OBJECTS = $(KERNEL_OBJECTS) $(GAME_OBJECTS)

## Objects explicitly added by the user
LINKONLYOBJECTS =

## Include Directories
INCLUDES = -I"$(KERNEL_DIR)" -I"../Game" -I"../DataHeaders"

## Resources
RESOURCES = ../DataHeaders/e1m1-uncompressed.inc ../DataHeaders/music-compressed.inc

## Build
all: $(RESOURCES) $(TARGET) $(PROJECT).hex $(PROJECT).eep $(PROJECT).lss $(PROJECT).uze size

## Compile Kernel files
uzeboxVideoEngineCore.o: $(KERNEL_DIR)/uzeboxVideoEngineCore.s
	$(CC) $(INCLUDES) $(ASMFLAGS) -c $< -o $@

uzeboxSoundEngineCore.o: $(KERNEL_DIR)/uzeboxSoundEngineCore.s
	$(CC) $(INCLUDES) $(ASMFLAGS) -c $< -o $@

uzeboxCore.o: $(KERNEL_DIR)/uzeboxCore.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

uzeboxSoundEngine.o: $(KERNEL_DIR)/uzeboxSoundEngine.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

uzeboxVideoEngine.o: $(KERNEL_DIR)/uzeboxVideoEngine.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

## Compile game sources from ../Game into this ./default build directory
Data_GUI.o: ../Game/Data_GUI.c ../DataHeaders/Data_GUI.h ../Game/Defines.h
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

Menu.o: ../Game/Menu.c ../DataHeaders/Data_GUI.h ../Game/Defines.h
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

%.o: ../Game/%.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

../DataHeaders/e1m1-uncompressed.inc: ../DataHeaders/e1m1.mid
	$(UZEBIN_DIR)/midiconv -f 15 ../DataHeaders/e1m1.mid ../DataHeaders/e1m1-uncompressed.inc -ve1m1_mus


MIDI_UNCOMPRESSED_LIST = ../DataHeaders/e1m1-uncompressed.inc

##Rebuild compressed music resources
../DataHeaders/music-compressed.inc: $(MIDI_UNCOMPRESSED_LIST)
	$(UZEBIN_DIR)/mconvert ../DataHeaders/music-compression.cfg


## Link
$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	$(OBJCOPY) -O ihex $(HEX_FLASH_FLAGS) $< $@

%.eep: $(TARGET)
	-$(OBJCOPY) $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(TARGET)
	$(OBJDUMP) -h -S $< > $@

%.uze: $(PROJECT).hex $(INFO)
	-$(UZEBIN_DIR)/packrom $(PROJECT).hex $@ $(INFO)

UNAME := $(shell sh -c 'uname -s 2>/dev/null || echo not')
AVRSIZEFLAGS := -A ${TARGET}
ifneq (,$(findstring MINGW,$(UNAME)))
AVRSIZEFLAGS := -C --mcu=${MCU} ${TARGET}
endif

size: ${TARGET}
	@echo
	@$(SIZE) -C --mcu=${MCU} ${TARGET}

## Clean target
.PHONY: all clean size
clean:
	-rm -rf $(OBJECTS) $(PROJECT).* dep/* *.uze

## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)
