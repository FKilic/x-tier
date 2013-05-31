#!/bin/bash

if [[ $# != 2 ]] 
then
        echo "Usage: run_performance_tests.sh <load_times> <execution_times>"
        exit
fi

echo "Each Module will be loaded $1 times and executed $2 times..."

DIR=$(pwd)

find . -name "*_performance.ko" | while read line; do
		cd $(dirname $line)
#		make 
		echo ""
		echo "================> $line <================"
		time $DIR/module_injector.sh $(basename $line) $1 $2
		echo "<================ $line ================>"
		cd $DIR
done
