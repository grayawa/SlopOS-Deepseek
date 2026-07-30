# SlopOS Build System
# SPDX-License-Identifier: 0BSD

ARCH := x86_64
TARGET := $(ARCH)-elf

CC := gcc
LD := ld
OBJCOPY := objcopy

KERNEL_SRC := kernel/src
BUILD_DIR := build

CFLAGS := \
	-m64 \
	-mcmodel=kernel \
	-mno-red-zone \
	-mno-mmx \
	-mno-sse \
	-mno-sse2 \
	-ffreestanding \
	-fno-stack-protector \
	-fno-pic \
	-fno-pie \
	-mno-80387 \
	-mgeneral-regs-only \
	-ggdb \
	-O2 \
	-Wall -Wextra \
	-Wno-unused-parameter \
	-Ikernel \
	-nostdlib \
	-fno-common

LDFLAGS := -nostdlib \
	-static \
	-z max-page-size=0x1000 \
	-T kernel/linker.ld

KERNEL_OBJS := $(BUILD_DIR)/main.o $(BUILD_DIR)/gdt.o $(BUILD_DIR)/idt.o $(BUILD_DIR)/isr.o $(BUILD_DIR)/pmm.o $(BUILD_DIR)/mouse.o $(BUILD_DIR)/wm.o
KERNEL_ELF := $(BUILD_DIR)/slopos.elf

LIMINE_DIR := limine-binary
LIMINE_DEPLOY := $(LIMINE_DIR)/limine
ISO := slopos.iso
ISO_DIR := $(BUILD_DIR)/iso_root

.PHONY: all iso run clean

all: $(KERNEL_ELF)

$(BUILD_DIR)/%.o: $(KERNEL_SRC)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(KERNEL_SRC)/%.S | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_ELF): $(KERNEL_OBJS) kernel/linker.ld
	$(LD) $(LDFLAGS) $(KERNEL_OBJS) -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(ISO_DIR):
	mkdir -p $(ISO_DIR)/boot

iso: $(KERNEL_ELF) $(ISO_DIR)
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/slopos.elf
	mkdir -p $(ISO_DIR)/boot/limine
	mkdir -p $(ISO_DIR)/limine
	cp kernel/limine.conf $(ISO_DIR)/boot/limine/limine.conf
	cp kernel/limine.conf $(ISO_DIR)/boot/limine.conf
	cp kernel/limine.conf $(ISO_DIR)/limine/limine.conf
	cp kernel/limine.conf $(ISO_DIR)/limine.conf
	cp $(LIMINE_DIR)/limine-bios.sys $(ISO_DIR)/boot/limine/
	cp $(LIMINE_DIR)/limine-bios.sys $(ISO_DIR)/boot/
	cp $(LIMINE_DIR)/limine-bios.sys $(ISO_DIR)/
	cp $(LIMINE_DIR)/limine-bios.sys $(ISO_DIR)/limine/
	cp $(LIMINE_DIR)/limine-bios-cd.bin $(ISO_DIR)/boot/limine/
	cp $(LIMINE_DIR)/limine-uefi-cd.bin $(ISO_DIR)/boot/limine/
	mkdir -p $(ISO_DIR)/EFI/BOOT
	cp $(LIMINE_DIR)/BOOTX64.EFI $(ISO_DIR)/EFI/BOOT/
	xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image \
		--protective-msdos-label \
		$(ISO_DIR) -o $(ISO) 2>&1
	$(LIMINE_DIR)/limine bios-install $(ISO) 2>&1 || true

run: iso
	qemu-system-x86_64 -cdrom $(ISO) -m 512M -no-reboot -serial stdio 2>&1 | head -200

run-gui: iso
	qemu-system-x86_64 -cdrom $(ISO) -m 1G -no-reboot -serial file:serial.log &
	sleep 2
	echo "QEMU started, serial output in serial.log"
	echo "Connect with VNC: vncviewer :5900 or check QEMU window"

run-debug: iso
	qemu-system-x86_64 -cdrom $(ISO) -m 512M -serial stdio -no-reboot -s -S &
	sleep 1
	echo "Connect with: gdb -ex 'target remote :1234'"

clean:
	rm -rf $(BUILD_DIR) $(ISO)
