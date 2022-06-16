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
sudo virt-install \
  --name PublicComponent \
  --os-variant ubuntu20.04 \ 
  --vcpus 4 \
  --ram 24576\ 
  --location http://ftp.ubuntu.com/ubuntu/dists/focal/main/installer-amd64/ \
  --network bridge=virbr0,model=virtio \
  --graphics none \
  --extra-args='console=ttyS0,115200n8 serial' 
```
