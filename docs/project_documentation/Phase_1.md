# Documentation of MinesVPN project, 1st milestone
In this phase, we designed a few experiments to study hardware capabilities, and find software tools that we can use in MinesVPN project. I will describe each experiment separately and provide the results and our understanding of those results.

## MoonGen, Rate control and packet generation
We started this phase by using the [MoonGen](https://github.com/ubc-systopia/MoonGen-1) as our primary tool to generate packets at a desired rate. According to the MoonGen [paper](https://www.net.in.tum.de/fileadmin/bibtex/publications/papers/MoonGen_IMC2015.pdf), This tool can provide hardware timestamps with resolution of 6.4ns with our NICs (section 6.1 of paper). Besides that, MoonGen leverages Hardware Rate Control of intel NICs to enfore constant bit-rate (CBR) traffic shape.

### Experiment 1
In this experiment, we used MoonGen to generate traffic at different rates (100Mbps, 1Gbps, 2Gbps, 4Gbps, 8Gbps) and transmit the traffic using intel X540 10G NICs and intel X710 40G NICs. Each NIC has two ports, we connected these two ports together directly. We send packets using one port and receive them at the other port. Then, we measured the transimmsion delay of each packet. When we constant bit rate enforced by MoonGen, in an ideal scenario, we expect to observe equal delay for all packets.(which consists of time that it takes to process each packet, and the time which is required for packets to travell through the wire). 
