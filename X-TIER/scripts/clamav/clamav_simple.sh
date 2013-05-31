#!/bin/bash

# Clam AV Scanner
more /tmp/scripts/clamav/main.hdb | grep $1 

if [[ $? == 1 ]]
then
  exit 0
fi

exit 1
