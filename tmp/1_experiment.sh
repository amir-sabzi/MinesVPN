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
  cd ~/MoonGen
  if ["$?" -ne 0]; then
    echo "There is no directory ~/MoonGen"
    break
  fi
  terminate_moongen &
  sudo ./build/MoonGen examples/l2-load-latency.lua 0 1 --rate "$rate" 
  #bash --rcfile <(echo '. ~/.bashrc; sudo ./build/MoonGen examples/l3-load-latency.lua 0 1 --rate $rate')

  TMPFILE=$(mktemp)

  # Add stuff to the temporary file
  # echo "source ~/.bashrc" > $TMPFILE
  # echo "sudo ./build/MoonGen examples/l3-load-latency.lua 0 1 --rate $rate" >> $TMPFILE
  # echo "rm -f $TMPFILE" >> $TMPFILE

  # Start the new bash shell 
  # bash --rcfile $TMPFILE
  sleep 60
  sudo mv histogram.csv histogram_{$rate}.csv
  sudo mv histogram_{$rate}.csv ~/MinesVPN/tmp/log/l2/
  echo "Experiment for rate $rate has been performed successfully."
done
