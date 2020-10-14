#!/bin/sh
# SPDX-License-Identifier: MIT
# Copyright (C) 2020 Beckhoff Automation GmbH & Co. KG

set -e
set -u

mkdir -p "/root/.ssh"

cat > /root/.ssh/config << EOF
Host *
	StrictHostKeyChecking no
	UserKnownHostsFile /dev/null
EOF
