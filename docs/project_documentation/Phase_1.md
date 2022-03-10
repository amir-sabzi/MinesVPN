# Documentation of MinesVPN project, 1st milestone
In this phase, we designed a few experiments to study hardware capabilities, and find software tools that we can use in the MinesVPN project. I will describe each experiment separately and provide the results and our understanding of those results.

## MoonGen, Rate control, and a packet generation
We started this phase by using the [MoonGen](https://github.com/ubc-systopia/MoonGen-1) as our primary tool to generate packets at the desired rate. According to the MoonGen [paper](https://www.net.in.tum.de/fileadmin/bibtex/publications/papers/MoonGen_IMC2015.pdf), This tool can provide hardware timestamps with a resolution of 6.4ns with our NICs (section 6.1 of paper). Besides that, MoonGen leverages Hardware Rate Control of intel NICs to enforce constant bit-rate (CBR) traffic shape.

### Experiment 1
Description:\
In this experiment, we used MoonGen to generate traffic at different rates (100Mbps, 1Gbps, 2Gbps, 4Gbps, 8Gbps) and transmit the traffic using intel X540 10G NICs and intel X710 40G NICs. Each NIC has two ports, we connected these two ports directly. We send packets using one port and receive them at the other port. Then, we measured the transmission delay of each packet. 

Expectation:\
When we have a constant bit rate enforced by MoonGen, in an ideal scenario, we expect to observe equal delay for all packets. (which consists of the time that it takes to process each packet, plus the time which is required for packets to travel through the wire). 

Results:\
The detailed results are accessible through this [link](https://github.com/ubc-systopia/minesvpn-benchmarking/blob/main/codes/data_analysis/1_2_experiment.ipynb). To describe the results briefly, MoonGen cannot provide constant delay for all packets. We can see a fair amount of deviation in the timing of different packets. This brings us to the second experiment.

### Experiment 2
Description:\
In this experiment, we modified the timestamping [lua file](https://github.com/ubc-systopia/MoonGen-1/blob/master/libmoon/lua/interval_timestamping.lua) in the libmoon project to calculate the inter-packet delays. In other words, we want to measure the time between the transmission of two consecutive packets. For both available NICs, we generate packets with rates of (100Mbps, 1Gbps, 2Gbps, 4Gbps, 8Gbps) and then measure the intervals for packets at the sender side. This means for each pair of packets transmitted consecutively, we calculate ``ts_{i+1} - ts_{i}``.

Expectation:\
If NIC enforces the constant bit rate (CBR) accurately, we expect to have constant, equal inter-arrival times for all packets leading to a completely constant rate of traffic.

Results:\
The results are available [here](https://github.com/ubc-systopia/minesvpn-benchmarking/blob/main/codes/data_analysis/1_2_experiment.ipynb). Based on this result, we have the majority of packets with intervals as small as 1ns. Because our NIC has a resolution of 6.4ns for timestamping packets, these small numbers are not valid. This means either NIC is sending packets in bursts with a rate higher than the rate enforced by MoonGen, or timestamps are not correct. To investigate the potential reasons for this observation, we designed the 3rd experiment.

### Experiment 3
This experiment was designed to investigate potential reasons for having extremely small intervals in MoonGen packet generation. We tried multiple things and here I will list our findings.

* **Batch size**: DPDK sends packets in form of batches to increase the overall throughput. We assumed that sending packets in batches might lead to short intervals between packets in the same batch. To address this problem, we decreased the batch size to 1 packet per batch. There were no changes in the final result. 
* **Sampled timestamping**: We realized that MoonGen doesn't timestamp all of the packets. It establishes two different queues and sends packets without timestamp in one of them and timestamped packets, with a lower rate, in the other one. This way of timestamping packets prevents us from calculating the accurate inter-arrival times. However, this method should lead to higher delays compared to a situation in which we timestamp all packets. Surprisingly, this change neither decrease the inter-arrival times nor increase them.
* **Limiting the software rate**: We even went further and tried to limit the rate of packet generation in the software by adding delay inside the loop that generates packets in the Lua code. Even this change didn't solve the problem! We end up considering this behavior as a bug/misconfiguration of MoonGen.

### Experiments 4 and 5
Description:\
To timestamp packets from userspace, I wrote a [C script](https://github.com/ubc-systopia/minesvpn-benchmarking/blob/main/codes/socket_code/socket_timestamping.c). In this script I generate packets in a sender-loop, timestamp them, and send them with modifiable inter-arrival times at the software. In these experiments, I tried a set of values for inter-arrival times in the code and measured the intervals based on the hardware timestamps. This enables us to accurately measure the NIC behavior in terms of managing packets in its queues. To study the NIC behavior, we set a value in microseconds as the interval between two packets generated in the userspace. Then, we measure the intervals based on hardware timestamps provided by the NIC. We calculate the average, standard deviation, and drift of intervals compared to software intervals for these packets. To have reliable data, we repeated each experiment 5 times and reported the average of mean, std, and drift as the final value.

To see the effect of the NIC rate limiting on the inter-arrival times of packets, we repeat the same experiment by using the following command to enforce a rate of 100Mbps by changing the link mode.
```
sudo ethtool -s enp66s0f1 speed 100 duplex full autoneg on
```
In the end, we revised those experiments to also capture timing information about different phases of the code to measure the kernel spends to send packets generated in the userspace.

Expectations:\
At the low rates, we expect the NIC to send packets immediately after they are placed inside its queues. As a result, the time between two packets generated inside the loop in the software should be almost equal to the time between two consecutive packets transmitted by the NIC. Besides that, if avoid saturating the NIC queues, we expect to observe low variations in intervals.

Results:\
You can find the result [here]() and [here](). TODO: summarize the result.
