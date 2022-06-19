# KVM Installation Procudere
Among all different different solutions to install the KVM, I found [this](https://phoenixnap.com/kb/ubuntu-install-kvm) one straightforward. 

Here is the command that I used to create the first instance of virtual machine:
```
sudo virt-install \
  --name=ubuntu-guest \
  --os-variant=ubuntu20.04 \ 
  --vcpus=2 \
  --ram=2048 \ 
  --location=http://ftp.ubuntu.com/ubuntu/dists/focal/main/installer-amd64/ \
  --network=bridge=virbr0,model=virtio \
  --graphics=none \
  --extra-args='console=ttyS0,115200n8 serial' 
```
**When you're installing a VM, make sure the SSH server has been installed and activated as an option in software installation tab.**

# VM Management
Withe the following command we can access to the IP address of each VM to use for ssh.
```
virsh net-dhcp-leases default
```
Having the IP address of the VM, if you have activated the SSH server during installation, you can access each VM easily with the `ssh username@<VM-IP>`.

# VM specification
We need two virtual machines running on each server. One of them is only responsible for making outgoing traffic DP, and the other one manages all other tasks in the machine.
We call the former one "Isolated Component" and the later one is being named "Public component". 

**Isolated Component specification**
```
sudo virt-install \
  --name=IsolatedComponent \
  --os-variant=ubuntu20.04 \ 
  --vcpus=2 \
  --ram=16384 \ 
  --location='http://ftp.ubuntu.com/ubuntu/dists/focal/main/installer-amd64/' \
  --network=bridge=virbr0,model=virtio \
  --graphics=none \
  --extra-args='console=ttyS0,115200n8 serial' 
```

**Public Component specification**
```
sudo virt-install\
  --name PublicComponent\
  --os-variant ubuntu20.04\ 
  --vcpus 4\
  --ram 24576\ 
  --location http://ftp.ubuntu.com/ubuntu/dists/focal/main/installer-amd64/\
  --network bridge=virbr0,model=virtio\
  --graphics none\
  --extra-args='console=ttyS0,115200n8 serial' 
```

# Virtual machines network
In this section I will describe the procedure of setting up the testbed and configuring the network for it.

## Network configuration
After installing two virtual machines, there would be only one network named default network with one virtual bridge called "virbr0". 
The default bridge has been configured with a .xml file located in `/etc/libvirt/qemu/networks/default.xml`. 
This files specifies the bridge name, forwarding mode, mac address, and the IP range of this network.
For the ease of management and guaranteeing isolation, we want two isolated public components to be connected to separated bridges.

The following command can list the networks that are available for virtual machines:
```
virsh net-list
```
Here, we want to create a new network, called "new", with a one bridge named "virbr1".
For now, we keep both virtual bridges on the same NIC, "eno1". 
In next steps of the project, we want to assign separated NICs to each bridge. 

In this part, I'll provide a step-by-step guide for setting up two virtual networks and assign each VM to one of them.
1. We need to make a copy of defualt network configuration file.
```
sudo cp /etc/libvirt/qemu/networks/default.xml /etc/libvirt/qemu/networks/new.xml
```
2. The new configuation file should be modified in admin mode. Make sure that you modified "uuid", "bridge name", "mac address", and IP address range.
After applying proper changes, save the file. 
3. Now, we should create a network based on the new configuration file. Use the following command to create the new network.
```
virsh net-create /etc/libvirt/qemu/networks/new.xml
```
4. To add auto address functionality, use the following command:
```
virsh net-autostart new
```
If you encounter any errors here, you should add a few empty lines to end of your network configuration file, `new.xml`, with the following command.
```
virsh net-edit new
<add empty lines to the end of this file>
<save>
```
Before moving to the next part, make sure you have restarted the libvirtd with the following command.
```
sudo systemctl restrart libvirtd
```
5. Now you have your new virtual netowrk ready. If you haven't create a VM yet, when configuring it, make sure to assign it to the new network by changing its network configuration:
```
sudo virt-install \ 
...
---network bridge=<name of the new bridge>, model=virtion \
...
```
If you created your VM before, and you want to change its virtual netowrk, VM configuration file should be modified.
```
virsh edit <VM Name>
```
In this file, search for interface name and chanage it to the name of interface in the new network configuration.
## IP addresses of all components 
We have two components on each machine, called isolated and public repectively.
To perform the initial testing and experiments, we are going to use following addresses for different components.
* Leap-417 (Middle box 1)
  * Isolated Component IP: 192.168.123.231
  * Public Component IP: 192.168.122.199
* Leap-418 (Middle box 2)
  * Isolated Component IP: 192.168.121.79
  * Public Component IP: 192.168.120.192

The username and password for all components for ssh connection are:
```
username: minesvpn
pass: minesvpn
```
