#!/bin/sh


PROGDIR=`dirname $0`
TEDEVALJAR=$PROGDIR/tedeval-2.2.jar
TEDEVALJARAPP=$PROGDIR/TedEvalApps.jar


PREF=$FINAL/READY_TO_SHIP_FINAL/FRENCH_SPMRL/
GOLDCONLL=$PREF/gold/conll/test/test.French.gold.conll
GOLDPTB=$PREF/gold/ptb/test/test.French.gold.ptb
TESTCONLL=$1
TESTPTB=$2

#java -Xmx768m   -cop /Archive/workspace/unipar/bin/  applications.Dtreebank2Ftreebank


cat $GOLDPTB | perl -pe 's/^\( /(TOP /' > $GOLDPTB.4tedeval
cat $TESTPTB | perl -pe 's/^\( /(TOP /' > $TESTPTB.4tedeval.noeval
GOLDPTB=$GOLDPTB.4tedeval
TESTPTB=$TESTPTB.4tedeval.noeval

java -Xmx768m   -cp $TEDEVALJARAPP:$TEDEVALJAR:. applications.Dtreebank2Ftreebank $GOLDCONLL $GOLDCONLL.ftrees

java -Xmx768m   -cp $TEDEVALJARAPP:$TEDEVALJAR:. applications.Dtreebank2Ftreebank $TESTCONLL $TESTCONLL.ftrees

java -Xmx768m   -jar $TEDEVALJAR -p1 $TESTCONLL.ftrees -g1 $GOLDCONLL.ftrees -o1 $TESTCONLL.tedeval-res -p2 $TESTPTB -g2 $GOLDPTB -o2 $TESTPTB.tedeval-crossfram.res
