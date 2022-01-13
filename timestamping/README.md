## Code and Result
### Software timestamping
All NICs, even 1Gs, can perform software timestamping properly. Based on what I've understood so far, this is the most accurate timestamp that can be provided by the third-party apps (e.g. tcpdump, wireshark). 

### Hardware timestamping
Based on the ``ethtool -T <NIC name>`` output, all NICs can support simplest version of hardware timestamping. The problem here is that none of the NICs has ``HWTSTAMP_FILTER_ALL`` as an option for ``rx_filter``, and this is the reason we fail to pass this test in MoonGen measurements for timestamp capabilities.\
Consequently, we should narrow our scope to a particular layer (e.g L2 or L4).
