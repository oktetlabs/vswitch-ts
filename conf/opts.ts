# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2016 - 2022 Xilinx, Inc. All rights reserved.
--conf-cs=cs/vswitch-ts.yml
--script=scripts/ta-def
--script=env/ta-build
--script=script.ta-build
--conf-cs=cs.vm-tmpl.yml
--script=scripts/defaults
--script=scripts/dpdk-trc-tags
