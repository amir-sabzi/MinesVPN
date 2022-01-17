# Documentation
I will use this .md file as documentation of the project. I chose git readme format because it available me to properly mention commands and write everything in the terminal.
## Useful information about testbed
* Leap-417: 
  * 1G NIC: Broadcom Inc. and subsidiaries NetXtreme BCM5720 2-port Gigabit Ethernet PCIe
    * eno1: 10.34.15.17
    * eno2: NULL
  * 10G NIC: Intel Corporation Ethernet Controller 10-Gigabit X540-AT2 
    * enp66s0f0: 10.34.15.177
    * enp66s0f1: NULL
  * 40G NIC: Intel Corporation Ethernet Controller XL710 for 40GbE QSFP+
    * enp8s0f0: NULL
    * enp8s0f1: NULL

* Leap-418:
  * 1G NIC: Broadcom Inc. and subsidiaries NetXtreme BCM5720 2-port Gigabit Ethernet PCIe
    * eno1: 10.34.15.18
    * eno2: NULL
  * 10G NIC: Intel Corporation Ethernet Controller 10-Gigabit X540-AT2 
    * enp66s0f0: 10.34.15.188
    * enp66s0f1: NULL
  * 40G NIC: Intel Corporation Ethernet Controller XL710 for 40GbE QSFP+
    * enp8s0f0: NULL
    * enp8s0f1: NULL
## Useful commands for experiments
To quickly chekc almost all information about NIC status and details, you can use following command:
```
sudo lspci -v | grep "Ethernet" -A 10
```
Although it's possible to use `` ifconfig `` to simply check the status of interfaces and their addresses, sometimes ``ip a`` can provide more information.\
To see whether an interface supporst hardware-timestamping or not, use the following command.
```
ethtool -T <interface name>
```
I wrote a bash script that can make it easier to check interfaces and setup them initially. ** add address of the file**

## Building MoonGen
When building MoonGen, if you encounter the following error:
```
error while loading shared libraries: libtbbmalloc.so.2: cannot open shared object file: No such file or directory
```
Modify the `` CMakeLists.txt `` and change the libraries as what follows:
```
set(libraries
-Wl, –disable-new-dtags
-Wl, –whole-archive
moon
)
```
Don't forgot to build from scratch.
