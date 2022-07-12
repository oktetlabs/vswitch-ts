#! /bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2016 - 2022 Xilinx, Inc. All rights reserved.

set -e

BUILD_DPDK="$1"

version_greater_or_eq() {
    test "$1" = "`echo -e "$1\n$2" | sort -V | tail -n1`"
}

test -f "${EXT_SOURCES}/configure" || {
    ret="$(pwd -P)"
    cd "${EXT_SOURCES}"
    ./boot.sh
    cd "${ret}"
}

config_args=()
kernel_ver="$(uname -r | grep -o '^[0-9]*\.[0-9]*\.[0-9]*')"
copy_modules=false
if ! version_greater_or_eq "$kernel_ver" "5.9.0"; then
    copy_modules=true
    config_args+=("--with-linux=/lib/modules/$(uname -r)/build")
fi

"${BUILD_DPDK:-false}" && config_args+=("--with-dpdk=static")

"${EXT_SOURCES}/configure" "${config_args[@]}"

make
make install prefix=${TE_PREFIX} exec_prefix=${TE_PREFIX}
if "${copy_modules}" ; then
    for ta_type in ${TE_TA_TYPES} ; do
        cp datapath/linux/*.ko "${TE_AGENTS_INST}/${ta_type}/"
    done
fi

cd ${TE_PREFIX}

components=(
    bin/ovs-appctl
    bin/ovsdb-tool
    bin/ovs-ofctl
    bin/ovs-vsctl
    sbin/ovsdb-server
    sbin/ovs-vswitchd
    share/openvswitch/vswitch.ovsschema
)

for ta_type in ${TE_TA_TYPES} ; do
    cp -t "${TE_AGENTS_INST}/${ta_type}" ${components[@]}
done
