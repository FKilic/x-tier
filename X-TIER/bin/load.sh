#!/bin/bash

# Inlcude
source $(dirname $0)/functions.sh

# ARGUMENTS
INJECTION_PATH="X-TIER"
KVM_PATH="kvm"
OS=$1
DEST=$2

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
printStep " Copying KVM Modules to $DEST"
printSubStep "1" "2" "Copying KVM"
executeCommand "cp $KVM_PATH/x86/kvm.ko $DEST/kvm.ko"
finishSubStep true

printSubStep "2" "2" "Copying KVM-Intel"
executeCommand "cp $KVM_PATH/x86/kvm-intel.ko $DEST/kvm-intel.ko"
finishSubStep true
finishStep

# Changing Access Rights
echo ""
printStep " Changing owner of modules to $(whoami)"
printSubStep "1" "2" "Changing owner of KVM"
executeCommand "sudo chown $(whoami) $DEST/kvm.ko"
finishSubStep true

printSubStep "2" "2" "Changing owner of KVM-Intel"
executeCommand "sudo chown $(whoami) $DEST/kvm-intel.ko"
finishSubStep true
finishStep

# Loading modules
echo ""
printStep " Loading modules"
printSubStep "1" "2" "Loading KVM"
executeCommand "sudo insmod $DEST/kvm.ko"
finishSubStep true

printSubStep "2" "2" "Loading KVM-Intel"
executeCommand "sudo insmod $DEST/kvm-intel.ko"
finishSubStep true
finishStep

# Copy data
if [[ $OS == "linux" ]]
then
        #linux
        printStep " Copying Injection Data"
        printSubStep "1" "6" "Creating directory $DEST/linux"
        executeCommand "mkdir -p $DEST/linux"
        finishSubStep false

        printSubStep "2" "6" "Copying *.ko files"
        executeCommand "find $INJECTION_PATH/kernelmodule/linux/ -name '*.ko' | while read line; do cp \$line $DEST/linux/; done"
        finishSubStep true

        printSubStep "3" "6" "Copying *.inject files"
        executeCommand "find $INJECTION_PATH/kernelmodule/linux/ -name '*.inject' | while read line; do cp \$line $DEST/linux/; done"
        finishSubStep true

        printSubStep "4" "6" "Copying parser"
        executeCommand "cp $INJECTION_PATH/parser/linux/X-TIER_linux_parser $DEST/linux/"
        finishSubStep true
        
        printSubStep "5" "6" "Copying wrapper"
        executeCommand "rm -rf $DEST/wrapper && cp -r $INJECTION_PATH/wrapper $DEST/"
        finishSubStep true
        
        printSubStep "6" "6" "Copying scripts"
        executeCommand "rm -rf $DEST/scripts && cp -r $INJECTION_PATH/scripts $DEST/"
        finishSubStep true
        finishStep
else
        #win
        printStep " Copying Injection Data"
        printSubStep "1" "6" "Creating directory $DEST/win"
        executeCommand "mkdir -p $DEST/win"
        finishSubStep false

        printSubStep "2" "6" "Copying *.sys files"
        executeCommand "find $INJECTION_PATH/kernelmodule/win/ -name '*.sys' | while read line; do cp \$line $DEST/win/; done"
        finishSubStep true

        printSubStep "3" "6" "Copying *.inject files"
        executeCommand "find $INJECTION_PATH/kernelmodule/win/ -name '*.inject' | while read line; do cp \$line $DEST/win/; done"
        finishSubStep true

        printSubStep "4" "6" "Copying parser"
        executeCommand "cp $INJECTION_PATH/parser/win/* $DEST/win/"
        finishSubStep true
        
        printSubStep "5" "6" "Copying wrapper"
        executeCommand "rm -rf $DEST/wrapper && cp -r $INJECTION_PATH/wrapper $DEST/"
        finishSubStep true
        
        printSubStep "6" "6" "Copying scripts"
        executeCommand "rm -rf $DEST/scripts && cp -r $INJECTION_PATH/scripts $DEST/"
        finishSubStep true
        finishStep
fi




