#!/bin/sh
#MALTHOME=$SHARED/TEDEVALSTUFF/maltparser-1.7.2  # to change to fit your own install
MALTHOME=~/public/workspace/Code/tedeval/maltparser-1.7.2

if test "$1" = "-ptb" ; then # do nothing if const. file
	echo "ptb file, doing nothing"  > /dev/stderr
	cat
else # reprojectivize the data
 java -jar $MALTHOME/maltparser-1.7.2.jar -c pproj_$$ -m proj -pp head -i /dev/stdin -o /dev/stdout
 rm -f pproj_$$.mco
 
fi


