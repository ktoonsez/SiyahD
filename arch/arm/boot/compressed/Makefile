#
# linux/arch/arm/boot/compressed/Makefile
#
# create a compressed vmlinuz image from the original vmlinux
#

# Check GCC version
CROSS_COMPILE   ?= $(CONFIG_CROSS_COMPILE:"%"=%)
GCCVERSION_OLD  := $(shell $(CROSS_COMPILE)gcc --version | cut -d " " -f3 | cut -c3-5 | grep -v 09 | grep -v ee | grep -v en)

GCCVERSION_NEW  := $(shell $(CROSS_COMPILE)gcc --version | cut -d " " -f4 | cut -c1-3 | grep -v Fre | grep -v sof | grep -v for)

ifeq ($(GCCVERSION_OLD),4.3)
include $(srctree)/arch/arm/boot/compressed/Makefile_old_gcc
endif
ifeq ($(GCCVERSION_OLD),4.4)
include $(srctree)/arch/arm/boot/compressed/Makefile_old_gcc
endif
ifeq ($(GCCVERSION_OLD),4.5)
include $(srctree)/arch/arm/boot/compressed/Makefile_old_gcc
endif
ifeq ($(GCCVERSION_NEW),4.6)
include $(srctree)/arch/arm/boot/compressed/Makefile_linaro
endif
ifeq ($(GCCVERSION_NEW),4.7)
include $(srctree)/arch/arm/boot/compressed/Makefile_linaro
endif

