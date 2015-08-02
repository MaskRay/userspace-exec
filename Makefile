arch := i386 x86-64 arm
goals := $(addprefix pie-,$(arch))
CFLAGS += -std=gnu99
CFLAGS += -g3

CC_arm := arm-unknown-linux-gnueabi-gcc
sinclude Makefile.local

define def
$1: main.c
	$$(LINK.c) $$^ $$(LDLIBS) -o $$@
endef

all: $(goals)

pie-i386: CFLAGS += -m32 -fpie -pie
pie-x86-64: CFLAGS += -fpie -pie
pie-arm: CC = $(CC_arm)
pie-arm: CFLAGS += -fpie -pie

$(foreach i, $(goals), $(eval $(call def, $i)))
.SUFFIXES:
.PSEUDO: all
