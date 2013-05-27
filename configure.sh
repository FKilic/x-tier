#!/bin/bash

########################################################
#       Simple Configuration Script for X-TIER
########################################################

#====================================================
#       Configuration Commands
#       Change these as you see fit.
#====================================================

CONFIGURE_KVM="configure"
CONFIGURE_QEMU="configure --target-list=x86_64-softmmu --enable-kvm"


#====================================================
#       Commands
#       You should not need to change anything below.
#====================================================

# Include helper functions
source $(dirname $0)/X-TIER/bin/functions.sh

echo ""
printStep " Configuring KVM & QEMU"
printSubStep "1" "2" "Configuring KVM..."
executeCommand "cd kvm && chmod +x configure && ./$CONFIGURE_KVM"
finishSubStep true

printSubStep "2" "2" "Configuring QEMU..."
executeCommand "cd qemu && chmod +x configure && ./$CONFIGURE_QEMU"
finishSubStep true
finishStep







