## Kernel-3.10.14 development for Amazfit Pace
### Status
The kernel boots, then after the screen timeouts the device is unresponsive.


### Compiling the kernel

You need to have on your system's path the mipsel-linux-android toolchain of your preference.
I used `mipsel-linux-android-4.8` for `linux-x86`.

```bash
# Defince cross compiler and architecture
export CROSS_COMPILE=mipsel-linux-android-
export ARCH=mips && export SUBARCH=mips
# Copy the configuration file we created for Pace
make pace_amazfitdevs_android_defconfig
# Configure the kernel (optional)
make menuconfig
# Build Kernel
make zImage
```

The zImage will be located at `./arch/mips/boot/compressed/zImage`.

If you want to continue building a bootimage, copy the zImage to the device folder:
```bash
cp ./arch/mips/boot/compressed/zImage <path-to-ingenic-newton-plus-sources>/device/ingenic/newton/kernel
```

If your toolchain is not on on path, you may temporary add it for this terminal session (ingenic sources includes them)
```bash
export PATH=<path-to-ingenic-newton-plus-sources>/prebuilts/gcc/linux-x86/mips/mipsel-linux-android-4.8/bin:$PATH
```
