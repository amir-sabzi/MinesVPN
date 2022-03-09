#!/bin/bash

# In this file, I will investigate the effect of load on the time between packets.
# data should be used with a python script to be reprepsented as a histogram.
# Test duration in seconds

# Termination function



# Rates you want to test with:
intervals=(100)
interval_itr=1;

cd ~/msc-project/minesvpn-benchmarking/codes/socket_code/
if [$? -ne 0]; then
  echo "~/msc-project/minesvpn-benchmarking/codes/socket_code/"
  exit 1
fi

make clean
make  
echo "Experiment 4  is started..."
  for interval in ${intervals[@]}; do
    for i in $(seq 1 $interval_itr); do
    echo "The ${i}th iteration for interval $interval is running."
    log_name="/home/asabzi/data/additional_exp/10g-NIC/userspace_ts/interval_${interval}us_${i}_$(date +"%Y-%m-%d_%I:%M-%p").csv"
    sudo ./simple_ts $interval $log_name -s
    sleep 5
    echo "Experiment for interval $interval has been performed successfully."
  done
done
