#!/bin/bash

# first param - number of consumers
# second param - number of producers
# third param - storage capacity

var=3

if [ "$#" != "3" ];
then
	echo "Usage: start.sh ConsumerNum ProducerNum StorageCapacity"
	exit
fi

for i in $(seq 1 $1)
do
	./prod c &
done

for i in $(seq 1 $2)
do
	./prod p &
done

./man $[$1+$2] $3
