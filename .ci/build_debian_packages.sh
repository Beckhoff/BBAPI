#!/bin/sh
# SPDX-License-Identifier: MIT
# Copyright (C) 2020 Beckhoff Automation GmbH & Co. KG

cleanup() {
	rm -rf "${tmp_dir}"
}

read_bbapi_version() {
	awk \
		'/^#define DRV_VERSION/ { gsub(/"/,"",$3); print $3; exit}' \
		"${package_dir}/api.c"
}

# By default, dkms mkbmdeb uses a template stored under the path.
# Based on this template, dkms mkbmdeb generates a debian package.
# The resulting package does not contain the maintainer or the detailed description of BBAPI.
# However, it is desirable that the generated package contains the above information.
# Therefore, within this function the debian package template is adapted so that the appropriate
# maintainer and package description of BBAPI are included in the package.
prepare_modules_package_basis() {
	# create directory required for dkms mkbmdeb command
	mkdir -pv "${tmp_dir}/usr/src/bbapi-${bbapi_version}/bbapi-dkms-mkbmdeb/debian/"

	# copy template Makefile provided by the dkms mkbmdeb tool
	cp \
		"/etc/dkms/template-dkms-mkbmdeb/Makefile" \
		"${tmp_dir}/usr/src/bbapi-${bbapi_version}/bbapi-dkms-mkbmdeb"

	# copy template debian/rules file provided by the dkms mkbmdeb tool
	cp \
		"/etc/dkms/template-dkms-mkbmdeb/debian/rules" \
		"${tmp_dir}/usr/src/bbapi-${bbapi_version}/bbapi-dkms-mkbmdeb/debian/"

	# replace latest BBAPI package version in changelog with MODULE_VERSION
	# as dkms mkbmdeb will replace this placeholder with an adapted package
	# version not containg a trailing debian_revision number, e.g. "-1"
	sed '1 s/(.*)/(MODULE_VERSION)/g' \
		"${package_dir}/debian/changelog" \
		> \
		"${tmp_dir}/usr/src/bbapi-${bbapi_version}/bbapi-dkms-mkbmdeb/debian/changelog"

	# copy debian/copyright file containing the correct copyright and debian/compat file
	cp -r \
		"${package_dir}/debian/copyright" \
		"${package_dir}/debian/compat" \
		"${tmp_dir}/usr/src/bbapi-${bbapi_version}/bbapi-dkms-mkbmdeb/debian/"

	# copy debian/control file containing the correct maintainer and package description
	cp \
		"${package_dir}/debian/bbapi-modules-control" \
		"${tmp_dir}/usr/src/bbapi-${bbapi_version}/bbapi-dkms-mkbmdeb/debian/control"
}

set -e
set -u

readonly package_dir="$(cd "$(dirname "${0}")" && pwd)/.."
readonly kernel_version='4.19.0-13-rt-amd64'
readonly bbapi_version="$(read_bbapi_version)"

trap cleanup EXIT INT TERM
tmp_dir="$(mktemp -d)"
readonly tmp_dir

apt update
apt install -y \
	dkms \
	"linux-headers-${kernel_version}"

rm -rf "${tmp_dir}"
bdpg build
mkdir -p "${tmp_dir}/bbapi/${bbapi_version}"
dpkg -x "${package_dir}/debian-release/bbapi-dkms_${bbapi_version}-1_all.deb" "${tmp_dir}"

mv "${tmp_dir}/usr/src/bbapi"* "${tmp_dir}/bbapi/${bbapi_version}/source"
dkms build --dkmstree "${tmp_dir}" -m "bbapi/${bbapi_version}" -k "${kernel_version}"

prepare_modules_package_basis
dkms mkbmdeb --dkmstree "${tmp_dir}" -m "bbapi/${bbapi_version}" -k "${kernel_version}"

mv \
	"${tmp_dir}/bbapi/${bbapi_version}/bmdeb/bbapi-modules-${kernel_version}_${bbapi_version}_amd64.deb" \
	"${package_dir}/debian-release/"
