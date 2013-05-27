#!/bin/bash

# Inlcude
source $(dirname $0)/functions.sh

# ARGUMENTS
INJECTION_HELPER="X-TIER"
KVM_PATH="kvm"
QEMU_PATH="qemu"

# Turn Output off
set +v

# Build Modules and QEMU
echo ""
printStep " Building KVM and QEMU"
printSubStep "1" "4" "Removing injection file for building KVM"
executeCommand "rm $INJECTION_HELPER/X-TIER.o"
finishSubStep false

printSubStep "2" "4" "Building KVM"
executeCommand "cd $KVM_PATH && make"
finishSubStep true

printSubStep "3" "4" "Removing injection file for building QEMU"
executeCommand "rm $INJECTION_HELPER/X-TIER.o"
finishSubStep false

printSubStep "4" "4" "Building QEMU"
executeCommand "cd $QEMU_PATH && make"
finishSubStep true
finishStep
