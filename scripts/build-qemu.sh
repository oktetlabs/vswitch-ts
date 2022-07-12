#! /bin/bash
# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2016 - 2022 Xilinx, Inc. All rights reserved.

set -e

declare -a config_args=()

config_args+=("--disable-docs")
config_args+=("--disable-guest-agent")
config_args+=("--disable-tools")
config_args+=("--prefix=${TE_PREFIX}")
config_args+=("--target-list=x86_64-softmmu")

if ! test -f Makefile ; then
    "${EXT_SOURCES}/configure" "${config_args[@]}"
fi

make -j
make install

cd "${TE_PREFIX}"

components=(
    bin/qemu-system-x86_64
)

for ta_type in ${TE_TA_TYPES} ; do
    rsync -avR ${components[@]} "${TE_AGENTS_INST}/${ta_type}/"
done
