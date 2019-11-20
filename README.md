![pace-status](https://img.shields.io/badge/Pace%20Kernel-boots-yellow.svg)
![stratos-status](https://img.shields.io/badge/Stratos%20Kernel-none-inactive.svg)
![verge-status](https://img.shields.io/badge/Verge%20Kernel-none-inactive.svg)

# Kernels for Amazfit watches
### Development of working kernel sources for Amazfit watches

This project aims at developing working open source kernels for all the Amazfit smart-watches. Such kernel sources will help the community develop/port other popular Softwares for the Amazfit devices.

In the main branch (here) are the Ingenic's Newton 2 Plus kernel-3.10.14 sources. Newton 2 Plus is a development board which have many similarities with the Amazfit smart-watches, thus we will base our kernels on these sources. The sources were pull from [ingenic's git server](http://git.ingenic.com.cn:8082/) for use with Amazfit ingenic products.

### Progress

| Platform | Status  | Branch Link |
|----------|---------|-------------|
| Pace     | boots   | [pace-mods-development](https://github.com/amazfitDev/platform_kernel-3.10.14/tree/pace-mods-development) |
| Stratos  | -       | -           |
| Verge    | -       | -           |

*hopfully we will have only one source/branch for all on the future*

### FAQ

*- What is this?*
*- You shouldn't be here...*

*- How I use it?*
*- You shouldn't be here...*

*- When will you release AW?*
*- You shouldn't be here...*

*- I lost my dog.*
*- It shouldn't be here...*

---

### Building the Sources with Ingenic's Android

Some info on building the sources of the ingenic newton 2 plus. This is not exactly a tutorial... This was tested on Ubuntu 18.04, but it may work on other GNU/Linux too.

#### Install dependancies

First get adb and fast boot

```sh
sudo apt-get install android-tools-adb android-tools-fastboot
```

And some more dependancies that I think you may need

```sh
sudo apt-get install git-core gnupg flex bc bison gperf build-essential zip curl zlib1g-dev gcc-multilib g++-multilib libc6-dev-i386 lib32ncurses5-dev x11proto-core-dev libx11-dev lib32z-dev libgl1-mesa-dev libxml2 libxml2-utils xsltproc unzip imagemagick lib32readline-dev lib32z1-dev liblz4-tool libncurses5-dev libsdl1.2-dev libssl-dev libwxgtk3.0-dev lzop pngcrush rsync schedtool squashfs-tools xsltproc yasm
```

#### Install Java 7 (needed by this android version)
So... you need jdk7... and you will have to manualy install it

Thus, you may added the openjdk repo for to install the version 8 and some other needed depenancies

```sh
sudo add-apt-repository ppa:openjdk-r/ppa  
sudo apt update
sudo apt install openjdk-8-jdk
# Then I got the packages from the experimental repo
# https://packages.debian.org/sid/libjpeg62-turbo
# https://packages.debian.org/experimental/openjdk-7-jre-headless
# https://packages.debian.org/experimental/openjdk-7-jre
# https://packages.debian.org/experimental/openjdk-7-jdk
# and installed them
sudo dpkg -i libjpeg62-turbo_*_amd64.deb
sudo dpkg -i openjdk-7-jre-headless_*_amd64.deb
sudo dpkg -i openjdk-7-jre_*_amd64.deb
sudo dpkg -i openjdk-7-jdk_*_amd64.deb
sudo apt --fix-broken install
# Then change the selected java version
sudo update-alternatives --config java
sudo update-alternatives --config javac
sudo update-alternatives --config javaws # no option, ignore
sudo update-alternatives --config javadoc
```



#### Get repo from ingenic

This will take some time...

```sh
wget http://git.ingenic.com.cn:8082/bj/repo
chmod +x repo
./repo init -u http://git.ingenic.com.cn:8082/gerrit/mipsia/platform/manifest.git -b ingenic-android-lollipop_mr1-kernel3.10.14-newton2_plus-v3.0-20160908
./repo sync
```

Personally, I failed to compile v3.0 (it had some API errors)

```sh
#./repo forall -c "git reset --hard ingenic-android-lollipop_mr1-kernel3.10.14-newton2_plus-v3.0-20160908"
```

So it got the v2.0

```sh
# Get v2.0
./repo forall -c "git reset --hard ingenic-android-lollipop_mr1-kernel3.10.14-newton2_plus-v2.0-20160516"
```

#### Fix some errors

Now edit `build/core/clang/HOST_x86_common.mk` to fix clang bug. Add the `-B$($(clang_2nd_arch_prefix)HOST_TOOLCHAIN_FOR_CLANG)/x86_64-linux/bin \` on line 11, so the lines from 8 to 12 would be:

```txt
ifeq ($(HOST_OS),linux)
CLANG_CONFIG_x86_LINUX_HOST_EXTRA_ASFLAGS := \
  --gcc-toolchain=$($(clang_2nd_arch_prefix)HOST_TOOLCHAIN_FOR_CLANG) \
  --sysroot=$($(clang_2nd_arch_prefix)HOST_TOOLCHAIN_FOR_CLANG)/sysroot \
  -B$($(clang_2nd_arch_prefix)HOST_TOOLCHAIN_FOR_CLANG)/x86_64-linux/bin \
  -no-integrated-as
# END
```

#### Download this kernel

Download this kernel and replace the stock one.

#### Start build
source build/envsetup.sh
lunch newton2_plus-userdebug
make update-api
make -j4
