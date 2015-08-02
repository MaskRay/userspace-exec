## defines

arch := i386 x86-64 arm
links := $(addprefix pie-,$(arch)) $(patsubst %,sc-%.so,$(arch))
entry := myexec
section = .shellcode
CFLAGS += -std=gnu99
#CFLAGS += -Os

CC_arm := arm-unknown-linux-gnueabi-gcc
OBJDUMP_arm := arm-unknown-linux-gnueabi-objdump
OBJCOPY_arm := arm-unknown-linux-gnueabi-objcopy

# SET THE PATH OF YOUR TOOLCHAIN
sinclude Makefile.local

define def
$1: main.c
	$$(LINK.c) $$^ $$(LDLIBS) -o $$@
endef

all: $(links)

clean:
	$(RM) *.o *.so $(links)

## pie

pie-i386: CFLAGS += -DDEBUG -g3 -m32 -fpie -pie
pie-x86-64: CFLAGS += -DDEBUG -g3 -fpie -pie
pie-arm: CC = $(CC_arm)
pie-arm: CFLAGS += -DDEBUG -g3 -fpie -pie

## shellcode

sc-i386.so: CFLAGS += -fpic -m32
sc-i386.so: LDFLAGS += -shared -Wl,-Bsymbolic
sc-i386: sc-i386.so
	objcopy -I elf32-i386 -O binary -j $(section) $< $@
dump-sc-i386: sc-i386
	entry=$$(nm -g $<.so | gawk '/\<$(entry)\>/{print $$1}')
	shellcode=$$(objdump -wh $<.so | gawk '/$(section)\>/{print $$4}')
	printf 'entry: 0x%08x\n' $$[0x$$entry-0x$$shellcode]
	#objdump -b binary -m i386 -M intel -D $<

sc-x86-64.so: CFLAGS += -fpic
sc-x86-64.so: LDFLAGS += -shared -Wl,-Bsymbolic
sc-x86-64: sc-x86-64.so
	objcopy -I elf64-x86-64 -O binary -j $(section) $< $@
dump-sc-x86-64: sc-x86-64
	entry=$$(nm -g $<.so | gawk '/\<$(entry)\>/{print $$1}')
	shellcode=$$(objdump -wh $<.so | gawk '/$(section)\>/{print $$4}')
	printf 'entry: 0x%016x\n' $$[0x$$entry-0x$$shellcode]
	#objdump -b binary -m i386:x86-64 -D -M intel $<

sc-arm.so: CC = $(CC_arm)
sc-arm.so: CFLAGS += -fpic
sc-arm.so: LDFLAGS += -shared -Wl,-Bsymbolic
sc-arm: sc-arm.so
	$(OBJCOPY_arm) -I elf32-littlearm -O binary -j $(section) $< $@
dump-sc-arm: sc-arm
	entry=$$(nm -g $<.so | gawk '/\<$(entry)\>/{print $$1}')
	shellcode=$$(objdump -wh $<.so | gawk '/$(section)\>/{print $$4}')
	printf 'entry: 0x%08x\n' $$[0x$$entry-0x$$shellcode]
	#$(OBJDUMP_arm) -b binary -m arm -D $<

## misc

$(foreach i, $(links), $(eval $(call def, $i)))
.SUFFIXES:
.PSEUDO: all clean
.ONESHELL:
