#!/bin/sh
export KERNELDIR=`readlink -f .`
export INITRAMFS_SOURCE=`readlink -f $KERNELDIR/../initramfs3`
export PARENT_DIR=`readlink -f ..`
export USE_SEC_FIPS_MODE=true

if [ "${1}" != "" ];then
	export KERNELDIR=`readlink -f ${1}`
fi

INITRAMFS_TMP="/tmp/initramfs-source"

if [ ! -f $KERNELDIR/.config ];
then
	make voku_defconfig
fi

. $KERNELDIR/.config

#Remove previous zImage files
if [ -e $KERNELDIR/zImage ]; then
	rm $KERNELDIR/zImage;
fi
if [ -e $KERNELDIR/arch/arm/boot/zImage ]; then
	rm $KERNELDIR/arch/arm/boot/zImage;
fi

#Remove all old modules before compile.
cd $KERNELDIR
OLDMODULES=`find -name *.ko`
for i in $OLDMODULES
do
	rm -f $i;
done

#remove previous initramfs files
if [ -e $INITRAMFS_TMP ]; then
	echo "removing old temp iniramfs";
	rm -rf $INITRAMFS_TMP;
fi
if [ -f /tmp/cpio* ]; then
	echo "removing old temp iniramfs_tmp.cpio";
	rm -rf /tmp/cpio*;
fi

#Clean initramfs old compile data
rm -f usr/initramfs_data.cpio
rm -f usr/initramfs_data.o

export ARCH="arm"
# export EXTRA_AFLAGS=-mfpu=neon
# default gcc
#export CROSS_COMPILE="/usr/bin/arm-linux-gnueabi-"
# use linaro compiler
# export CROSS_COMPILE="~/sgs2/android-toolchain-eabi/bin/arm-eabi-"
# use codesourcery compiler
export CROSS_COMPILE="~/sgs2/android-toolchain-eabi2/bin/arm-none-eabi-"

cd $KERNELDIR/
nice -n 10 make -j8 modules || exit 1

#copy initramfs files to tmp directory
cp -ax $INITRAMFS_SOURCE $INITRAMFS_TMP
#clear git repositories in initramfs
if [ -e $INITRAMFS_TMP/.git ]; then
	find $INITRAMFS_TMP -name .git -exec rm -rf {} \;
fi
#remove empty directory placeholders
find $INITRAMFS_TMP -name EMPTY_DIRECTORY -exec rm -rf {} \;
#remove mercurial repository
if [ -d $INITRAMFS_TMP/.hg ]; then
	rm -rf $INITRAMFS_TMP/.hg;
fi
#copy modules into initramfs
mkdir -p $INITRAMFS/lib/modules
find -name '*.ko' -exec cp -av {} $INITRAMFS_TMP/lib/modules/ \;
# /usr/bin/arm-linux-gnueabi-strip --strip-debug $INITRAMFS_TMP/lib/modules/*.ko
/root/sgs2/android-toolchain-eabi2/bin/arm-none-eabi-strip --strip-debug $INITRAMFS_TMP/lib/modules/*.ko
chmod 755 $INITRAMFS_TMP/lib/modules/*
nice -n 10 make -j8 zImage CONFIG_INITRAMFS_SOURCE="$INITRAMFS_TMP" || exit 1

if [ -e $KERNELDIR/arch/arm/boot/zImage ]; then
	$KERNELDIR/mkshbootimg.py $KERNELDIR/zImage $KERNELDIR/arch/arm/boot/zImage $KERNELDIR/payload.tar $KERNELDIR/recovery.tar.xz

	#Copy all needed to ready kernel folder.
	cp $KERNELDIR/.config $KERNELDIR/arch/arm/configs/voku_defconfig
	cp $KERNELDIR/.config $KERNELDIR/READY/
	rm $KERNELDIR/READY/boot/zImage
	rm $KERNELDIR/READY/Kernel_Voku-SGII-ICS*
	stat $KERNELDIR/zImage
	cp $KERNELDIR/zImage /$KERNELDIR/READY/boot/
	cd $KERNELDIR/READY/
	zip -r Kernel_Voku-SGII-ICS.`date +"%H-%M-%d%m%y"`.zip .
else
	echo "Kernel STUCK in BUILD! no zImage exist"
fi

