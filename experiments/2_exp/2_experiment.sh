#!/bin/bash

# In this file, I will investigate the effect of load on the time between packets.
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
  sudo ./build/MoonGen examples/l2-load-latency.lua 0 1 --rate "$rate"
  
  log_name="dist_{$rate}Mbps_{$(date +"%Y-%m-%d_%I:%M-%p")}.csv"
  sleep 60
  awk -F "\"*,\"*" 'NR == 1{old = $1; next} {printf "%d%s %d\n", $1 - old, "," ,$2; old =$1;}'  histogram.csv | awk -F, '{a[$1]+=$2;}END{for(i in a)print i","a[i];}' > $log_name 
  sudo mv $log_name  ~/data/2-exp/40g-NIC/l2/
  echo "Experiment for rate $rate has been performed successfully."
done
