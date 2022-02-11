#!/bin/bash

#This helper has been written to make things ready for 4th experiment. We need to configure interfaces for this experiment at sender (leap-417) and receiver (leap-418) sides. This configuration includes checking to see whether they are ready to be bounded and assign them IP addresses.


ipAddr_417="192.168.1.17/24"
ipAddr_418="192.168.1.18/24"

intName_417="enp8s0f1"
intName_418="enp8s0f1"

if [[ $(uname -n) == "leap-418" ]]; then
  echo "I am 418"
  sudo ifconfig $intName_418 $ipAddr_418
elif [[ $(uname -n) == "leap-417" ]]; then
  echo "I am 417" 
  sudo ifconfig $intName_417 $ipAddr_417
fi
