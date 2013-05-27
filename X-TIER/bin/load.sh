#!/bin/bash

# Inlcude
source $(dirname $0)/functions.sh

# ARGUMENTS
INJECTION_PATH="X-TIER"
KVM_PATH="kvm"
OS=$3
DEST=$4

# Turn Output off
set +v

# Obtain sudo
printStep " Getting higher privileges"
executeCommand "sudo echo -en TROGDOR the BURNINATOR!"
checkError true
finishStep

# Remove Modules
echo ""
printStep " Unloading KVM Modules"
printSubStep "1" "2" "Removing KVM-Intel"
executeCommand "sudo modprobe -r kvm-intel"
finishSubStep true

printSubStep "2" "2" "Removing KVM"
executeCommand "sudo modprobe -r kvm"
finishSubStep true
finishStep

# Copy Modules to TMP
echo ""
printStep " Copying KVM Modules to $4"
printSubStep "1" "2" "Copying KVM"
executeCommand "cp $KVM_PATH/x86/kvm.ko $4/kvm.ko"
finishSubStep true

printSubStep "2" "2" "Copying KVM-Intel"
executeCommand "cp $KVM_PATH/x86/kvm-intel.ko $4/kvm-intel.ko"
finishSubStep true
finishStep

# Changing Access Rights
echo ""
printStep " Changing owner of modules to $(whoami)"
printSubStep "1" "2" "Changing owner of KVM"
executeCommand "sudo chown $(whoami) $4/kvm.ko"
finishSubStep true

printSubStep "2" "2" "Changing owner of KVM-Intel"
executeCommand "sudo chown $(whoami) $4/kvm-intel.ko"
finishSubStep true
finishStep

# Loading modules
echo ""
printStep " Loading modules"
printSubStep "1" "2" "Loading KVM"
executeCommand "sudo insmod $4/kvm.ko"
finishSubStep true

printSubStep "2" "2" "Loading KVM-Intel"
executeCommand "sudo insmod $4/kvm-intel.ko"
finishSubStep true
finishStep

# Copy data
if [[ $OS == "linux" ]]
then
        #linux
        printStep " Copying Injection Data"
        printSubStep "1" "6" "Creating directory $4/linux"
        executeCommand "mkdir -p $4/linux"
        finishSubStep false

        printSubStep "2" "6" "Copying *.ko files"
        executeCommand "find $INJECTION_PATH/kernelmodule/linux/ -name '*.ko' | while read line; do cp \$line $4/linux/; done"
        finishSubStep true

        printSubStep "3" "6" "Copying *.inject files"
        executeCommand "find $INJECTION_PATH/kernelmodule/linux/ -name '*.inject' | while read line; do cp \$line $4/linux/; done"
        finishSubStep true

        printSubStep "4" "6" "Copying parser"
        executeCommand "cp $INJECTION_PATH/parser/linux/Debug/inject-parser $4/linux/"
        finishSubStep true
        
        printSubStep "5" "6" "Copying wrapper"
        executeCommand "rm -rf $4/wrapper && cp -r $INJECTION_PATH/wrapper $4/"
        finishSubStep true
        
        printSubStep "6" "6" "Copying scripts"
        executeCommand "rm -rf $4/scripts && cp -r $INJECTION_PATH/scripts $4/"
        finishSubStep true
        finishStep
else
        #win
        printStep " Copying Injection Data"
        printSubStep "1" "6" "Creating directory $4/win"
        executeCommand "mkdir -p $4/win"
        finishSubStep false

        printSubStep "2" "6" "Copying *.sys files"
        executeCommand "find $INJECTION_PATH/kernelmodule/win/ -name '*.sys' | while read line; do cp \$line $4/win/; done"
        finishSubStep true

        printSubStep "3" "6" "Copying *.inject files"
        executeCommand "find $INJECTION_PATH/kernelmodule/win/ -name '*.inject' | while read line; do cp \$line $4/win/; done"
        finishSubStep true

        printSubStep "4" "6" "Copying parser"
        executeCommand "cp $INJECTION_PATH/parser/win/* $4/win/"
        finishSubStep true
        
        printSubStep "5" "6" "Copying wrapper"
        executeCommand "rm -rf $4/wrapper && cp -r $INJECTION_PATH/wrapper $4/"
        finishSubStep true
        
        printSubStep "6" "6" "Copying scripts"
        executeCommand "rm -rf $4/scripts && cp -r $INJECTION_PATH/scripts $4/"
        finishSubStep true
        finishStep
fi




