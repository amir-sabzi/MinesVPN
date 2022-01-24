#!/bin/bash

# In this file, I will test the effect of load on the latency.
# I will increase the load on the NIC to saturation level, and then, 
# measure the the effect on the packet delay.

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



# Rates you want to test with:
Rates=(100 1000 2000 4000 8000 16000 32000)


echo "Experiment is started..."
for rate in ${Rates[@]}; do
  echo "rate is $rate"
  cd ~/msc-project/MoonGen-1/
  if [$? -ne 0]; then
    echo "There is no directory ~/msc-project/MoonGen-1/"
    break
  fi
  terminate_moongen &
  sudo ./build/MoonGen examples/l2-load-latency.lua 0 1 --rate "$rate"

  sleep 60
  sudo mv histogram.csv histogram_{$rate}.csv
  sudo mv histogram_{$rate}.csv ~/data/1-exp/10g-NIC/
  echo "Experiment for rate $rate has been performed successfully."
done
