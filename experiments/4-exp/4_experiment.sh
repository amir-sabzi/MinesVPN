#!/bin/bash

# In this file, I will investigate the effect of load on the time between packets.
# data should be used with a python script to be reprepsented as a histogram.
# Test duration in seconds

# Termination function



# Rates you want to test with:
intervals=(10 20 50 100 200 500 1000 2000 5000 10000 20000 50000 100000)


echo "Experiment 4  is started..."
for interval in ${intervals[@]}; do
  echo "interval is $interval"
  cd ~/msc-project/minesvpn-benchmarking/codes/socket_code/
  if [$? -ne 0]; then
    echo "~/msc-project/minesvpn-benchmarking/codes/socket_code/"
    break
  fi
  make clean
  make  
  log_name="/home/asabzi/data/4-exp/40g-NIC/softstamp/interval_${interval}us_$(date +"%Y-%m-%d_%I:%M-%p").csv"
  sudo ./simple_ts $interval $log_name -s
  sleep 5
  echo "Experiment for interval $interval has been performed successfully."
done
