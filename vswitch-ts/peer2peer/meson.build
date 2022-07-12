# SPDX-License-Identifier: Apache-2.0
# (c) Copyright 2016 - 2022 Xilinx, Inc. All rights reserved.

tests = [
    'p2p_prologue',
    'tcp_conn',
    'udp_conn',
    'tcp_udp_perf',
    'nginx_perf_wrk',
    'ping',
]

foreach test : tests
    test_exe = test
    test_c = test + '.c'
    package_tests_c += [ test_c ]
    executable(test_exe, test_c, install: true, install_dir: package_dir,
               dependencies: test_deps)
endforeach

tests_info_xml = custom_target(package_dir.underscorify() + 'tests-info-xml',
                               install: true, install_dir: package_dir,
                               input: package_tests_c,
                               output: 'tests-info.xml', capture: true,
                               command: [ te_tests_info_sh,
                                          meson.current_source_dir() ])

cs_conf = [
    'geneve.yml',
    'vlan.yml',
    'vm.yml',
    'vm_ovs_bridge.yml',
    'vm_ovs_bridge_geneve.yml',
    'vm_ovs_bridge_vlan.yml',
    'vm_ovs_bridge_vxlan.yml',
    'vm_ovs_dpdk_bridge.yml',
    'vm_ovs_dpdk_bridge_geneve.yml',
    'vm_ovs_dpdk_bridge_vlan.yml',
    'vm_ovs_dpdk_bridge_vxlan.yml',
    'vm_ovs_dpdk_rep_bridge.yml',
    'vm_pass_through.yml',
    'vxlan.yml',
    'ovs_bridge.yml',
    'ovs_dpdk_rep_bridge.yml',
    'inc.tst_system_geneve.yml',
    'inc.tst_system_vxlan.yml',
    'inc.iut_vm_pci_passthrough.yml',
    'inc.iut_vm_tap.yml',
    'inc.iut_vm_vhost_user.yml',
    'inc.iut_ovs_dpdk_bridge_tunnel.yml',
    'inc.iut_ovs_bridge_tunnel.yml',
]

install_data([ 'package.dox', 'package.xml', cs_conf ],
             install_dir: package_dir)
