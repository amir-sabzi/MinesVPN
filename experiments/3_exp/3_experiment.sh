#!/bin/bash

# In this experiment, I am measuring the interval delays using the built-in moongen script.
# data should be used with a python script to be reprepsented as a histogram.

# Test duration in seconds
Test_duration=180

# Termination function
terminate_moongen(){
  sleep $Test_duration
  T=$(ps axf | grep MoonGen | grep -v grep | awk 'FNR==2 {print $1}')
  sudo kill -SIGINT $T
  echo $?
  echo "The process $T is killed"
}

data_dict=~/data/3-exp/10G-NIC/l3/

# Rates you want to test with:
Rates=(100 1000 2000 4000)


echo "Experimenti 2  is started..."
for rate in ${Rates[@]}; do
  echo "rate is $rate"
  cd ~/msc-project/MoonGen-1/
  if [$? -ne 0]; then
    echo "There is no directory ~/msc-project/MoonGen-1/"
    break
  fi
  terminate_moongen &
  sudo ./build/MoonGen examples/l3-load-latency.lua 0 1 --rate "$rate"
  
  log_name="dist_${rate}Mbps_$(date +"%Y-%m-%d_%I:%M-%p").csv"
  sleep 60
  ls $data_dict
  if [$? -ne 0]; then
    echo "The directory $data_dict hasn't created before. Creating directory..."
    mkdir -p $data_dict
  fi
  sudo mv $log_name $data_dict 
  echo "Experiment for rate $rate has been performed successfully."
done
