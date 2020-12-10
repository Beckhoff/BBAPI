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
dkms mkbmdeb --dkmstree "${tmp_dir}" -m "bbapi/${bbapi_version}" -k "${kernel_version}"
mv \
	"${tmp_dir}/bbapi/${bbapi_version}/bmdeb/bbapi-modules-${kernel_version}_${bbapi_version}_amd64.deb" \
	"${package_dir}/debian-release/"
