#!/bin/sh

# This was copied from https://git.beckhoff.dev/beckhoff/debian-install/-/blob/05d6e9e08d344b3580e3c71bf3e62952fd8b115b/debian-mkrootfs
# We dropped the linux-image-bhf kernel and packages we don't need. Important
# is we need a kernel without bbapi driver compiled in, because here we want
# to test our out of tree module version.
mkrootfs() {
	mmdebstrap \
		${DEBIAN_MKROOTFS_ARCH:+--architectures="${DEBIAN_MKROOTFS_ARCH}"} \
		--variant=minbase \
		--hook-directory=/usr/share/misc/debian-install/mmdebstrap-hooks \
		--include="${default_packages}" \
		--verbose \
		"${DEBIAN_MKROOTFS_SUITE}" \
		"${rootfs}" \
		https://deb-mirror.beckhoff.com/debian
}

set -e
set -u
set -x

readonly rackctl_device=test-device

device_ip="$(rackctl-config get "${rackctl_device}" ip)"

readonly device_ip
readonly ssh_target="Administrator@${device_ip}"

eval "$(cleanup init)"

# We need our mmdebstrap hooks
sudo apt update && sudo apt install debian-install

# Generate a rootfs with an upstream Debian kernel
export DEBIAN_MKROOTFS_ADMIN_PASSWORD=1
export DEBIAN_MKROOTFS_ADMIN_USER=Administrator
export DEBIAN_MKROOTFS_BHF_DISTRO=bookworm-experimental
export DEBIAN_MKROOTFS_PIPELINE_REPOS=""
export DEBIAN_MKROOTFS_SUITE=bookworm
default_packages="\
bhfinfo,\
build-essential,\
ca-certificates,\
cmake,\
curl,\
linux-headers-rt-amd64,\
linux-image-rt-amd64,\
ssh,\
sudo,\
systemd,\
systemd-sysv,\
zstd"

# ATTENTION: We can't use /tmp here. NFS root mount would complain about
# missing permissions, so we put our tmpdir in the working directory.
tmpdir="$(pwd)/tmpdir"
"${CLEANUP}/add" "rm -rf '${tmpdir}'"

# Fake directory structure for rackctl-netboot
readonly rootfs="${tmpdir}/nfs/rootfs"
mkdir -p "${rootfs}"
mkrootfs

# TODO replae this with a proper tool or proper test-runner container
. 03_init_ssh.sh

# Netboot an upstream Debian kernel on a device with Beckhoff BIOS
#	ln -sf /proc/net/pnp "${rootfs}/etc/resolv.conf"
	cat > "${rootfs}/etc/resolv.conf" <<- EOF
		domain beckhoff.com
		search beckhoff.com
		nameserver $(rackctl-config get "${rackctl_device}" rackcontroller/ip)
	EOF

	# We need noninteractive sudo for modules install and to access /dev/bbapi
	printf 'ALL\tALL = (ALL) NOPASSWD: ALL\n' >> "${rootfs}/etc/sudoers"

	rackctl-netboot \
		--workdir="${tmpdir}" \
		"${rackctl_device}" local &
        "${CLEANUP}/add" "neokill $!"
        wait_ssh "${ssh_target}"

./test_stage/test_stage.sh "$@" --config-dir=./.test-stage --target-os=tclur rackctl "${rackctl_device}"
