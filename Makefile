## defines

arch := i386 x86-64 arm mipsel
pie_goals := $(addprefix pie-,$(arch))
dyn_goals := $(addprefix dyn-,$(arch))
shellcode_goals := $(patsubst %,shellcode-%,$(arch))
test_goals := tests/dl tests/shellcode
goals := $(pie_goals) $(dyn_goals) $(shellcode_goals) $(test_goals)
link_targets := $(pie_goals) $(dyn_goals) $(patsubst %,shellcode-%.exe,$(arch))
entry := myexec
section = .shellcode
CFLAGS += -std=gnu99 -I.
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

## pseudo

all: $(goals);
pie: $(pie_goals);
shared: $(shared_goals);
shellcode: $(shellcode_goals);
test: $(test_goals);

clean:
	$(RM) $(link_targets) shellcode-*

## pie

pie-i386: CFLAGS += -DDEBUG -g3 -m32 -fpie -pie
pie-x86-64: CFLAGS += -DDEBUG -g3 -fpie -pie
pie-arm: CC = $(CC_arm)
pie-arm: CFLAGS += -DDEBUG -g3 -fpie -pie
pie-mipsel: CC = $(CC_mipsel)
pie-mipsel: CFLAGS += -DDEBUG -g3 -fpie -pie

## standalone executable dyn (./dyn-x86-64 will load /tmp/.rxc/a)

dyn-i386: CFLAGS += -nostdlib -Wl,-Bsymbolic,-emain -g3 -m32 -fpic -shared
dyn-x86-64: CFLAGS += -nostdlib -Wl,-Bsymbolic,-emain -g3 -fpic -shared
dyn-arm: CC = $(CC_arm)
dyn-arm: CFLAGS += -nostdlib -Wl,-Bsymbolic,-emain -g3 -fpic -shared
dyn-mipsel: CC = $(CC_mipsel)
dyn-mipsel: CFLAGS += -nostdlib -Wl,-Bsymbolic,-emain -g3 -fpic -shared

## shellcode

shellcode-i386.exe: CFLAGS += -m32
shellcode-i386.exe: LDFLAGS += -Wl,-Bsymbolic
shellcode-i386: shellcode-i386.exe
	objcopy -I elf32-i386 -O binary -j $(section) $< $@
dump-shellcode-i386: shellcode-i386
	entry=$$(nm -g $<.exe | gawk '/\<$(entry)\>/{print $$1}')
	shellcode=$$(objdump -wh $<.exe | gawk '/$(section)\>/{print $$4}')
	printf 'entry: 0x%08x\n' $$[0x$$entry-0x$$shellcode]
	#objdump -b binary -m i386 -M intel -D $<

shellcode-x86-64.exe: CFLAGS +=
shellcode-x86-64.exe: LDFLAGS += -Wl,-Bsymbolic
shellcode-x86-64: shellcode-x86-64.exe
	objcopy -I elf64-x86-64 -O binary -j $(section) $< $@
dump-shellcode-x86-64: shellcode-x86-64
	entry=$$(nm -g $<.exe | gawk '/\<$(entry)\>/{print $$1}')
	shellcode=$$(objdump -wh $<.exe | gawk '/$(section)\>/{print $$4}')
	printf 'entry: 0x%016x\n' $$[0x$$entry-0x$$shellcode]
	#objdump -b binary -m i386:x86-64 -D -M intel $<

shellcode-arm.exe: CC = $(CC_arm)
shellcode-arm.exe: CFLAGS += -mthumb
shellcode-arm.exe: LDFLAGS += -Wl,-Bsymbolic
shellcode-arm: shellcode-arm.exe
	$(OBJCOPY_arm) -I elf32-littlearm -O binary -j $(section) $< $@
dump-shellcode-arm: shellcode-arm
	entry=$$(nm -g $<.exe | gawk '/\<$(entry)\>/{print $$1}')
	shellcode=$$(objdump -wh $<.exe | gawk '/$(section)\>/{print $$4}')
	printf 'entry: 0x%08x\n' $$[0x$$entry-0x$$shellcode]
	#$(OBJDUMP_arm) -b binary -m arm -D $<

shellcode-mips.exe: CC = $(CC_mips)
shellcode-mips.exe: CFLAGS += -mthumb
shellcode-mips.exe: LDFLAGS += -Wl,-Bsymbolic
shellcode-mips: shellcode-mips.exe
	$(OBJCOPY_mips) -I elf32-tradlittlemips -O binary -j $(section) $< $@
dump-shellcode-mips: shellcode-mips
	entry=$$(nm -g $<.exe | gawk '/\<$(entry)\>/{print $$1}')
	shellcode=$$(objdump -wh $<.exe | gawk '/$(section)\>/{print $$4}')
	printf 'entry: 0x%08x\n' $$[0x$$entry-0x$$shellcode]
	#$(OBJDUMP_mips) -b binary -m mips -D $<

## test

tests/dl: LDLIBS += -g3 -ldl

## misc

$(foreach i, $(link_targets), $(eval $(call def, $i)))
.PSEUDO: all clean pie shared shellcode test
.ONESHELL:
