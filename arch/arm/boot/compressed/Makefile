#
# linux/arch/arm/boot/compressed/Makefile
#
# create a compressed vmlinuz image from the original vmlinux
#

# Check GCC version
GCCVERSION  := $(shell $(CC) --version | grep ^gcc | sed 's/^.* //g' | cut -c1-3)

ifeq ($(GCCVERSION),4.3)
include $(srctree)/arch/arm/boot/compressed/Makefile_old_gcc
endif
ifeq ($(GCCVERSION),4.4)
include $(srctree)/arch/arm/boot/compressed/Makefile_old_gcc
endif
ifeq ($(GCCVERSION),4.5)
include $(srctree)/arch/arm/boot/compressed/Makefile_old_gcc
endif
ifeq ($(GCCVERSION),4.6)
include $(srctree)/arch/arm/boot/compressed/Makefile_linaro
endif
ifeq ($(GCCVERSION),4.7)
include $(srctree)/arch/arm/boot/compressed/Makefile_linaro
endif

