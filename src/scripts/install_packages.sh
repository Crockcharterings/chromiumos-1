#!/bin/bash

# Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script to install packages into the target root file system.
#
# NOTE: This script should be called by build_image.sh. Do not run this
# on your own unless you know what you are doing.

# Load common constants.  This should be the first executable line.
# The path to common.sh should be relative to your script's location.
. "$(dirname "$0")/common.sh"

# Script must be run inside the chroot
assert_inside_chroot
assert_not_root_user

DEFAULT_PKGLIST="${SRC_ROOT}/package_repo/package-list-prod.txt"

# Flags
DEFINE_string output_dir "" \
  "The location of the output directory to use [REQUIRED]."
DEFINE_string root ""      \
  "The root file system to install packages in."
DEFINE_string target "x86" \
  "The target architecture to build for. One of { x86, arm }."
DEFINE_string build_root "$DEFAULT_BUILD_ROOT"                \
  "Root of build output"
DEFINE_string package_list "$DEFAULT_PKGLIST" \
  "The package list file to use."
DEFINE_string server "$DEFAULT_IMG_MIRROR" \
  "The package server to use."
DEFINE_string suite "$DEFAULT_IMG_SUITE" \
  "The package suite to use."

# Parse command line
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"

# Die on any errors.
set -e

KERNEL_DEB_PATH=$(find "${FLAGS_build_root}/${FLAGS_target}/local_packages" \
  -name "linux-image-*.deb")
KERNEL_DEB=$(basename "${KERNEL_DEB_PATH}" .deb | sed -e 's/linux-image-//' \
  -e 's/_.*//')
KERNEL_VERSION=${KERNEL_VERSION:-${KERNEL_DEB}}

if [[ -z "$FLAGS_output_dir" ]]; then
  echo "Error: --output_dir is required."
  exit 1
fi
OUTPUT_DIR=$(readlink -f "$FLAGS_output_dir")
SETUP_DIR="${OUTPUT_DIR}/local_repo"
ROOT_FS_DIR="${OUTPUT_DIR}/rootfs"
if [[ -n "$FLAGS_root" ]]; then
  ROOT_FS_DIR=$(readlink -f "$FLAGS_root")
fi
mkdir -p "$OUTPUT_DIR" "$SETUP_DIR" "$ROOT_FS_DIR"

# Make sure anything mounted in the rootfs is cleaned up ok on exit.
cleanup_rootfs_mounts() {
  # Occasionally there are some daemons left hanging around that have our
  # root image file system open. We do a best effort attempt to kill them.
  PIDS=`sudo lsof -t "$ROOT_FS_DIR" | sort | uniq`
  for pid in $PIDS
  do
    local cmdline=`cat /proc/$pid/cmdline`
    echo "Killing process that has open file on our rootfs: $cmdline"
    ! sudo kill $pid  # Preceded by ! to disable ERR trap.
  done

  # Sometimes the volatile directory is left mounted and sometimes it is not,
  # so we precede by '!' to disable the ERR trap.
  ! sudo umount "$ROOT_FS_DIR"/lib/modules/2.6.*/volatile/ > /dev/null 2>&1

  sudo umount "${ROOT_FS_DIR}/proc"
  sudo umount "${ROOT_FS_DIR}/sys"
}

# Create setup directory and copy over scripts, config files, and locally
# built packages.
mkdir -p "${SETUP_DIR}/local_packages"
cp "${FLAGS_build_root}/${FLAGS_target}/local_packages"/* \
  "${SETUP_DIR}/local_packages"

# Set up repository for local packages to install in the rootfs via apt-get.
cd "$SETUP_DIR"
dpkg-scanpackages local_packages/ /dev/null | \
   gzip > local_packages/Packages.gz
cd -

# Create the temporary apt source.list used to install packages.
APT_SOURCE="${OUTPUT_DIR}/sources.list"
cat <<EOF > "$APT_SOURCE"
deb file:"$SETUP_DIR" local_packages/
deb $FLAGS_server $FLAGS_suite main restricted multiverse universe
EOF

# Cache directory for APT to use.
APT_CACHE_DIR="${OUTPUT_DIR}/tmp/cache/"
mkdir -p "${APT_CACHE_DIR}/archives/partial"

# Create the apt configuration file. See "man apt.conf"
NO_MAINTAINER_SCRIPTS=""
if [ -n "$EXPERIMENTAL_NO_MAINTAINER_SCRIPTS" ]; then
  NO_MAINTAINER_SCRIPTS="Bin { dpkg \"${SCRIPTS_DIR}/dpkg_no_scripts.sh\"; };"
fi
APT_PARTS="${OUTPUT_DIR}/apt.conf.d"
mkdir -p "$APT_PARTS"  # An empty apt.conf.d to avoid other configs.
export APT_CONFIG="${OUTPUT_DIR}/apt.conf"
cat <<EOF > "$APT_CONFIG"
APT
{
  Install-Recommends "0";
  Install-Suggests "0";
  Get
  {
    Assume-Yes "1";
  };
};
Dir
{
  $NO_MAINTAINER_SCRIPTS
  Cache "$APT_CACHE_DIR";
  Cache {
    archives "${APT_CACHE_DIR}/archives";
  };
  Etc
  {
    sourcelist "$APT_SOURCE";
    parts "$APT_PARTS";
  };
  State "${ROOT_FS_DIR}/var/lib/apt/";
  State
  {
    status "${ROOT_FS_DIR}/var/lib/dpkg/status";
  };
};
DPkg
{
  options {"--root=${ROOT_FS_DIR}";};
};
EOF

# TODO: Full audit of the apt conf dump to make sure things are ok.
apt-config dump > "${OUTPUT_DIR}/apt.conf.dump"

# Add debootstrap link for the suite, if it doesn't exist.
if [ ! -e "/usr/share/debootstrap/scripts/$FLAGS_suite" ]
then
  sudo ln -s /usr/share/debootstrap/scripts/jaunty \
    "/usr/share/debootstrap/scripts/$FLAGS_suite"
fi

if [ -z "$EXPERIMENTAL_NO_DEBOOTSTRAP" -a \
     -z "$EXPERIMENTAL_NO_MAINTAINER_SCRIPTS" ]; then
  # Use debootstrap, which runs maintainer scripts.
  sudo debootstrap --arch=i386 $FLAGS_suite "$ROOT_FS_DIR" "${FLAGS_server}"
else
  # A hack-in-progress that does our own debootstrap equivalent and skips
  # maintainer scripts.

  # TODO: Replace with a pointer to lool's repo or maybe apt-get --download-only?
  REPO="${GCLIENT_ROOT}/repo/var/cache/make_local_repo"

  # The set of required packages before apt can take over.
  # TODO: Trim this as much as possible. It is *very* picky, so be careful.
  PACKAGES="base-files base-passwd bash bsdutils coreutils dash debconf debconf-i18n debianutils diff dpkg e2fslibs e2fsprogs findutils gcc-4.4-base grep gzip hostname initscripts insserv libacl1 libattr1 libblkid1 libc-bin libc6 libcomerr2 libdb4.7 libdbus-1-3 libgcc1 liblocale-gettext-perl libncurses5 libpam-modules libpam-runtime libpam0g libselinux1 libsepol1 libslang2 libss2 libssl0.9.8 libstdc++6 libtext-charwidth-perl libtext-iconv-perl libtext-wrapi18n-perl libudev0 libuuid1 locales login lsb-base lzma makedev mawk mount mountall ncurses-base ncurses-bin passwd perl-base procps python-minimal python2.6-minimal sed sysv-rc sysvinit-utils tar tzdata upstart util-linux zlib1g apt"

  # Prep the rootfs to work with dpgk and apt
  sudo mkdir -p "${ROOT_FS_DIR}/var/lib/dpkg/info"
  sudo touch "${ROOT_FS_DIR}/var/lib/dpkg/available"   \
    "${ROOT_FS_DIR}/var/lib/dpkg/diversions"           \
    "${ROOT_FS_DIR}/var/lib/dpkg/status"
  sudo mkdir -p "${ROOT_FS_DIR}/var/lib/apt/lists/partial"  \
    "${ROOT_FS_DIR}/var/lib/dpkg/updates"

  i=0
  for p in $PACKAGES; do
    set +e
    PKG=$(ls "${REPO}"/${p}*_i386.deb)
    set -e
    if [ -z "$PKG" ]; then
      PKG=$(ls "${REPO}"/${p}*_all.deb)
    fi
    echo "Installing package: $PKG [$i]"
    sudo "${SCRIPTS_DIR}"/dpkg_no_scripts.sh \
      --root="$ROOT_FS_DIR" --unpack "$PKG"
    i=$((i + 1))
  done

  # TODO: Remove when we stop having maintainer scripts altogether.
  sudo cp -a /dev/* "${ROOT_FS_DIR}/dev"

  # ----- MAINTAINER SCRIPT FIXUPS -----

  # base-passwd
  sudo cp "${ROOT_FS_DIR}/usr/share/base-passwd/passwd.master" \
    "${ROOT_FS_DIR}/etc/passwd"
  sudo cp "${ROOT_FS_DIR}/usr/share/base-passwd/group.master" \
    "${ROOT_FS_DIR}/etc/group"

  # libpam-runtime
  # The postinst script calls pam-auth-update, which is a perl script that
  # expects to run within the targetfs. Until we fix this, we just copy
  # from the build chroot.
  sudo cp -a /etc/pam.d/common-* \
    /etc/pam.d/login             \
    /etc/pam.d/newusers          \
    /etc/pam.d/su                \
    /etc/pam.d/sudo              \
    "${ROOT_FS_DIR}/etc/pam.d/"

  # mawk
  sudo ln -s mawk "${ROOT_FS_DIR}/usr/bin/awk"

  # base-files?
  sudo touch "${ROOT_FS_DIR}/etc/fstab"
fi  # EXPERIMENTAL_NO_DEBOOTSTRAP


# Set up mounts for working within the rootfs. We copy some basic
# network information from the host so that maintainer scripts can
# access the network as needed.
# TODO: All of this rootfs mount stuff can be removed as soon as we stop
# running the maintainer scripts on install.
sudo mount -t proc proc "${ROOT_FS_DIR}/proc"
sudo mount -t sysfs sysfs "${ROOT_FS_DIR}/sys" # TODO: Do we need sysfs?
sudo cp /etc/hosts "${ROOT_FS_DIR}/etc"
trap cleanup_rootfs_mounts EXIT

# Make sure that apt is ready to work.
sudo APT_CONFIG="$APT_CONFIG" DEBIAN_FRONTEND=noninteractive apt-get update

# TODO: We found that apt-get install --fix-broken is needed. It removes some
# -dev packages and we need to allow it to run maintainer scripts for now.
TMP_FORCE_DPKG="-o=Dir::Bin::dpkg=/usr/bin/dpkg"
sudo APT_CONFIG="$APT_CONFIG" DEBIAN_FRONTEND=noninteractive \
  apt-get $TMP_FORCE_DPKG --force-yes -f install

# Install prod packages
COMPONENTS=`cat $FLAGS_package_list | grep -v ' *#' | grep -v '^ *$' | sed '/$/{N;s/\n/ /;}'`
sudo APT_CONFIG="$APT_CONFIG" DEBIAN_FRONTEND=noninteractive \
  apt-get --force-yes install $COMPONENTS

# Create kernel installation configuration to suppress warnings,
# install the kernel in /boot, and manage symlinks.
cat <<EOF | sudo dd of="${ROOT_FS_DIR}/etc/kernel-img.conf"
link_in_boot = yes
do_symlinks = yes
minimal_swap = yes
clobber_modules = yes
warn_reboot = no
do_bootloader = no
do_initrd = yes
warn_initrd = no
EOF

# Install the kernel.
sudo APT_CONFIG="$APT_CONFIG" DEBIAN_FRONTEND=noninteractive \
  apt-get --force-yes install "linux-image-${KERNEL_VERSION}"

# Clean up the apt cache.
# TODO: The cache was populated by debootstrap, not these installs. Remove
# this line when we can get debootstrap to stop doing this.
sudo rm -f "${ROOT_FS_DIR}"/var/cache/apt/archives/*.deb
# Need to rm read-only created lock files in order for archiving step to work
sudo rm -rf "$APT_CACHE_DIR"

# List all packages installed so far, since these are what the local
# repository needs to contain.
# TODO: Replace with list_installed_packages.sh when it is fixed up.
dpkg --root="${ROOT_FS_DIR}" -l > \
  "${OUTPUT_DIR}/package_list_installed.txt"

cleanup_rootfs_mounts
trap - EXIT
