#!/bin/sh
# SPDX-License-Identifier: MIT
# Copyright (C) 2020 Beckhoff Automation GmbH & Co. KG

set -e
set -u

if test -z "${SSH_AUTH_SOCK-}"; then
	eval $(ssh-agent -s)
	echo "${APTLY_SSH_KEY}" | ssh-add -
fi

if ! bdpg git-pre-push-check master; then
	exit 0
fi

readonly package_dir="$(cd "$(dirname "${0}")" && pwd)/.."
bdpg push "${package_dir}/debian-release/bbapi-dev_"*.deb
bdpg push "${package_dir}/debian-release/bbapi-dkms_"*.deb
