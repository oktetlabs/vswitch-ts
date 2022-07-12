/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2016 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief vSwitch Test Suite
 *
 * Declarations of common functions for tests.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#ifndef __TS_VSWITCH_TS_H__
#define __TS_VSWITCH_TS_H__

/**
 * Perform basic configuration of all interfaces on every node
 * of network configuration.
 *
 * @note The function jumps out on failure
 */
extern void test_prepare_all_interfaces();

#endif /* !__TS_VSWITCH_TS_H__ */
