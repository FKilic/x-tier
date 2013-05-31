#!/bin/bash

# Clam AV Scanner

# PARAMS
UBUNTU_64="/local/vms/ubuntu-11-64-server"
MOUNT="/local/vm_hds"
NBD="/dev/nbd0"

# FUNCTIONS
checkError()
{
	if  [[ $? -ne 0 ]]
	then
		echo "An error occurred aborting."
		exit 0
	fi
}

verifyCmd()
{
	if [[ $(echo $2 | egrep "\([A-Za-z0-9_-/]+\)" | wc -l) != 1 ]]
	then
		echo "Filename '$2' seems to be invalid!"
		exit 0
	fi
}

# Args
if [[ $# != 2 ]]
then
	echo "Usage clamav.sh 1=Ubuntu-64|2=Windows32 <path>"
	exit 0
fi

# Map Drive Accoring to OS
if [[ $1 == 1 ]]
then
	# Ubuntu 64-bit
  
	# Mount Image
	qemu-nbd --read-only -c $NBD $UBUNTU_64
	checkError
	vgscan
	checkError
	vgchange -ay $(basename $UBUNTU_64)
	checkError
	mount -o ro,noload /dev/$(basename $UBUNTU_64)/root $MOUNT/$(basename $UBUNTU_64)
	checkError
	
	# Verify the command
	verifyCmd
	
	# Check
	SCAN=$(clamscan --no-summary --database=db.cvd $2 | grep FOUND | wc -l)
	
	# Unmount Image
	umount $MOUNT/$(basename $UBUNTU_64)
	checkError
	vgchange -an $(basename $UBUNTU_64)
	checkError
	qemu-nbd -d $NBD
	checkError
	
	# return
	exit $SCAN
fi

echo "Unsupported Guest System!"