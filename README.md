# VSWITCH-TS - test suite for virtual switches

VSwitch TS carries out testing and performance evaluation of virtual switches.
Currently, the test suite only covers Open vSwitch.

The tests are performed under configurations:

* with and without DPDK,
* with different tunneling protocols,
* with and without VMs as hosts.

OKTET Labs TE is used as an Engine that allows to prepare desired environment for every test.
This guarantees reproducible test results.

## License

See the LICENSE file in this repository
