#!/bin/bash

if [[ $# != 3 ]] 
then
	echo "Usage: module_injector.sh <injection_times> <load_times> <execution_times>"
	exit
fi

echo "Module $1 will be loaded $2 times and executed $3 times..."

for (( i = 1; i <= $2; i++ ))
do
	sudo insmod $1 loop=$3
	
	if [[ $? != 0 ]]
	then
		exit
	fi

	sudo rmmod $1

	if [[ $? != 0 ]]
        then
                exit
        fi

	while [[ $(sudo tail -10 /var/log/kern.log | grep "Execution Time" | wc -l) == 0 ]]
	do
		:		
	done

	# Get exec time
        echo "---------------------------------"
        echo "Module: $1"
        echo "Load: $i"
        echo "Executions per Load: $3"
        sudo tail -10 /var/log/kern.log | grep "Execution Time"
        echo "----------------------------------"


done
