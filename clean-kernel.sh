cp .config .config.bkp;
make ARCH=arm CROSS_COMPILE=linaro-12-05-android-toolchain/bin/arm-eabi- mrproper;
cp .config.bkp .config;
make clean;
