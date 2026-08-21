# SlopOS build system

CC      := gcc
CFLAGS  := -m64 -ffreestanding -fno-stack-protector -fno-pic -fno-pie \
           -nostdlib -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -mno-3dnow \
           -std=gnu11 -Wall -Wextra -Wno-unused-parameter -O2 -g \
           -fno-asynchronous-unwind-tables -fno-builtin
LDFLAGS := -m64 -ffreestanding -nostdlib -static -Wl,--build-id=none \
           -Wl,-z,max-page-size=0x1000

BUILD   := build
SRC     := src
OBJS    := $(BUILD)/boot.o $(BUILD)/kernel.o $(BUILD)/serial.o \
           $(BUILD)/fb.o $(BUILD)/font8x8.o

.PHONY: all iso run clean font

all: iso

font:
	python3 tools/genfont.py src/font8x8.c

$(BUILD)/%.o: $(SRC)/%.S
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(SRC)/%.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/slopos.elf: $(OBJS)
	$(CC) $(LDFLAGS) -T $(SRC)/linker.ld -o $@ $(OBJS)

iso: $(BUILD)/slopos.elf
	rm -rf $(BUILD)/iso
	mkdir -p $(BUILD)/iso/boot/grub
	cp $(BUILD)/slopos.elf $(BUILD)/iso/boot/slopos.elf
	cp grub/grub.cfg $(BUILD)/iso/boot/grub/grub.cfg
	grub-mkrescue -o $(BUILD)/slopos.iso $(BUILD)/iso >/dev/null 2>&1
	@echo "built $(BUILD)/slopos.iso"

run: iso
	./run.sh

clean:
	rm -rf $(BUILD)
