#
# linux/arch/arm/vfp/Makefile
#
# Copyright (C) 2001 ARM Limited
#

# ccflags-y := -DDEBUG
# asflags-y := -DDEBUG

#EXTRA_AFLAGS	=-mfpu=neon
KBUILD_AFLAGS  :=$(KBUILD_AFLAGS:-mhard-float=-Wa,-mfpu=vfp3)
#KBUILD_AFLAGS  :=$(KBUILD_AFLAGS:-msoft-float=-Wa,-mfpu=neon)
LDFLAGS		   +=--no-warn-mismatch

obj-y				+= vfp.o

vfp-$(CONFIG_VFP)	+= vfpmodule.o entry.o vfphw.o vfpsingle.o vfpdouble.o
