cp .config .config.bkp
make ARCH=arm CROSS_COMPILE=/media/Source-Code/android/system/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi- mrproper
cp .config.bkp .config

