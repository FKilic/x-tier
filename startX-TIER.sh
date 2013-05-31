#!/bin/bash

########################################################
#       X-TIER Start Script
#
#       This script will:
#           1) compile KVM and QEMU
#           2) load the KVM modules
#           3) copy the modules to the specified folder
#           4) execute the specified VM
# 
########################################################

#====================================================
#       Variables
#       Change these as you see fit.
#====================================================

# The folder where scripts and modules will reside.
# Scripts and wrapper will be copied to $DESTINATION_FOLDER/scripts and
# $DESTINATION_FOLDER/wrapper respectively, while the modules will be copied 
# to $DESTINATION_FOLDER/linux/ and $DESTINATION_FOLDER/win/.
DESTINATION_FOLDER="/tmp/"

# Define your VMs here
LINUX_CMD="-hda /local/vms/ubuntu-11-64-server -k de -monitor telnet:127.0.0.1:3333,server -serial pty -net nic,vlan=0 -net user,vlan=0,hostfwd=tcp::12345-:22 -m 1024 -enable-kvm"
WINDOWS_CMD="-hda /local/vms/windows-7-32-bit -k de -monitor telnet:127.0.0.1:3333,server -usbdevice tablet -m 1024 -enable-kvm"

#====================================================
#       Commands
#       You should not need to change anything below.
#====================================================

if [[ $# -lt 1 ]]
then
        echo ""
        echo "Which VM would you like to start?"
        echo "  -> Linux: 1"
        echo "  -> Windows: 2"
        echo ""  

        #Print Menu
        while read CHOICE
        do
                if [[ $CHOICE -gt 2 || $CHOICE -lt 1 ]]
                then
                        echo "Invalid Choice."
                        echo ""
                        echo "Which VM would you like to start?"
                        echo "  -> Linux: 1"
                        echo "  -> Windows: 2"
                        echo ""  
                else
                        break
                fi
        done
else
        CHOICE=$1                
fi


# Build modules
chmod +x X-TIER/bin/build.sh && X-TIER/bin/build.sh

# Check for errors
if  [[ $? -ne 0 ]]
then
    exit -1
fi

# Load modules and copy data
case $CHOICE in
        "1")
        chmod +x X-TIER/bin/load.sh && X-TIER/bin/load.sh linux $DESTINATION_FOLDER

        # Check for errors
        if  [[ $? -ne 0 ]]
        then
            exit -1
        fi
        
        cd qemu/x86_64-softmmu/ && ./qemu-system-x86_64 $LINUX_CMD;;

        "2")
        chmod +x X-TIER/bin/load.sh && X-TIER/bin/load.sh win $DESTINATION_FOLDER

        # Check for errors
        if  [[ $? -ne 0 ]]
        then
            exit -1
        fi
        
        cd qemu/x86_64-softmmu/ && ./qemu-system-x86_64 $WINDOWS_CMD;;

        *) echo "Unknown choice!";;
esac
