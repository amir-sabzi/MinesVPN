## Meeting Tue 18 Jan
### Things that I've done so far:
* I build MoonGen, there was a problem in the repository CMakelist.txt.You can find the explanation in the documentation section of the repository.
* Using the timestamping capabilities of the MoonGen, I wrote and script to generate l3 load with different rates, and then measured the packet latencies for all packets. I plotted the corresponding histogram for each of them to be compared.
* I wrote a c socket program to implement timestamping by hand. This code has two sender and receiver modes. It gives us a good understanding of the way we can leverage timestamping capabilities from userspace with socket programming. I will add the documentation for this code in the corresponding repository.

### Things that we should discuss in the meeting:
* Running the first experiment on MoonGen, I faced a problem. When I increase the rate more than a particular threshold, about 6630 Mbps, MoonGen fails to generate the desired rate. It is stuck at the 6630 rate and cannot change. With this issue, it's not possible to saturate the NIC with this rate. The reason is not clear to me but maybe the hardware fails to generate traffic at a higher rate!
* To timestamps packets that are being handled with sockets, two main things should be done: First, we should set parameters of the socket to capture timestamps. Second, we should make an IOCTL call to the NIC driver to active timestamping for outgoing and incoming packets. Here is the problem that arose:
  * This is the output of ``ethtool -T <NIC name>``:
  ```
  Time stamping parameters for enp66s0f0:
  Capabilities:
  hardware-transmit     (SOF_TIMESTAMPING_TX_HARDWARE)
  software-transmit     (SOF_TIMESTAMPING_TX_SOFTWARE)
  hardware-receive      (SOF_TIMESTAMPING_RX_HARDWARE)
  software-receive      (SOF_TIMESTAMPING_RX_SOFTWARE)
  software-system-clock (SOF_TIMESTAMPING_SOFTWARE)
  hardware-raw-clock    (SOF_TIMESTAMPING_RAW_HARDWARE)
  PTP Hardware Clock: 5
  Hardware Transmit Timestamp Modes:
  off                   (HWTSTAMP_TX_OFF)
  on                    (HWTSTAMP_TX_ON)
  Hardware Receive Filter Modes:
  none                  (HWTSTAMP_FILTER_NONE)
  ptpv1-l4-sync         (HWTSTAMP_FILTER_PTP_V1_L4_SYNC)
  ptpv1-l4-delay-req    (HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ)
  ptpv2-event           (HWTSTAMP_FILTER_PTP_V2_EVENT)
  ```
  It has all socket options, and we can enable timestamping at **sender side** by setting ``HWTSTAMP_TX_ON``. However, it is not possible to timestamp packets on the receiver side. After a little bit of search, I realized our NICs don't have the ``HWTSTAMP_FILTER_ALL`` as a receiver filter mode. I tested all other options for the filter mode but none of them works properly. We should discuss the way we can address this problem.
