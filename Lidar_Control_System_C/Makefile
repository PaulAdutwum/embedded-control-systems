#==============================================================================
# Makefile for Autonomous LIDAR Control System
# Target: AVR ATmega328P (Arduino Uno/Nano)
#==============================================================================

# Project name
PROJECT = lidar_control

# MCU settings
MCU = atmega328p
F_CPU = 16000000UL
BAUD = 115200

# Programmer settings (for Arduino Uno)
PROGRAMMER = arduino
PORT = /dev/cu.usbmodem*

# Directories
SRC_DIR = src
DRV_DIR = drivers
INC_DIR = include
BUILD_DIR = build

# Source files
SOURCES = $(SRC_DIR)/main.c \
          $(SRC_DIR)/hal.c \
          $(SRC_DIR)/state_machine.c \
          $(DRV_DIR)/uart.c \
          $(DRV_DIR)/pwm.c \
          $(DRV_DIR)/sensor.c

# Object files
OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)

# Compiler and tools
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
AVRDUDE = avrdude

# Compiler flags
CFLAGS = -mmcu=$(MCU) \
         -DF_CPU=$(F_CPU) \
         -Os \
         -Wall \
         -Wextra \
         -Werror \
         -std=c99 \
         -ffunction-sections \
         -fdata-sections \
         -I$(INC_DIR)

# Linker flags
LDFLAGS = -mmcu=$(MCU) \
          -Wl,--gc-sections \
          -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map

# Debug build
ifdef DEBUG
CFLAGS += -g -DDEBUG
endif

#==============================================================================
# Build rules
#==============================================================================

.PHONY: all clean flash size disasm

all: $(BUILD_DIR)/$(PROJECT).hex size

# Create build directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)/$(SRC_DIR)
	mkdir -p $(BUILD_DIR)/$(DRV_DIR)

# Compile source files
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Link object files
$(BUILD_DIR)/$(PROJECT).elf: $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@

# Generate hex file
$(BUILD_DIR)/$(PROJECT).hex: $(BUILD_DIR)/$(PROJECT).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

# Flash to Arduino
flash: $(BUILD_DIR)/$(PROJECT).hex
	$(AVRDUDE) -p $(MCU) -c $(PROGRAMMER) -P $(PORT) -b $(BAUD) -U flash:w:$<:i

# Display size information
size: $(BUILD_DIR)/$(PROJECT).elf
	@echo ""
	@echo "=== Memory Usage ==="
	$(SIZE) --mcu=$(MCU) --format=avr $<
	@echo ""

# Generate disassembly
disasm: $(BUILD_DIR)/$(PROJECT).elf
	$(OBJDUMP) -d -S $< > $(BUILD_DIR)/$(PROJECT).lst

# Clean build files
clean:
	rm -rf $(BUILD_DIR)

#==============================================================================
# Simulation build (for testing on host)
#==============================================================================

.PHONY: sim sim-clean

SIM_CC = gcc
SIM_CFLAGS = -Wall -Wextra -g -DSIMULATION -I$(INC_DIR)
SIM_BUILD = build_sim

sim: $(SIM_BUILD)/$(PROJECT)_sim
	@echo "Simulation build complete: $(SIM_BUILD)/$(PROJECT)_sim"

$(SIM_BUILD):
	mkdir -p $(SIM_BUILD)

$(SIM_BUILD)/$(PROJECT)_sim: $(SOURCES) | $(SIM_BUILD)
	$(SIM_CC) $(SIM_CFLAGS) $^ -o $@ -lm

sim-clean:
	rm -rf $(SIM_BUILD)

#==============================================================================
# Help
#==============================================================================

.PHONY: help

help:
	@echo ""
	@echo "╔══════════════════════════════════════════════════════════════╗"
	@echo "║           LIDAR Control System - Build System                ║"
	@echo "╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build the project (default)"
	@echo "  flash    - Upload to Arduino"
	@echo "  size     - Show memory usage"
	@echo "  disasm   - Generate disassembly listing"
	@echo "  clean    - Remove build files"
	@echo "  sim      - Build for host simulation"
	@echo "  sim-clean- Clean simulation build"
	@echo "  help     - Show this help"
	@echo ""
	@echo "Configuration:"
	@echo "  MCU      = $(MCU)"
	@echo "  F_CPU    = $(F_CPU)"
	@echo "  PORT     = $(PORT)"
	@echo ""

