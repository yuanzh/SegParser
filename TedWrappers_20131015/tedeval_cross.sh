#!/bin/sh

# wrapper script to make tedeval work on SPMRL Shared task data set
# Djame Seddah

# version  August 19, 03:49
# fixing java.nullPointer exception (removing sentence 908 and 1889 from dev arabic pred+pred)
# version  August 18, 02:14
# fixing java.nullPointer exception (removing sentence 799 from test test arabic pred+pred)



#  see
# http://stackoverflow.com/questions/402377/using-getopts-in-bash-shell-script-to-get-long-and-short-command-line-options/7680682#7680682
# options  
# 	-d (debug version), -n  
# 	-labeled, -unlalebed (*)
#	-ptb, -conll (*)
# 	-ar  (for arabic, default hebrew)
#   -test  (test set gold file used, default dev)
#   -cut   (cut-off lenght + bad sentences removed, fixed?)
#  -predfile FILE   (predicted parsed file)
#  -predmap  FILE  (predicted mapping file) # if not given, calculated
#  -gold FILE  
# -begin  starting line to be evaluated, 1 if nothing
# -end  end+1 line to be evaluated	+1000000 if nothing 


#set -x

# VARIABLES
PROGDIR=`dirname $0`
TEDEVALJAR=$PROGDIR/tedeval-2.2.jar
TEDEVALJARAPP=$PROGDIR/TedEvalApps.jar
#TEDEVALJAR=$PROGDIR/tedeval.jar
LABEL="-unlabeled"
TYPE="conll"
TYPENAME=conll
ARG="-c"
LANGUAGE=""
SKIPFILE=/dev/null
PREF=test
DOCUT=1
CUTOFF=70  #should be a parameter, later...
PREDFILE=""
PREDMAP=""
PREDLAT=""
GOLDFILE=""
GOLDLAT=""
LATSUF=tobeparsed.gold_tagged+gold_token.lattices
START=1
END=100000  # let's hope that no file will ever be that long (linewise)
SPMRLDATA_ROOTDIR=$SHARED/SPMRL_FINAL/READY_TO_SHIP_FINAL/
FIVEK=""
GOLDPTB=""
TESTPTB=""

if [ -z $1 ] ; then
	echo "tedeval_cross.sh OPTIONS -g GOLDFILE -s SYSTEMFILE 
    -D | --debug 	 	use tedeval + debug outputs
    -n | --new 	   		use latest tedeval-2.2.jar (default is 2.1	
	-u | --unlabeled 	unlabeled evaluation  (default)	 
	-l | --labeled 	 	labeled evaluation
	-p | --ptb 	 		evaluate const. files
	-c | --conll 	 	evaluate conll files (default)
	-a | --arabic 	 	dev mode for Arabic, do not use 
	-h | --hebrew 	 	dev mode for Hebrew, do not use 
	-y | --any	LANGUAGE	LANGUAGE being one the SPRML 2013 shared task (ARABIC, FRENCH,...)
	-t | --test 			dev mode, test file, do not use 
	-d | --dev 	  			dev mode, test file, do not use 
	-k | --cut 	 			CUTOFF mode (tedeval is really slow for long sentences	, length cutoff hardcoded to 70 
	-s | --system FILE		test file to be evaluated 
	-g | --gold FILE		gold standard file ;;
	-L | --predlat FILE		predicted lattice files as provided by  SPMRL. If not given, spmrl one will be used
	-m | --predmap			mapping for predicted files (use it only when with non-spmrl predicted file	. Generated from predlat file otherwise
	-b | --begin 			line ID to start evaluate
	-e | --end 	 			line ID+1 to stop the evaluation
	--fivek					to evaluate only the first 5 thousands tokens (or so, wrt sentence boundaries) 
	--spmrldata_rootdir 	root directory to the SPMRL FINAL DATA SET (default \$FINAL/SPMRL_FINAL/READY_TO_SHIP_FINAL/
	--gold_ptb				for cross framework evaluation (if not given, taken from spmrldata_rootdir data)
	--test_ptb				test ptb data
	--help 	
	" ; 
	exit
fi

echo "###########################"
echo "Running\: tedeval.sh $@"
echo "###########################"
echo "\n"
TEMP=`/sw/bin/getopt -o DnlupcAHy:tdks:m:L:g:b:e: --long help,debug,new,labeled,unlabeled,ptb,conll,Arabic,Hebrew,any:,test,dev,cut,predfile:,system:,predmap:,predlat:,gold:,begin:,end:,spmrldata_rootdir:,fivek,test_ptb:,gold_ptb: -- "$@"`

if [ $? != 0 ] ; then echo "Terminating..." >&2 ; exit 1 ; fi

# Note the quotes around `$TEMP': they are essential!
eval set -- "$TEMP"


while true; do
  case "$1" in
    -D | --debug ) TEDEVALJAR=$PROGDIR/tedeval_debug.jar; shift ;;
    -n | --new )   TEDEVALJAR=$PROGDIR/tedeval-2.2.jar; shift ;;
	-u | --unlabeled ) LABEL="-unlabeled" ; shift ;;
	-l | --labeled ) LABEL="" ; shift ;;
	-p | --ptb ) TYPE=ptb ; TYPENAME=bracketed ; ARG="" ; shift   ;;
	-c | --conll ) TYPE=conll ; TYPENAME=conll ; ARG="-c" ; shift ;;
	-A | --arabic ) LANGUAGE=ARABIC ;SUF=""; SKIPFILE="" ; exit ; shift ;; 
	-H | --hebrew ) LANGUAGE=HEBREW  ; shift ;;
	-y | --any	) LANGUAGE="$2" ; shift 2 ;;
	-t | --test ) PREF="test" ; shift ;;
	-d | --dev )  PREF="dev"  ; shift ;;
	-k | --cut ) DOCUT=1 ; shift ;;
	-s | --system ) PREDFILE="$2" ; shift 2;;
	-g | --gold ) GOLDFILE="$2" ; shift 2;;
	-m | --predmap ) PREDMAP="$2" ; shift 2;;
	-L | --predlat ) PREDLAT="$2" ; shift 2;;
	-b | --begin )	START="$2"  ; shift 2;;
	-e | --end ) END="$2"  ; shift 2;;
	-R | --spmrldata_rootdir ) SPMRLDATA_ROOTDIR="$2" ; shift 2;;
	--fivek ) FIVEK="-5k" ; shift ;;
	--gold_ptb ) GOLDPTB="$2" ; shift 2 ;;
	--test_ptb ) TESTPTB="$2" ; shift 2 ;;
	--help )
	echo "tedeval_simple.sh OPTIONS -g GOLDFILE -s SYSTEMFILE 
    -D | --debug 	 	use tedeval + debug outputs
    -n | --new 	   		use latest tedeval-2.2.jar (default is 2.1	
	-u | --unlabeled 	unlabeled evaluation  (default)	 
	-l | --labeled 	 	labeled evaluation
	-p | --ptb 	 		evaluate const. files
	-c | --conll 	 	evaluate conll files (default)
	-a | --arabic 	 	dev mode for Arabic, do not use 
	-h | --hebrew 	 	dev mode for Hebrew, do not use 
	-y | --any	LANGUAGE	LANGUAGE being one the SPRML 2013 shared task (ARABIC, FRENCH,...)
	-t | --test 			dev mode, test file, do not use 
	-d | --dev 	  			dev mode, test file, do not use 
	-k | --cut 	 			CUTOFF mode (tedeval is really slow for long sentences	, length cutoff hardcoded to 70 
	-s | --system FILE		test file to be evaluated 
	-g | --gold FILE		gold standard file ;;
	-b | --begin 			line ID to start evaluate
	-e | --end 	 			line ID+1 to stop the evaluation 
	--spmrldata_rootdir 	root directory to the SPMRL FINAL DATA SET (default \$FINAL/SPMRL_FINAL/READY_TO_SHIP_FINAL/	
	--help 	
	" ; 
	shift ; exit 1;;
    -- ) shift; break ;;
    * ) break ;;
  esac
done


# *** INIT
# arabic lines bug
#START=699 # should be 799 but 100 sentences > 70 were removed
#START=167 # same bug as 699 but in labeled 

#END=700
#END=168
LDIR=`echo $LANGUAGE | perl -ne 'print uc $_'`_SPMRL
LANGUPPED=`echo $LANGUAGE | perl -ne 'print uc $_'`
LANGUAGE=`echo $LANGUAGE|perl -p -ne '$_ = ucfirst lc $_ ;'`
if [ ! -z "$FIVEK" ] ; then
	FIVEK="-5k $LANGUPPED"
fi


if [ -z "$PREDFILE" ]; then
		PREDFILE=${PREF}.${LANGUAGE}.pred.$TYPE.tobeparsed.pred_tagged+pred_token.disamb.lattices.parsed$SUF
fi

if [ ! -f "$PREDFILE" ] ; then
	echo "PREDFILE: $PREDFILE not found"
	exit 0
fi

echo "LDIR = $LDIR   LANGUAGE $LANGUAGE  PREDLAT $PREDLAT"
#exit




# we're always picking the 
if [ -z "$GOLDFILE" ] ; then
	GOLDFILE=${SPMRLDATA_ROOTDIR}/${LDIR}/gold/$TYPE/${PREF}/${PREF}.${LANGUAGE}.gold.$TYPE
fi

if [ -z "$GOLDPTB" ] ; then
	GOLDPTB=${SPMRLDATA_ROOTDIR}/${LDIR}/gold/ptb/${PREF}/${PREF}.${LANGUAGE}.gold.ptb
fi


	

# checking all files


if [ -f "$GOLDFILE" ] ; then
	echo "gold: $GOLDFILE found"
else
	wc -L $GOLDFILE
	echo "gold: $GOLDFILE not found"
	exit
fi



if [ -f "$PREDFILE" ] ; then
	echo "pred file: $PREDFILE found"
else
	echo "pred file: $PREDFILE not found"
	exit
fi

if [ -f "$TESTPTB" ] ; then
	echo "pred file: $TESTPTB found"
else
	echo "pred file: $TESTPTB not found"
	exit
fi



#exit

	
#PREDFILE=${PREF}.${LANGUAGE}.pred.$TYPE.tobeparsed.pred_tagged+pred_token.disamb.lattices

#  dev.Hebrew.pred.$TYPE.tobeparsed.pred_tagged+pred_token.disamb.parsed.lattices.5k
# test.Arabic.pred.$TYPE.tobeparsed.pred_tagged+pred_token.disamb.lattices.parsed.full


# fixing the blank line bug
#perl -i.bak -pe 's/^\s*$/\n/g' $GOLFILE $PREDFILE $PREDLAT $GOLDLAT

####################################################
##  real workd done here
######################################################


## calc
if test $DOCUT = 1 ; then
	echo "generating lines to be skipped"
	if test $TYPE = "-conll" ; then
		cat $GOLDFILE | perl -pe 's/^\s*$/\n/' |get_cutoffed_sent.pl -c -K 70 /dev/null > $GOLDFILE.tobeskipped
	else
		cat $GOLDFILE.tobeparsed.tagged.lattices|perl -pe 's/^\s*$/\n/' |get_cutoffed_sent.pl -c -K 70 /dev/null > $GOLDFILE.tobeskipped	
		# bug fixed tedeval_simple couldn't work on ptb file
		# because of get_cutoffed_sent.pl 
		# raaaaaaaaah
	fi
	
	# obsolete part was in for pre tedeval-2.2.jar  (no one would believe how much I sweat blood on those sentneces)
	if test "$PREF.$LANGUAGE" = "test.Arabic" ; then
		#echo -e "\n799" >> $GOLDFILELAT.tobeskipped # buggy sentence for unlabeld evaluation on arabic test
		echo "\n" >>  $GOLDFILE.tobeskipped
	elif test "$PREF.$LANGUAGE" = "dev.Arabic" ; then
		#echo -e "\n904\n1889" >>  $GOLDFILELAT.tobeskipped #buggy sentence for unlabeld evaluation on arabic dev
		echo "\n" >>  $GOLDFILE.tobeskipped

	fi
	SKIPFILE=$GOLDFILE.tobeskipped
else
	SKIPFILE=/dev/null
fi


#exit
 
echo "generating normalized files"
echo -e "\t==> conll gold and pred"

SUF4TED=4tedeval.$$
for FILE in $GOLDFILE $PREDFILE ; do 
echo "cleanup and normalizing $FILE"
	wc $ARG  $FILE
	cat $FILE|perl -pe 's/^\s*$/\n/'| skip_lines.pl $ARG $FIVEK  $SKIPFILE  | lines $ARG $START $END | reprojectivize.sh -$TYPE|   clean$TYPE.pl| perl -pe 's/^\s*$/\n/' |uniq > $FILE.$SUF4TED
	wc  $FILE.$SUF4TED
done 
echo -e "\t==> conll gold and pred"

for FILE in $GOLDPTB $TESTPTB ; do 
echo "cleanup and normalizing $FILE"

	wc  $FILE
	cat $FILE|perl -pe 's/^\s*$/\n/'| skip_lines.pl  $FIVEK  $SKIPFILE  | lines  $START $END | reprojectivize.sh -ptb |cleanptb.pl| perl -pe 's/^\s*$/\n/' |uniq > $FILE.$SUF4TED
	wc  $FILE.$SUF4TED
done 

if test $LABEL = "-unlabeled" ; then
	SUF="-unlabeled"
else
	SUF="-labeled"
fi

GOLDCONLL=$GOLDFILE.$SUF4TED
TESTCONLL=$PREDFILE.$SUF4TED
GOLDPTB=$GOLDPTB.$SUF4TED
TESTPTB=$TESTPTB.$SUF4TED









#echo "java  -Xmx768m -jar $TEDEVALJAR $LABEL -g  $GOLDFILE.$SUF4TED  -p $PREDFILE.$SUF4TED -format $TYPENAME -o $PREDFILE.4tedeval.simple_tedeval.res$SUF" > /dev/stderr

echo " "

java -Xmx768m   -cp $TEDEVALJARAPP:$TEDEVALJAR:. applications.Dtreebank2Ftreebank $GOLDCONLL $GOLDCONLL.ftrees
#echo "java -Xmx768m   -cp $TEDEVALJARAPP:$TEDEVALJAR:. applications.Dtreebank2Ftreebank $GOLDCONLL $GOLDCONLL.ftrees"

#exit
java -Xmx768m   -cp $TEDEVALJARAPP:$TEDEVALJAR:. applications.Dtreebank2Ftreebank $TESTCONLL $TESTCONLL.ftrees
java -Xmx768m   -jar $TEDEVALJAR -unlabeled -p1 $TESTCONLL.ftrees -g1 $GOLDCONLL.ftrees  -p2 $TESTPTB -g2 $GOLDPTB -o1 $TESTCONLL.cross_tedeval.res -o2 $TESTPTB.cross_tedeval.res

file1="$TESTCONLL.cross_tedeval.res.ted"
file2="$TESTPTB.cross_tedeval.res.ted"

cat $file1 | grep "AVG:"|  perl -p -s -e 'chomp ; s/(.)$/\1\t$file \n/' -- -file=$file1 | get_ted_res.pl | tee $LANGUAGE.conll.cross_tedeval.res
cat $file2 | grep "AVG:"|  perl -p -s -e 'chomp ; s/(.)$/\1\t$file \n/' -- -file=$file2 | get_ted_res.pl | tee  $LANGUAGE.ptb.cross_tedeval.res
cat $LANGUAGE.conll.cross_tedeval.res $LANGUAGE.ptb.cross_tedeval.res > $LANGUAGE.cross_tedeval.res



echo -e "\n\n"

