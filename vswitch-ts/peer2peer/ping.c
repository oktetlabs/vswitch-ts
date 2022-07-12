/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2016 - 2022 Xilinx, Inc. All rights reserved. */
/*
 * vSwitch Test Suite
 * Peer to peer configuration.
 */

/** @page peer2peer-ping Ping statistics test
 *
 * @objective Report ping statistics
 *
 * @author Ivan Ilchenko <Ivan.Ilchenko@oktetlabs.ru>
 *
 * Use ping tool to report packets transmission statistics
 *
 * @par Scenario:
 */

#define TE_TEST_NAME  "peer2peer/ping"

#include "vswitch_test.h"
#include "tapi_sockaddr.h"
#include "tapi_rpc_params.h"
#include "tapi_job_factory_rpc.h"
#include "signal.h"
#include "tapi_ping.h"
#include "tapi_wrk.h"

#define TIMEOUT_FOR_PACKETS_NUM(pnum) ((pnum - 1) * 1000 + 500)

int
main(int argc, char *argv[])
{
    rcf_rpc_server                         *server_rpcs = NULL;
    rcf_rpc_server                         *client_rpcs = NULL;
    const struct sockaddr                  *server_addr = NULL;
    unsigned int                            packets_num, packet_size;

    tapi_ping_opt                           ping_opts = tapi_ping_default_opt;
    tapi_ping_app                          *ping_app = NULL;
    tapi_ping_report                        ping_report;
    tapi_job_factory_t                     *job_factory = NULL;
    te_mi_logger                           *logger = NULL;

    TEST_START;
    TEST_GET_PCO(server_rpcs);
    TEST_GET_PCO(client_rpcs);
    TEST_GET_ADDR(server_rpcs, server_addr);
    TEST_GET_UINT_PARAM(packets_num);
    TEST_GET_UINT_PARAM(packet_size);

    TEST_STEP("Set ping options");
    ping_opts.destination = te_sockaddr_get_ipstr(server_addr);
    ping_opts.packet_size = packet_size;

    TEST_STEP("Set up ping application");
    CHECK_RC(tapi_job_factory_rpc_create(client_rpcs, &job_factory));
    CHECK_RC(tapi_ping_create(job_factory, &ping_opts, &ping_app));

    TEST_STEP("Start ping application and wait for its report");
    CHECK_RC(tapi_ping_start(ping_app));
    tapi_ping_wait(ping_app, TIMEOUT_FOR_PACKETS_NUM(packets_num));
    CHECK_RC(tapi_ping_stop(ping_app));
    CHECK_RC(tapi_ping_get_report(ping_app, &ping_report));

    TEST_STEP("Log ping report in MI format");
    CHECK_RC(te_mi_logger_meas_create("ping", &logger));
    tapi_ping_report_mi_log(logger, &ping_report);

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(tapi_ping_destroy(ping_app));
    te_mi_logger_destroy(logger);
    tapi_job_factory_destroy(job_factory);

    TEST_END;
}
