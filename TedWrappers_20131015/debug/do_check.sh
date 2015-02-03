#!/bin/sh

for file in `ls *.lattices`; do
	echo "processing $file"
	./check_sourceid.pl $file
done

