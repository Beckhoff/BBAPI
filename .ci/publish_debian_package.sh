#!/bin/sh
# SPDX-License-Identifier: MIT
# Copyright (C) 2020 - 2021 Beckhoff Automation GmbH & Co. KG

bdpg_push() {
	local _package
	for _package in "$@"; do
		bdpg push --distros=bullseye "${_package}"
	done
}

set -e
set -u

# release on master only
if ! bdpg git-pre-push-check master; then
	exit 0
fi

if test -z "${SSH_AUTH_SOCK-}"; then
	eval $(ssh-agent -s)
	ssh-add - < "${SSH_KEY_APTLY}"
fi

readonly package_dir="$(cd "$(dirname "${0}")" && pwd)/.."
bdpg_push \
	"${package_dir}/debian-release/bbapi-dev_"*.deb \
	"${package_dir}/debian-release/bbapi-dkms_"*.deb \
	"${package_dir}/debian-release/bbapi-modules-"*.deb
