#!/bin/bash
# use testnet settings,  if you need mainnet,  use ~/.beenodecore/beenoded.pid file instead
beenode_pid=$(<~/.beenodecore/testnet3/beenoded.pid)
sudo gdb -batch -ex "source debug.gdb" beenoded ${beenode_pid}
