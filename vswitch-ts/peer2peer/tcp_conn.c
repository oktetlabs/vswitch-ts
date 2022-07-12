/* SPDX-License-Identifier: Apache-2.0 */
/* (c) Copyright 2016 - 2022 Xilinx, Inc. All rights reserved. */
/*
 * vSwitch Test Suite
 * Peer to peer configuration.
 */

/** @page peer2peer-tcp_conn TCP connection test
 *
 * @objective Test TCP connection
 *
 * @param packets_num           Number of packets to transmit
 * @param client_packets        List of packet size for client to transmit
 * @param server_packets        List of packet size for server to transmit
 *
 * @type performance
 *
 * @author Ivan Ilchenko <Ivan.Ilchenko@oktetlabs.ru>
 *
 * Establish TCP connection and transmit packets with specified sizes.
 *
 * @par Scenario:
 */

#define TE_TEST_NAME  "peer2peer/tcp_conn"

#include "vswitch_test.h"
#include "tapi_sockaddr.h"
#include "tapi_rpc_params.h"
#include "tapi_rpc_socket.h"
#include "tapi_rpcsock_macros.h"

#define MAX_PACK 1024
static char buff[MAX_PACK];

static int
send_recv_pack(rcf_rpc_server *sender_rpcs, int sender_sk,
               rcf_rpc_server *recv_rpcs, int recv_sk, int packet_size)
{
    struct rpc_iovec iov = { .iov_base = buff, .iov_len = packet_size,
                             .iov_rlen = packet_size };
    struct rpc_mmsghdr mmsghdr = { .msg_hdr =
                                        {
                                        .msg_iov = &iov, .msg_iovlen = 1,
                                        .msg_riovlen = 1,
                                        .msg_name = NULL, .msg_control = NULL,
                                        .msg_flags = 0
                                        },
                                   .msg_len = 0
                                  };
    struct tarpc_timespec timeout = { .tv_sec = 1, .tv_nsec = 0 };

    if (MAX_PACK < packet_size)
        return -1;
    if (packet_size == 0)
        return 0;
    if (rpc_send(sender_rpcs, sender_sk, buff, packet_size, 0) != packet_size)
        return -1;

    if (rpc_recvmmsg_alt(recv_rpcs, recv_sk, &mmsghdr, 1, 0, &timeout) == -1)
        return -1;
    if (mmsghdr.msg_len != (unsigned int)packet_size)
        return -1;

    return 0;
}

int
main(int argc, char *argv[])
{
    rcf_rpc_server                         *server_rpcs = NULL;
    rcf_rpc_server                         *client_rpcs = NULL;
    const struct sockaddr                  *server_addr = NULL;
    const struct sockaddr                  *client_addr = NULL;
    int                                    *client_packets, *server_packets;
    int                                     serv_sk   = -1;
    int                                     client_sk = -1;
    int                                     accept_sk = -1;
    int                                     p, packets_num;
    int                                     domain;

    TEST_START;
    TEST_GET_PCO(server_rpcs);
    TEST_GET_PCO(client_rpcs);
    TEST_GET_ADDR(server_rpcs, server_addr);
    TEST_GET_ADDR(client_rpcs, client_addr);

    TEST_GET_INT_LIST_PARAM(server_packets, packets_num);
    TEST_GET_INT_LIST_PARAM(client_packets, packets_num);

    switch (server_addr->sa_family)
    {
        case AF_INET:
            domain = RPC_PF_INET;
            break;
        case AF_INET6:
            domain = RPC_PF_INET6;
            break;
        default:
            TEST_FAIL("Unknown protocol family");
    }

    TEST_STEP("Set server to listen");
    serv_sk = rpc_socket(server_rpcs, domain, RPC_SOCK_STREAM,
                        RPC_PROTO_DEF);
    rpc_bind(server_rpcs, serv_sk, server_addr);
    rpc_listen(server_rpcs, serv_sk, 64);

    TEST_STEP("Set client to connect");
    client_sk = rpc_socket(client_rpcs, domain, RPC_SOCK_STREAM,
                        RPC_PROTO_DEF);
    RPC_AWAIT_IUT_ERROR(client_rpcs);
    rc = rpc_connect(client_rpcs, client_sk, server_addr);
    if (rc != 0)
        TEST_FAIL("Client failed to connect to the server");

    TEST_STEP("Accept connection");
    accept_sk = rpc_accept(server_rpcs, serv_sk, NULL, NULL);

    TEST_STEP("Send/recv packets");
    for (p = 0; p < packets_num; p++)
    {
        CHECK_RC(send_recv_pack(client_rpcs, client_sk, server_rpcs, accept_sk,
                    client_packets[p]));
        CHECK_RC(send_recv_pack(server_rpcs, accept_sk, client_rpcs, client_sk,
                    server_packets[p]));
    }

    TEST_SUCCESS;

cleanup:
    CLEANUP_RPC_CLOSE(server_rpcs, serv_sk);
    CLEANUP_RPC_CLOSE(server_rpcs, accept_sk);
    CLEANUP_RPC_CLOSE(client_rpcs, client_sk);

    TEST_END;
}
