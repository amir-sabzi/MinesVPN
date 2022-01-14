## Code and Result
### Software timestamping
#### Results
All NICs, even 1Gs, can perform software timestamping properly. Based on what I've understood so far, this is the most accurate timestamp that can be provided by the third-party apps (e.g. tcpdump, wireshark). 

### Hardware timestamping
#### Description
Based on the ``ethtool -T <NIC name>`` output, all NICs can support simplest version of hardware timestamping. The problem here is that none of the NICs has ``HWTSTAMP_FILTER_ALL`` as an option for ``rx_filter``, and this is the reason we fail to pass this test in MoonGen measurements for timestamp capabilities.\
Consequently, we should narrow our scope to a particular layer (e.g L2 or L4).
#### Results
I Can hw-timestamp outgoing packets on 1G NIC by setting ``htwstamp_config.tx_type = HWSTAMP_TX_ON`` at the sender side. The problem is that there is no similar option at the receiver side for hw-timestamping incoming packets. According to the linux networking documentation, hw-timestamping for incoming packets is controlled with the ``htwstamp_config.rx_filter``:
```
/* possible values for hwtstamp_config->rx_filter */
enum {
        /* time stamp no incoming packet at all */
        HWTSTAMP_FILTER_NONE,

        /* time stamp any incoming packet */
        HWTSTAMP_FILTER_ALL,

        /* return value: time stamp all packets requested plus some others */
        HWTSTAMP_FILTER_SOME,

        /* PTP v1, UDP, any kind of event packet */
        HWTSTAMP_FILTER_PTP_V1_L4_EVENT,

        /* for the complete list of values, please check
        * the include file include/uapi/linux/net_tstamp.h
        */
};
```
The ``HWTSTAMP_FILTER_ALL`` is not available in any of our interfaces. I used other available filters but none of them has worked for 1G NICs so far.
