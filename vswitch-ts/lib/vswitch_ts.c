/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2016 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief vSwitch Test Suite
 *
 * Implementation of common functions.
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

/** User name of vSwitch test suite library */
#define TE_LGR_USER     "Library"

#include "te_config.h"
#include "te_toeplitz.h"
#include "te_alloc.h"
#include "te_ethernet.h"
#include "tapi_cfg_base.h"

#include "vswitch_ts.h"
#include "vswitch_test.h"

static tapi_cfg_net_node_cb test_assign_mac_if_zeroed;
static te_errno
test_assign_mac_if_zeroed(cfg_net_t *net, cfg_net_node_t *node,
                          const char *oid_str, cfg_oid *oid, void *cookie)
{
    static uint8_t assign_mac[ETHER_ADDR_LEN] = {
        0x00, 0x56, 0x63, 0xe4, 0x2a, 0x00
    };
    uint8_t mac[ETHER_ADDR_LEN];
    te_bool zeroed = TRUE;
    unsigned int i;
    te_errno rc;

    UNUSED(net);
    UNUSED(node);
    UNUSED(cookie);

    if (strcmp(cfg_oid_inst_subid(oid, 1), "agent") != 0 ||
        strcmp(cfg_oid_inst_subid(oid, 2), "interface") != 0)
        return 0;

    rc = tapi_cfg_base_if_get_mac(oid_str, mac);
    if (rc != 0)
    {
        ERROR("Failed to get MAC address of '%s'", oid_str);
        return rc;
    }

    for (i = 0; i < TE_ARRAY_LEN(mac); i++)
    {
        if (mac[i] != 0)
        {
            zeroed = FALSE;
            break;
        }
    }

    if (!zeroed)
        return 0;

    rc = tapi_cfg_base_if_set_mac(oid_str, assign_mac);
    if (rc != 0)
        return rc;

    assign_mac[ETHER_ADDR_LEN - 1]++;

    return 0;
}

void
test_prepare_all_interfaces()
{
    te_errno rc;

    rc = tapi_cfg_net_foreach_node(test_assign_mac_if_zeroed, NULL);
    if (rc != 0)
        TEST_FAIL("Failed to assign MAC to interfaces without MAC: %r", rc);

    rc = tapi_cfg_net_all_up(FALSE);
    if (rc != 0)
        TEST_FAIL("Failed to up all interfaces mentioned in networks "
                  "configuration: %r", rc);

    rc = tapi_cfg_net_delete_all_ip4_addresses();
    if (rc != 0)
        TEST_FAIL("Failed to delete all IPv4 addresses from all "
                  "interfaces mentioned in networks configuration: %r",
                  rc);

    rc = tapi_cfg_net_all_assign_ip(AF_INET);
    if (rc != 0)
        TEST_FAIL("Failed to assign IPv4 subnets: %r", rc);

    rc = tapi_cfg_net_all_assign_ip(AF_INET6);
    if (rc != 0)
        TEST_FAIL("Failed to assign IPv6 subnets: %r", rc);
}
