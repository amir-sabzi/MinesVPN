# KVM Installation Procudere
Among all different different solutions to install the KVM, I found [this](https://phoenixnap.com/kb/ubuntu-install-kvm) one straightforward. 

Here is the command that I used to create the first instance of virtual machine:
```
sudo virt-install \
  --name ubuntu-guest \
  --os-variant ubuntu20.04 \ 
  --vcpus 2 \
  --ram 2048 \ 
  --location http://ftp.ubuntu.com/ubuntu/dists/focal/main/installer-amd64/ \
  --network bridge=virbr0,model=virtio \
  --graphics none \
  --extra-args='console=ttyS0,115200n8 serial' 
```

# VM Management
Withe the following command we can access to the IP address of each VM to use for ssh.
```
virsh net-dhcp-leases default
```

