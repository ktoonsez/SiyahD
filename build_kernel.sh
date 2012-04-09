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
  make siyah_defconfig
fi

. $KERNELDIR/.config

#remove previous zImage files
if [ -e $KERNELDIR/zImage ]; then
rm $KERNELDIR/zImage
rm $KERNELDIR/arch/arm/boot/zImage
fi

export ARCH="arm"
export CROSS_COMPILE="/usr/bin/arm-linux-gnueabi-"

cd $KERNELDIR/
nice -n 10 make -j8 || exit 1

#remove previous initramfs files
if [ -d $INITRAMFS_TMP ]; then
rm -rf $INITRAMFS_TMP
fi
if [ -e $INITRAMFS_TMP.cpio ]; then
rm -rf $INITRAMFS_TMP.cpio
fi
#copy initramfs files to tmp directory
cp -ax $INITRAMFS_SOURCE $INITRAMFS_TMP
#clear git repositories in initramfs
if [ -d $INITRAMFS_TMP/.git ]; then
find $INITRAMFS_TMP -name .git -exec rm -rf {} \;
fi
#remove empty directory placeholders
find $INITRAMFS_TMP -name EMPTY_DIRECTORY -exec rm -rf {} \;
#remove mercurial repository
if [ -d $INITRAMFS_TMP/.hg ]; then
rm -rf $INITRAMFS_TMP/.hg
fi
#copy modules into initramfs
mkdir -p $INITRAMFS/lib/modules
find -name '*.ko' -exec cp -av {} $INITRAMFS_TMP/lib/modules/ \;

nice -n 10 make -j3 zImage CONFIG_INITRAMFS_SOURCE="$INITRAMFS_TMP" || exit 1

if [ -e $KERNELDIR/arch/arm/boot/zImage ]; then
cp $KERNELDIR/arch/arm/boot/zImage zImage
#$KERNELDIR/mkshbootimg.py $KERNELDIR/zImage $KERNELDIR/arch/arm/boot/zImage $KERNELDIR/../payload.cpio $KERNELDIR/../recovery.cpio.xz

#Copy all needed to ready kernel folder.
cp $KERNELDIR/.config $KERNELDIR/arch/arm/configs/siyah_defconfig
cp $KERNELDIR/.config $KERNELDIR/READY/
rm $KERNELDIR/READY/boot/zImage
rm $KERNELDIR/READY/Kernel_Voku-SGII-ICS-V*
cp $KERNELDIR/zImage /$KERNELDIR/READY/boot/
cd $KERNELDIR/READY/
zip -r Kernel_Voku-SGII-ICS-V1.`date +"%H-%M-%d%m%y"`.zip .
else
echo "Kernel STUCK in BUILD! no zImage exist"
fi

