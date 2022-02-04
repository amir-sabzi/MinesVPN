## Experiment description 
In this experiment, we measured the latency of transmiting a packet between two ports of the same NIC. Ports are connected directly with a single wire without any switches in between. In an ideal scenario, we expect to have a constant letancy for all packets when we are sending them with a constant rate. To do this, I used both X540 and X740 NICs, and transmitted Ethernet (L2) and UDP (L3) traffic. Therefore, there should be 4 different classes of results for this experiment.

## Experiment results
Although in the paper, we had seen a similar experiment, results were different with ours. The authors' results represent that range of changes in transmission time is about 64ns. Our results show a larger value. You can find them [here](https://colab.research.google.com/drive/1Kml6L51Q2PCtcDbnMXtRswcVfL7ymM7C#scrollTo=Ya07FXd_V9s4&uniqifier=1) at the colab.
