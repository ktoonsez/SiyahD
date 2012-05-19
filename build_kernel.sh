#!/bin/sh
export KERNELDIR=`readlink -f .`
export INITRAMFS_SOURCE=`readlink -f $KERNELDIR/../initramfs3`
export PARENT_DIR=`readlink -f ..`
export USE_SEC_FIPS_MODE=true
NAMBEROFCPUS=`grep 'processor' /proc/cpuinfo | wc -l`


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
rm $KERNELDIR/zImage
fi
if [ -e $KERNELDIR/arch/arm/boot/zImage ]; then
rm $KERNELDIR/arch/arm/boot/zImage
fi

#Remove all old modules before compile.
cd $KERNELDIR
OLDMODULES=`find -name '*.ko'`
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
export EXTRA_AFLAGS=-mfpu=neon

#building with latest toolchain with gcc 4.5.2
#export CROSS_COMPILE=$PARENT_DIR/toolchain/bin/arm-none-eabi-

#building with latest CM9 toolchain with gcc 4.4.3
#export CROSS_COMPILE=/media/Source-Code/android/system/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi-

#Build with Linearo 4.7
export CROSS_COMPILE=$PARENT_DIR/linaro/bin/arm-eabi-

cd $KERNELDIR/
nice -n 10 make -j$NAMBEROFCPUS modules || exit 1

#Copy initramfs files to tmp directory
cp -ax $INITRAMFS_SOURCE $INITRAMFS_TMP
#Clear git repositories in initramfs
if [ -e /tmp/initramfs-source/.git ]; then
rm -rf /tmp/initramfs-source/.git
fi
#Remove empty directory placeholders
find $INITRAMFS_TMP -name EMPTY_DIRECTORY -exec rm -rf {} \;
#Remove mercurial repository
if [ -d $INITRAMFS_TMP/.hg ]; then
rm -rf $INITRAMFS_TMP/.hg
fi
#Copy modules into /system/lib/modules for symlink creation.
mkdir -p $INITRAMFS/lib/modules
if [ ! -e /system/lib/modules ]
then
mkdir -p /system/lib/modules/
fi
#Clean $KERNELDIR/READY/system/lib/modules/ from old modules.
rm -rf $KERNELDIR/READY/system/lib/modules/*.ko
rm -rf /system/lib/modules/*.ko 
#Find all new modules in kernel folders an cp them to READY kernel folder
NEWMODULES=`find -name '*.ko'`
for n in $NEWMODULES
do
cp -av $n $KERNELDIR/READY/system/lib/modules/
done
#Strip debug code from modules to reduce size
${CROSS_COMPILE}strip --strip-debug $KERNELDIR/READY/system/lib/modules/*.ko
#Symlink READY kernel folder modules to lib/modules just in case rom read them there.
chmod 755 $KERNELDIR/READY/system/lib/modules/*
#Copy modules to symlink folder
cp $KERNELDIR/READY/system/lib/modules/* /system/lib/modules/
READYMODULES=`ls $KERNELDIR/READY/system/lib/modules/`
for m in $READYMODULES
do
ln -sv /system/lib/modules/$m $INITRAMFS_TMP/lib/modules/
done
#Clean /system/lib/modules/ for next build.
nice -n 10 make -j$NAMBEROFCPUS zImage CONFIG_INITRAMFS_SOURCE="$INITRAMFS_TMP" || exit 1

if [ -e $KERNELDIR/arch/arm/boot/zImage ]; then
$KERNELDIR/mkshbootimg.py $KERNELDIR/zImage $KERNELDIR/arch/arm/boot/zImage $KERNELDIR/payload.tar $KERNELDIR/recovery.tar.xz

#Copy all needed to ready kernel folder.
cp $KERNELDIR/.config $KERNELDIR/arch/arm/configs/dorimanx_defconfig
cp $KERNELDIR/.config $KERNELDIR/READY/
rm $KERNELDIR/READY/boot/zImage
rm $KERNELDIR/READY/Kernel_Dorimanx-SGII-ICS*
stat $KERNELDIR/zImage
GETVER=`grep 'Dorimanx-V' arch/arm/configs/dorimanx_defconfig | cut -c 32-35`
cp $KERNELDIR/zImage /$KERNELDIR/READY/boot/
cd $KERNELDIR/READY/
zip -r Kernel_Dorimanx-SGII-ICS-$GETVER-`date +"Date-%d-%m-Time-%H-%M"`.zip .
else
echo "Kernel STUCK in BUILD! no zImage exist"
fi

