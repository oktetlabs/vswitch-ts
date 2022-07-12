/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2016 - 2022 Xilinx, Inc. All rights reserved. */
/*
 * vSwitch Test Suite
 * Peer to peer configuration.
 */

/** @page peer2peer-nginx_perf_wrk Nginx performance test
 *
 * @objective Report nginx performance with wrk
 *
 * @param duration_sec      Duration of the benchmark in seconds
 * @param connections       Number of connections to keep open
 * @param n_wrk_threads     Number of wrk threads to use
 *
 * @type performance
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 *
 * Get the performance statistics of nginx server using wrk tool and report it.
 *
 * @par Scenario:
 */

#define TE_TEST_NAME  "peer2peer/nginx_perf_wrk"

#include "vswitch_test.h"
#include "te_units.h"
#include "tapi_sockaddr.h"
#include "tapi_rpc_params.h"
#include "tapi_cfg_nginx.h"
#include "tapi_wrk.h"
#include "te_mi_log.h"
#include "tapi_job_factory_rpc.h"

#define TEST_BENCH_DURATION_SEC 6
#define NGINX_NAME "webserver"
#define SRV_NAME "dflt"
#define LISTEN_NAME "1"

int
main(int argc, char *argv[])
{
    rcf_rpc_server                         *server_rpcs = NULL;
    rcf_rpc_server                         *client_rpcs = NULL;
    const struct sockaddr                  *server_addr = NULL;
    const char                             *server_addr_str = NULL;
    char                                   *http_url = NULL;

    tapi_wrk_opt                            wrk_opts = tapi_wrk_default_opt;
    tapi_wrk_app                           *wrk_app = NULL;
    tapi_wrk_report                         wrk_report;
    te_mi_logger                           *logger = NULL;
    tapi_job_factory_t                     *job_factory = NULL;

    TEST_START;
    TEST_GET_PCO(server_rpcs);
    TEST_GET_PCO(client_rpcs);
    TEST_GET_ADDR(server_rpcs, server_addr);

    CHECK_NOT_NULL(server_addr_str = te_sockaddr2str(server_addr));
    if (te_asprintf(&http_url, "http://%s", server_addr_str) < 0)
        TEST_FAIL("Failed to create url string");

    TEST_STEP("Set wrk options");
    wrk_opts.connections = TEST_UINT_PARAM(connections);
    wrk_opts.duration_s = TEST_UINT_PARAM(duration_sec);
    wrk_opts.n_threads = TEST_UINT_PARAM(n_wrk_threads);
    wrk_opts.host = http_url;
    wrk_opts.latency = TRUE;

    TEST_STEP("Set up wrk and nginx applications");
    CHECK_RC(tapi_job_factory_rpc_create(client_rpcs, &job_factory));
    CHECK_RC(tapi_wrk_create(job_factory, &wrk_opts, &wrk_app));
    CHECK_RC(tapi_cfg_nginx_add(server_rpcs->ta, NGINX_NAME));
    CHECK_RC(tapi_cfg_nginx_http_server_add(server_rpcs->ta, NGINX_NAME,
                                            SRV_NAME));
    CHECK_RC(tapi_cfg_nginx_http_listen_entry_add(server_rpcs->ta, NGINX_NAME,
                                                  SRV_NAME, LISTEN_NAME,
                                                  server_addr_str));

    TEST_STEP("Start applications and wait for wrk report");
    CHECK_RC(tapi_cfg_nginx_enable(server_rpcs->ta, NGINX_NAME));
    CHECK_RC(tapi_wrk_start(wrk_app));
    CHECK_RC(tapi_wrk_wait(wrk_app, -1));
    CHECK_RC(tapi_wrk_get_report(wrk_app, &wrk_report));

    TEST_STEP("Log performance statistics in MI format");
    CHECK_RC(te_mi_logger_meas_create("wrk", &logger));
    tapi_wrk_report_mi_log(logger, &wrk_report);

    TEST_SUCCESS;

cleanup:
    free(http_url);
    tapi_wrk_destroy(wrk_app);
    te_mi_logger_destroy(logger);
    if (server_rpcs != NULL)
    {
        tapi_cfg_nginx_disable(server_rpcs->ta, NGINX_NAME);
        tapi_cfg_nginx_del(server_rpcs->ta, NGINX_NAME);
    }
    tapi_job_factory_destroy(job_factory);

    TEST_END;
}
