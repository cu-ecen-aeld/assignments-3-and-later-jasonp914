#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
export CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    echo "Cleaning build directory ... "
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- mrproper
    echo "Setup default config ... "
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig
    echo "Compile linux ... "
    make -j8 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- all 
fi

echo "Adding the Image in outdir"
cd "$OUTDIR"
cp "linux-stable/arch/arm64/boot/Image" . 

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
mkdir -p rootfs
cd "${OUTDIR}/rootfs"
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log
mkdir -p home/conf/

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make config
   
else
    cd busybox
fi

# TODO: Make and install busybox
make distclean 
make defconfig
make -j8 ARCH=${ARCH} CROSS_COMPLE=${CROSS_COMPILE}
make CONFIG_PREFIX=../rootfs ARCH=${ARCH} CROSS_COMPLE=${CROSS_COMPILE} install

cd "${OUTDIR}/rootfs"
echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
cd "${OUTDIR}/rootfs"
ARMSYSROOTDIR="/home/user/Documents/installs/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu"
echo "Copy library dependences"
cp "${ARMSYSROOTDIR}/libc/lib/ld-linux-aarch64.so.1" "./lib/ld-linux-aarch64.so.1"  
cp "${ARMSYSROOTDIR}/libc/lib64/libm.so.6" "./lib64/libm.so.6"
cp "${ARMSYSROOTDIR}/libc/lib64/libresolv.so.2" "./lib64/libresolv.so.2"
cp "${ARMSYSROOTDIR}/libc/lib64/libc.so.6" "./lib64/libc.so.6"

# TODO: Make device nodes
echo "Make device nodes"
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 666 dev/console c 5 1

# TODO: Clean and build the writer utility
cd ~/Documents/coursera/intro_to_linux/assignment-2-jasonp914/finder-app
make clean
make

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs

cp "writer" "${OUTDIR}/rootfs/home/" 
cp "finder.sh" "${OUTDIR}/rootfs/home/"   
cp "conf/username.txt" "${OUTDIR}/rootfs/home/conf/"
cp "conf/assignment.txt" "${OUTDIR}/rootfs/home/conf/" 
cp "finder-test.sh" "${OUTDIR}/rootfs/home/"
cp "autorun-qemu.sh" "${OUTDIR}/rootfs/home/"        

# TODO: Chown the root directory
sudo chown root "${OUTDIR}/rootfs/"
# TODO: Create initramfs.cpio.gz
cd "${OUTDIR}/rootfs"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd "${OUTDIR}"
gzip -f initramfs.cpio

