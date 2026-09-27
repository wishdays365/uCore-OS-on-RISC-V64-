PROJ	:= lab2
EMPTY	:=
SPACE	:= $(EMPTY) $(EMPTY)
SLASH	:= /

V       := @

GCCPREFIX := /home/chao/sw-lab/swgcc710-cross-6b-9916/usr/bin/sw_64sw6b-sunway-linux-gnu-

export LD_LIBRARY_PATH := /home/chao/sw-lab/swgcc710-cross-6b-9916/usr/lib_for_gcc/lib:$(LD_LIBRARY_PATH)

ifndef QEMU
QEMU := /home/chao/sw-lab/qemu-bin/qemu-system-sw64
endif

.SUFFIXES: .c .S .h
.DELETE_ON_ERROR:

HOSTCC		:= gcc
HOSTCFLAGS	:= -Wall -O2

GDB		:= $(GCCPREFIX)gdb
CC		:= $(GCCPREFIX)gcc
CFLAGS  := -std=gnu99 -Wno-unused -Werror
CFLAGS	+= -fno-builtin -Wall -O2 -nostdinc $(DEFS)
CFLAGS	+= -fno-stack-protector -ffunction-sections -fdata-sections
CTYPE	:= c S

LD      := $(GCCPREFIX)ld
LDFLAGS	:= -m elf64sw_64
LDFLAGS	+= -nostdlib --gc-sections

OBJCOPY := $(GCCPREFIX)objcopy
OBJDUMP := $(GCCPREFIX)objdump

COPY	:= cp
MKDIR   := mkdir -p
MV	:= mv
RM	:= rm -f
AWK	:= awk
SED	:= sed
SH	:= sh
TR	:= tr
TOUCH	:= touch -c

OBJDIR	:= obj
BINDIR	:= bin

include tools/function.mk

listf_cc = $(call listf,$(1),$(CTYPE))
add_files_cc = $(call add_files,$(1),$(CC),$(CFLAGS) $(3),$(2),$(4))
create_target_cc = $(call create_target,$(1),$(2),$(3),$(CC),$(CFLAGS))

add_files_host = $(call add_files,$(1),$(HOSTCC),$(HOSTCFLAGS),$(2),$(3))
create_target_host = $(call create_target,$(1),$(2),$(3),$(HOSTCC),$(HOSTCFLAGS))

cgtype = $(patsubst %.$(2),%.$(3),$(1))
objfile = $(call toobj,$(1))
asmfile = $(call cgtype,$(call toobj,$(1)),o,asm)
symfile = $(call cgtype,$(call toobj,$(1)),o,sym)

match = $(shell echo $(2) | $(AWK) '{for(i=1;i<=NF;i++){if(match("$(1)","^"$$(i)"$$")){exit 1;}}}'; echo $$?)

# include libs
INCLUDE	+= libs/
CFLAGS	+= $(addprefix -I,$(INCLUDE))
LIBDIR	+= libs
$(call add_files_cc,$(call listf_cc,$(LIBDIR)),libs,)

# kernel
KINCLUDE	+= kern/debug/ kern/driver/ kern/trap/ kern/mm/ kern/arch/
KSRCDIR		+= kern/init kern/libs kern/debug kern/driver kern/trap kern/mm
KCFLAGS		+= $(addprefix -I,$(KINCLUDE))
$(call add_files_cc,$(call listf_cc,$(KSRCDIR)),kernel,$(KCFLAGS))

KOBJS	= $(call read_packet,kernel libs)
KOBJS += libs/__divlu.o libs/__remlu.o libs/__divwu.o libs/__remwu.o

kernel = $(call totarget,kernel)
$(kernel): tools/kernel.ld
$(kernel): $(KOBJS)
	@echo + ld $@
	$(V)$(LD) $(LDFLAGS) -T tools/kernel.ld -o $@ $(KOBJS)
	@$(OBJDUMP) -S $@ > $(call asmfile,kernel)

$(call create_target,kernel)

UCOREIMG	:= $(call totarget,ucore.img)
$(UCOREIMG): $(kernel)
	$(OBJCOPY) $(kernel) --strip-all -O binary $@
$(call create_target,ucore.img)

$(call finish_all)

IGNORE_ALLDEPS	= clean dist-clean grade touch print-.+ handin
ifeq ($(call match,$(MAKECMDGOALS),$(IGNORE_ALLDEPS)),0)
-include $(ALLDEPS)
endif

TARGETS: $(TARGETS)
.DEFAULT_GOAL := TARGETS

.PHONY: qemu
qemu: $(kernel)
	$(V)$(QEMU) -machine core3 -smp 1 -m 512m \
		-kernel ./bin/kernel -monitor stdio

.PHONY: clean
clean:
	-$(RM) -r $(OBJDIR) $(BINDIR)
