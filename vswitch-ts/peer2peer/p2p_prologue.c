/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2016 - 2022 Xilinx, Inc. All rights reserved. */
/** @file
 * @brief Peer-to-peer package prologue
 *
 * vSwitch Test Suite
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#ifndef DOXYGEN_TEST_SPEC

/** Logging subsystem entity name */
#define TE_TEST_NAME    "p2p_prologue"

#include "vswitch_test.h"

#include "te_kvpair.h"
#include "conf_api.h"
#include "te_sockaddr.h"
#include "tapi_rpc_rte_eal.h"
#include "tapi_cfg_ovs.h"
#include "tapi_cfg_modules.h"

#define TEST_VNI    55

/** MTU to be set on transport interface */
#define TEST_MTU    1500

#define TEST_ETHER_HDR_LEN  14
#define TEST_VLAN_TAG_LEN   4
#define TEST_IP4_HDR_LEN    20
#define TEST_IP6_HDR_LEN    40
#define TEST_UDP_HDR_LEN    8
#define TEST_VXLAN_HDR_LEN  8
#define TEST_GENEVE_HDR_LEN 8
/**
 * MTU to be set on tunnel transport interface to allow #c TEST_MTU
 * over tunnel.
 */
#define TEST_TNL_LOWER_MTU (TEST_MTU + TEST_ETHER_HDR_LEN + \
                            (TEST_VLAN_TAG_LEN * 2) + \
                            MAX(TEST_IP4_HDR_LEN, TEST_IP6_HDR_LEN) + \
                            TEST_UDP_HDR_LEN + \
                            MAX(TEST_VXLAN_HDR_LEN, TEST_GENEVE_HDR_LEN))

/** IUT VM name */
#define TEST_VM_IUT "iut_vm"

/** Test agent name to be run on IUT VM */
#define TEST_VM_TA  "Agt_VM"

/** Representor ID on the IUT to use when representor is tested */
#define TEST_VF_NUMBER 0

static tapi_cfg_net_node_cb iut_node_set_mtu;
static te_errno
iut_node_set_mtu(cfg_net_t *net, cfg_net_node_t *node,
                 const char *oid_str, cfg_oid *oid, void *cookie)
{
    te_errno rc;
    int mtu = *((int *)cookie);

    UNUSED(net);
    UNUSED(node);
    UNUSED(oid);

    rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, mtu), "%s/mtu:", oid_str);
    if (rc != 0)
        return rc;

    return 0;
}

/**
 * Modify configuration in accordance with configuration file.
 *
 * @retval EXIT_SUCCESS     success
 * @retval EXIT_FAILURE     failure
 */
int
main(int argc, char **argv)
{
/* Redefine as empty to avoid environment processing in TEST_START */
#undef TEST_START_VARS
#define TEST_START_VARS
#undef TEST_START_SPECIFIC
#define TEST_START_SPECIFIC
#undef TEST_END_SPECIFIC
#define TEST_END_SPECIFIC
    rcf_rpc_server              *iut_rpcs = NULL;
    tapi_env_net                *net      = NULL;
    tapi_env_host               *iut_host = NULL;
    const struct if_nameindex   *iut_if   = NULL;
    const struct sockaddr       *iut_addr = NULL;
    tapi_env_host               *tst_host = NULL;
    rcf_rpc_server              *tst_rpcs = NULL;
    const struct if_nameindex   *tst_if   = NULL;
    const struct sockaddr       *tst_addr = NULL;
    tapi_env                     env;
    te_bool                      env_init = FALSE;
    cfg_net_pci_info_t           iut_if_pci_info;

    char *config = NULL;
    te_bool with_dpdk;
    int mtu;
    const char *vm_os = NULL;

    char addr_buf[INET6_ADDRSTRLEN];

    te_kvpair_h expand_vars = TAILQ_HEAD_INITIALIZER(expand_vars);

    TEST_START;
    memset(&env, 0, sizeof(env));

    TEST_GET_BOOL_PARAM(with_dpdk);

    CHECK_RC(tapi_cfg_net_get_iut_if_pci_info(&iut_if_pci_info));

    if (with_dpdk)
    {
        CHECK_RC(tapi_cfg_net_bind_driver_by_node(NET_NODE_TYPE_NUT,
                                                  NET_DRIVER_TYPE_DPDK));
        CHECK_RC(tapi_cfg_net_nodes_switch_pci_fn_to_interface(
                                                    NET_NODE_TYPE_AGENT));
    }
    else
    {
        CHECK_RC(tapi_cfg_net_nodes_switch_pci_fn_to_interface(
                                                NET_NODE_TYPE_INVALID));
    }
    test_prepare_all_interfaces();

    TEST_START_ENV;
    env_init = TRUE;

    TEST_GET_FILENAME_PARAM(config);
    TEST_GET_PCO(iut_rpcs);
    TEST_GET_PCO(tst_rpcs);
    TEST_GET_IF(iut_if);
    TEST_GET_IF(tst_if);

    /* Do not free environment to keep RPC servers in the case of reuse PCO */
    /* TODO May be we should be it in a more elegant way via TAPI Env API */
    {
        const char *reuse_pco = getenv("TE_ENV_REUSE_PCO");

        if (reuse_pco != NULL && strcasecmp(reuse_pco, "yes") == 0)
        {
            const tapi_env_pco *iut_pco = tapi_env_rpcs2pco(&env, iut_rpcs);
            const tapi_env_pco *tst_pco = tapi_env_rpcs2pco(&env, tst_rpcs);

            ((tapi_env_pco *)iut_pco)->created = FALSE;
            ((tapi_env_pco *)tst_pco)->created = FALSE;
        }
    }

    if (config == NULL)
    {
        CHECK_RC(cfg_set_instance_fmt(CFG_VAL(INTEGER, TEST_MTU),
                                      "/agent:%s/interface:%s/mtu:",
                                      iut_rpcs->ta, iut_if->if_name));
        CHECK_RC(cfg_set_instance_fmt(CFG_VAL(INTEGER, TEST_MTU),
                                      "/agent:%s/interface:%s/mtu:",
                                      tst_rpcs->ta, tst_if->if_name));
        TEST_SUCCESS;
    }

    TEST_GET_STRING_PARAM(vm_os);
    TEST_GET_NET(net);
    TEST_GET_HOST(iut_host);
    TEST_GET_ADDR_NO_PORT(iut_addr);
    TEST_GET_HOST(tst_host);
    TEST_GET_ADDR_NO_PORT(tst_addr);

    CHECK_RC(te_kvpair_add(&expand_vars, "OLD_NET", "%s", net->cfg_net->name));
    /* FIXME: is there a better way to get the network prefix (IPv4 and IPv6)? */
    CHECK_RC(te_kvpair_add(&expand_vars, "NET_PREFIX", "%u", net->ip4pfx));
    CHECK_RC(te_kvpair_add(&expand_vars, "TA_IUT", "%s", iut_host->ta));
    CHECK_RC(te_kvpair_add(&expand_vars, "IF_IUT", "%s", iut_if->if_name));
    CHECK_RC(te_kvpair_add(&expand_vars, "IF_IUT_PCI", "%s",
                           iut_if_pci_info.pci_addr));
    CHECK_RC(te_kvpair_add(&expand_vars, "IF_IUT_DRIVER", "%s",
                           iut_if_pci_info.net_driver));
    CHECK_RC(te_kvpair_add(&expand_vars, "TA_TST", "%s", tst_host->ta));
    CHECK_RC(te_kvpair_add(&expand_vars, "IF_TST", "%s", tst_if->if_name));
    CHECK_RC(te_kvpair_add(&expand_vars, "VLAN_ID", "%u", 5));

    CHECK_RC(te_kvpair_add(&expand_vars, "VNI", "%u", TEST_VNI));
    CHECK_RC(te_kvpair_add(&expand_vars, "IF_IUT_TNL", "tunnel%u", TEST_VNI));
    CHECK_RC(te_kvpair_add(&expand_vars, "IF_TST_TNL", "tunnel%u", TEST_VNI));

    CHECK_RC(te_kvpair_add(&expand_vars, "IF_IUT_BR", "br_iut_%u",
                           iut_if->if_index));

    CHECK_RC(te_sockaddr_h2str_buf(tst_addr, addr_buf, sizeof(addr_buf)));
    CHECK_RC(te_kvpair_add(&expand_vars, "TNL_IUT_REMOTE", "%s", addr_buf));
    CHECK_RC(te_sockaddr_h2str_buf(iut_addr, addr_buf, sizeof(addr_buf)));
    CHECK_RC(te_kvpair_add(&expand_vars, "TNL_TST_REMOTE", "%s", addr_buf));

    mtu = TEST_MTU;
    CHECK_RC(te_kvpair_add(&expand_vars, "MTU", "%u", mtu));
    CHECK_RC(te_kvpair_add(&expand_vars, "TNL_LOWER_MTU", "%u",
                           TEST_TNL_LOWER_MTU));
    CHECK_RC(te_kvpair_add(&expand_vars, "VF_NUMBER", "%u",
                           TEST_VF_NUMBER));

    if (strlen(vm_os) > 0)
    {
        cfg_val_type    val_type = CVT_STRING;
        char           *ta_type = NULL;
        te_errno        rc;

        rc = cfg_get_instance_fmt(&val_type, &ta_type,
                                  "/local:/vm:x86_64_%s/rcf:", vm_os);
        if (rc != 0 || strlen(ta_type) == 0)
        {
            free(ta_type);
            TEST_SKIP("Test agent for %s is not available", vm_os);
        }
        free(ta_type);

        CHECK_RC(te_kvpair_add(&expand_vars, "VM_IUT",
                               "%s", TEST_VM_IUT));
        CHECK_RC(te_kvpair_add(&expand_vars, "VM_TEMPLATE",
                               "x86_64_%s", vm_os));
        CHECK_RC(te_kvpair_add(&expand_vars, "TE_IUT",
                               "%s", getenv("TE_IUT")));
        CHECK_RC(te_kvpair_add(&expand_vars, "TA_IUT_VM",
                               "%s", TEST_VM_TA));
    }

    if (with_dpdk)
    {
        tapi_cfg_ovs_cfg cfg;
        char **eal_argv = NULL;
        int eal_argc = 0;
        unsigned int i;

        CHECK_RC(tapi_rte_make_eal_args(&env, iut_rpcs, NULL, NULL, 0, NULL,
                                        &eal_argc, &eal_argv));

        if (eal_argc - 1 < 1)
            TEST_FAIL("At least 1 EAL argument expected");

        CHECK_RC(tapi_cfg_ovs_convert_eal_args(eal_argc - 1,
                    (const char *const *)eal_argv + 1, &cfg));
        for (i = 0; i < TAPI_CFG_OVS_CFG_DPDK_NTYPES; i++)
        {
           if (cfg.values[i] != NULL)
           {
                CHECK_RC(cfg_add_instance_fmt(NULL, CVT_STRING, cfg.values[i],
                                              "/agent:%s/ovs:/%s", iut_host->ta,
                                              tapi_cfg_ovs_cfg_name[i]));
           }
        }
    }

    CHECK_RC(cfg_process_history(config, &expand_vars));

    /*
     * /agent/interface does not have a dependency from
     * /agent/tunnel/vxlan and creation of VXLAN does not trigger
     * interfaces resync.
     * Attempt to add a dependency makes infinitive recursion in
     * Configurator and crashes it.
     */
    CHECK_RC(rc = cfg_synchronize("/:", TRUE));

    if ((rc = tapi_cfg_net_remove_empty()) != 0)
        TEST_FAIL("Failed to remove /net instances with empty interfaces");

    rc = tapi_cfg_net_reserve_all();
    if (rc != 0)
        TEST_FAIL("Failed to reserve all interfaces mentioned in networks "
                  "configuration: %r", rc);

    CHECK_RC(tapi_cfg_net_nodes_switch_pci_fn_to_interface(
                                                        NET_NODE_TYPE_INVALID));

    CHECK_RC(tapi_cfg_net_foreach_node(iut_node_set_mtu, &mtu));

    test_prepare_all_interfaces();

    CFG_WAIT_CHANGES;

    CHECK_RC(rc = cfg_synchronize("/:", TRUE));
    CHECK_RC(rc = cfg_tree_print(NULL, TE_LL_RING, "/:"));

    TEST_SUCCESS;

cleanup:
    te_kvpair_fini(&expand_vars);
    free(config);
    if (env_init)
    {
        TEST_END_ENV;
    }

    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
