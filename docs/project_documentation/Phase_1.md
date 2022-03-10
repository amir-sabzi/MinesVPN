# Documentation of MinesVPN project, 1st milestone
In this phase, we designed a few experiments to study hardware capabilities, and find software tools that we can use in MinesVPN project. I will describe each experiment separately and provide the results and our understanding of those results.

## MoonGen, Rate control and packet generation
We started this phase by using the [MoonGen](https://github.com/ubc-systopia/MoonGen-1) as our primary tool to generate packets at a desired rate. According to the MoonGen [paper](https://www.net.in.tum.de/fileadmin/bibtex/publications/papers/MoonGen_IMC2015.pdf), This tool can provide hardware timestamps with resolution of 6.4ns with our NICs (section 6.1 of paper). Besides that, MoonGen leverages Hardware Rate Control of intel NICs to enfore constant bit-rate (CBR) traffic shape.

### Experiment 1
Description:\
In this experiment, we used MoonGen to generate traffic at different rates (100Mbps, 1Gbps, 2Gbps, 4Gbps, 8Gbps) and transmit the traffic using intel X540 10G NICs and intel X710 40G NICs. Each NIC has two ports, we connected these two ports together directly. We send packets using one port and receive them at the other port. Then, we measured the transimmsion delay of each packet. 

Expectation:\
When we have a constant bit rate enforced by MoonGen, in an ideal scenario, we expect to observe equal delay for all packets.(which consists of time that it takes to process each packet, plus the time which is required for packets to travell through the wire). 

Results:\
The detailed results are accessible through this [link](https://github.com/ubc-systopia/minesvpn-benchmarking/blob/main/codes/data_analysis/1_2_experiment.ipynb). To describe the results briefly, MoonGen cannot provide constant delay for all packets. We can see a fair amount of deviation in timing of different packets. This brings us to the second experiment.

### Experiment 2
Description:\
In this experiment, we modified the timestamping [lua file](https://github.com/ubc-systopia/MoonGen-1/blob/master/libmoon/lua/interval_timestamping.lua) in libmoon project to calculate the inter-packet delays. In other words, we want to measure the time between tranmission of two consecutive packets. For both of available NICs, we generate packets with rates of (100Mbps, 1Gbps, 2Gbps, 4Gbps, 8Gbps) and then measure the intervals for packets at sender side. This means for each pair of packets transmitted consecutively, we calculate ``ts_{i+1} - ts_{i}``.

Expectation:\ 
If NIC enforces the constant bit-rate (CBR) accurately, we expect to have constant, equal inter-arrival times for all packets leading to a completely constant rate of traffic.

Results:\
The results are available [here](https://github.com/ubc-systopia/minesvpn-benchmarking/blob/main/codes/data_analysis/1_2_experiment.ipynb). Based on this result, we have majority of packets with intervals as small as 1ns. Considering the fact that our NIC have the resolution of 6.4ns for timestamping packets, this small numbers are not valid. This means either NIC is sending packets in bursts with rate higher than the rate enforced by MoonGen, or timestamps are not correct. To investigate the potential reasons for this observation, we designed the 3rd experiment.

### Experiment 3
This experiment was designed to investigate potential reasons for having extremely small intervals in MoonGen packet generation. We tried multiple things and here I will list our findings.

* Batch size: DPDK sends packets in form of batches to increase the overall throughput. We assumed that sending packets in batches might lead to short intervals between packets in the same batch. To address this problem, we decreased the batch size to 1 packet per batch. There was no changes in final result. 
* Sampled timestamping: We realized that MoonGen doesn't timestamps all of the packets. It establishes two different queues and send packets without timestamp in one of them and timestamped packets, with lower rate, in the other one. This way of timestamping packets prevent us from calculating the accurate inter-arival times. However, this method should lead to higher delays compared to a situation in which we timestamp all packets. Surprisingly, this change neither decrease the inter-arrival times nor increase them.
* Limiting the software rate: We even went further and tried to limit the rate of packet generation in the software by adding delay inside the loop that generates packets in the lua code. Even this change didn't solve the problem! We end up considering this behavior as a bug/misconfiguration of MoonGen.

### Experiment 4

