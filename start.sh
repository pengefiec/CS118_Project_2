#!/bin/bash

for i in {0..5}
do
	gnome-terminal -e "./myrouter $i"
	sleep 1
done
