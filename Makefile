arch := i386 x86-64 arm
goals := $(addprefix pie-,$(arch))
CFLAGS += -std=gnu99
CFLAGS += -g3

CC_arm := arm-unknown-linux-gnueabi-gcc
OBJDUMP_arm := arm-unknown-linux-gnueabi-objdump
OBJCOPY_arm := arm-unknown-linux-gnueabi-objcopy

# SET THE PATH OF YOUR TOOLCHAIN
sinclude Makefile.local

define def
$1: main.c
	$$(LINK.c) $$^ $$(LDLIBS) -o $$@
endef

all: $(goals)

## pie

pie-i386: CFLAGS += -DDEBUG -m32 -fpie -pie
pie-x86-64: CFLAGS += -DDEBUG -fpie -pie
pie-arm: CC = $(CC_arm)
pie-arm: CFLAGS += -DDEBUG -fpie -pie

## shellcode

sc-i386.o: main.c
	$(COMPILE.c) -m32 $< -c -o $@
sc-i386: sc-i386.o
	objcopy -I elf32-i386 -O binary -j .shellcode $< $@
dump-sc-i386: sc-i386
	objdump -b binary -m i386 -M intel -D $<

sc-x86-64.o: main.c
	$(COMPILE.c) $< -c -o $@
sc-x86-64: sc-x86-64.o
	objcopy -I elf64-x86-64 -O binary -j .shellcode $< $@
dump-sc-x86-64: sc-x86-64
	objdump -b binary -m i386:x86-64 -D -M intel $<

sc-arm.o: CC = $(CC_arm)
sc-arm.o: main.c
	$(COMPILE.c) $< -c -o $@
sc-arm: sc-arm.o
	$(OBJCOPY_arm) -I elf32-littlearm -O binary -j .shellcode $< $@
dump-sc-arm: sc-arm
	$(OBJDUMP_arm) -b binary -m arm -D $<

$(foreach i, $(goals), $(eval $(call def, $i)))
.SUFFIXES:
.PSEUDO: all
