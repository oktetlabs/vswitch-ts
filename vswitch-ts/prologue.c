/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2016 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Test Suite prologue
 *
 * vSwitch Test Suite
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#ifndef DOXYGEN_TEST_SPEC

/** Logging subsystem entity name */
#define TE_TEST_NAME    "prologue"

#include "vswitch_test.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif

#include "tapi_cfg_base.h"
#include "tapi_cfg_modules.h"
#include "tapi_cfg_net.h"
#include "logger_ten.h"
#include "tapi_test.h"
#include "rcf_api.h"
#include "tapi_sh_env.h"

static rcf_ta_cb load_ovs_modules_if_need_be;
static te_errno
load_ovs_modules_if_need_be(const char *ta, void *cookie)
{
    static const char *modules[] = { "openvswitch", "vport-vxlan" };
    const char *ovs_src = getenv("OVS_SRC");
    te_bool iut_node_found = FALSE;
    cfg_val_type type = CVT_STRING;
    cfg_nets_t nets;
    unsigned int i;
    te_errno rc;

    UNUSED(cookie);

    if (ovs_src == NULL)
        return 0;

    rc = tapi_cfg_net_get_nets(&nets);
    if (rc != 0)
    {
        ERROR("Failed to get networks from Configurator: %r", rc);
        return rc;
    }

    for (i = 0; i < nets.n_nets; i++)
    {
        cfg_net_t *net = &nets.nets[i];
        unsigned int j;

        for (j = 0; j < net->n_nodes; ++j)
        {
            char *oid_str;
            char *agent;

            rc = cfg_get_instance(net->nodes[j].handle, &type, &oid_str);
            if (rc != 0)
            {
                ERROR("Failed to get network node instance");
                tapi_cfg_net_free_nets(&nets);
                return rc;
            }

            agent = cfg_oid_str_get_inst_name(oid_str, 1);
            free(oid_str);
            if (agent == NULL)
            {
                ERROR("Failed to get agent of a network node");
                tapi_cfg_net_free_nets(&nets);
                return rc;
            }

            if (strcmp(agent, ta) == 0 &&
                net->nodes[j].type == NET_NODE_TYPE_NUT)
            {
                iut_node_found = TRUE;
                break;
            }
        }
    }

    tapi_cfg_net_free_nets(&nets);

    /* Load modules only for IUT agent */
    if (!iut_node_found)
        return 0;

    for (i = 0; i < TE_ARRAY_LEN(modules); i++)
    {
        rc = tapi_cfg_module_add_from_ta_dir_or_fallback(ta, modules[i], TRUE);
        if (rc != 0)
            return rc;

        rc = tapi_cfg_module_change_finish(ta, modules[i]);
        if (rc != 0)
            return rc;
    }

    return rc;
}

/**
 * Reserve network interfaces and assign test IP addresses for test
 * networks. Load OVS kernel datapath modules.
 *
 * @retval EXIT_SUCCESS     success
 * @retval EXIT_FAILURE     failure
 */
int
main(int argc, char **argv)
{
/* Redefine as empty to avoid environment processing here */
#undef TEST_START_VARS
#define TEST_START_VARS
#undef TEST_START_SPECIFIC
#define TEST_START_SPECIFIC
#undef TEST_END_SPECIFIC
#define TEST_END_SPECIFIC

    TEST_START;

    CHECK_RC(tapi_expand_path_all_ta(NULL));

    if ((rc = tapi_cfg_net_remove_empty()) != 0)
        TEST_FAIL("Failed to remove /net instances with empty interfaces");

    rc = tapi_cfg_net_reserve_all();
    if (rc != 0)
        TEST_FAIL("Failed to reserve all interfaces mentioned in networks "
                  "configuration: %r", rc);

    CHECK_RC(rcf_foreach_ta(load_ovs_modules_if_need_be, NULL));

    CFG_WAIT_CHANGES;

    CHECK_RC(rc = cfg_synchronize("/:", TRUE));
    CHECK_RC(rc = cfg_tree_print(NULL, TE_LL_RING, "/:"));

    TEST_SUCCESS;

cleanup:

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
