import enum
from nis import match
import sys
import time
from subprocess import Popen, PIPE
import shlex
import enum
import re
class modes(enum.Enum):
    none = 1
    cap = 2
    fltr = 3
    Tx_tmp = 4

def ethtoodResultParser(result):
    capabilities = []
    filters = []
    tx_modes = []
    mode = modes.none
    raw_caps = []
    raw_fltrs = []
    raw_txModes = []
    for line in result.splitlines():
        if(line == "Capabilities:"):
            mode = modes.cap
        elif(line == "Hardware Transmit Timestamp Modes:"):
            mode = modes.Tx_tmp
        elif(line == "Hardware Receive Filter Modes:"):
            mode = modes.fltr
        match mode:
            case modes.cap:
                raw_caps.append(line) 
            case modes.Tx_tmp:
                raw_txModes.append(line)
            case modes.fltr:
                raw_fltrs.append(line)

    for line in raw_caps:
        tmp = line.split()
        if(len(tmp)==2):
            tmp = re.sub("[()]", "", tmp[1])
            capabilities.append(tmp)
    for line in raw_txModes:
        tmp = line.split()
        if(len(tmp)==2):
            tmp = re.sub("[()]", "", tmp[1])
            tx_modes.append(tmp)
    for line in raw_fltrs:
        tmp = line.split()
        if(len(tmp)==2):
            tmp = re.sub("[()]", "", tmp[1])
            filters.append(tmp)
       
    print(capabilities)
def runCommand(cmd):
    output = Popen(cmd,stdout=PIPE,shell=True)
    output_str = output.communicate()[0].decode('utf-8') 
    ethtoodResultParser(output_str)


def main():
    inteface_name = "enp8s0f1"
    cmd = "ethtool -T " + inteface_name
    runCommand(cmd)

if __name__ == '__main__':
    main()