arch := i386 x86-64 arm
goals := $(addprefix pie-,$(arch))
entry := myexec
section = .shellcode
CFLAGS += -std=gnu99
CFLAGS += -Os

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

clean:
	$(RM) *.o *.so

## pie

pie-i386: CFLAGS += -DDEBUG -g3 -m32 -fpie -pie
pie-x86-64: CFLAGS += -DDEBUG -g3 -fpie -pie
pie-arm: CC = $(CC_arm)
pie-arm: CFLAGS += -DDEBUG -g3 -fpie -pie

## shellcode

sc-i386.so: CFLAGS += -fpic
sc-i386.so: LDFLAGS += -Wl,-Bsymbolic
sc-i386.so: main.c
	$(LINK.c) -m32 $< -shared -o $@
sc-i386: sc-i386.so
	objcopy -I elf32-i386 -O binary -j $(section) $< $@
dump-sc-i386: sc-i386
	entry=$$(nm -g $<.so | gawk '/\<$(entry)\>/{print $$1}')
	shellcode=$$(objdump -h $<.o | gawk '/$(section)\>/{print $$6}')
	printf 0x%08x $$[0x$$entry-0x$$shellcode]
	objdump -b binary -m i386 -M intel -D $<

sc-x86-64.o: main.c
	$(LINK.c) $< -c -o $@
sc-x86-64: sc-x86-64.o
	objcopy -I elf64-x86-64 -O binary -j $(section) $< $@
dump-sc-x86-64: sc-x86-64
	objdump -b binary -m i386:x86-64 -D -M intel $<

sc-arm.o: CC = $(CC_arm)
sc-arm.o: main.c
	$(COMPILE.c) $< -c -o $@
sc-arm: sc-arm.o
	$(OBJCOPY_arm) -I elf32-littlearm -O binary -j $(section) $< $@
dump-sc-arm: sc-arm
	$(OBJDUMP_arm) -b binary -m arm -D $<

$(foreach i, $(goals), $(eval $(call def, $i)))
.SUFFIXES:
.PSEUDO: all clean
.ONESHELL:
