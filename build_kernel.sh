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
  make dorimanx_defconfig
fi

. $KERNELDIR/.config

#Remove previous zImage files
if [ -e $KERNELDIR/zImage ]; then
rm $KERNELDIR/zImage
rm $KERNELDIR/arch/arm/boot/zImage
fi

#Remove all old modules before compile.
cd $KERNELDIR
OLDMODULES=`find -name *.ko`
for i in $OLDMODULES
do
rm -f $i
done

#Remove previous initramfs files
if [ -e $INITRAMFS_TMP ]; then
echo "removing old temp iniramfs"
rm -rf $INITRAMFS_TMP
fi
if [ -f /tmp/cpio* ]; then
echo "removing old temp iniramfs_tmp.cpio"
rm -rf /tmp/cpio*
fi

#Clean initramfs old compile data
rm -f usr/initramfs_data.cpio
rm -f usr/initramfs_data.o

export ARCH=arm
#I am building with latest toolchain with gcc 4.5.2
export CROSS_COMPILE=$PARENT_DIR/toolchain/bin/arm-none-eabi-

cd $KERNELDIR/
nice -n 10 make -j8 modules || exit 1

#Copy initramfs files to tmp directory
cp -ax $INITRAMFS_SOURCE $INITRAMFS_TMP
#Clear git repositories in initramfs
if [ -e $INITRAMFS_TMP/.git ]; then
find $INITRAMFS_TMP -name .git -exec rm -rf {} \;
fi
#Remove empty directory placeholders
find $INITRAMFS_TMP -name EMPTY_DIRECTORY -exec rm -rf {} \;
#Remove mercurial repository
if [ -d $INITRAMFS_TMP/.hg ]; then
rm -rf $INITRAMFS_TMP/.hg
fi
#Copy modules into initramfs
mkdir -p $INITRAMFS/lib/modules
find -name '*.ko' -exec cp -av {} $INITRAMFS_TMP/lib/modules/ \;
${CROSS_COMPILE}strip --strip-debug $INITRAMFS_TMP/lib/modules/*.ko
chmod 755 $INITRAMFS_TMP/lib/modules/*
nice -n 10 make -j8 zImage CONFIG_INITRAMFS_SOURCE="$INITRAMFS_TMP" || exit 1

if [ -e $KERNELDIR/arch/arm/boot/zImage ]; then
$KERNELDIR/mkshbootimg.py $KERNELDIR/zImage $KERNELDIR/arch/arm/boot/zImage $KERNELDIR/payload.tar $KERNELDIR/recovery.tar.xz

#Copy all needed to ready kernel folder.
cp $KERNELDIR/.config $KERNELDIR/arch/arm/configs/dorimanx_defconfig
cp $KERNELDIR/.config $KERNELDIR/READY/
rm $KERNELDIR/READY/boot/zImage
rm $KERNELDIR/READY/Kernel_Dorimanx-SGII-ICS*
stat $KERNELDIR/zImage
cp $KERNELDIR/zImage /$KERNELDIR/READY/boot/
cd $KERNELDIR/READY/
zip -r Kernel_Dorimanx-SGII-ICS-`date +"Date-%d-%m-%y-Time-%H-%M"`.zip .
else
echo "Kernel STUCK in BUILD! no zImage exist"
fi

