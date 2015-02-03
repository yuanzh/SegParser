#!/usr/bin/perl


use strict;
my $LANG;
my $GOLD;
my $TYPE;
while(<>){
	chomp;
	my $line=$_;
	if($line=~/(HEBREW)/i) {
		$LANG="HEBREW";
	}elsif($line=~/(ARABIC)/i) {
		$LANG="ARABIC";
	}
	
	if ($line=~/ptb/){
		$TYPE="ptb";
	}elsif ($line=~/conll/){
		$TYPE="conll";
	}
	my $Lang=ucfirst lc $LANG;
	#print "$Lang\n"; next;
	 my $file=$line;
	 $GOLD="../READY_TO_SHIP_FINAL/${LANG}_SPMRL/gold/$TYPE/test/test.$Lang.gold.$TYPE";	
	 my $TFM="tedeval.sh  --unlabeled --$TYPE --any $LANG -k  -g $GOLD -s $file | tee $file.djam_log ;  cat $file.4tedeval.evalted.res.ted-unlabeled |grep \"AVG:\"| perl -ne 'chomp ; print \"\$_\\t$file\\n\"'" ;
	 #print STDERR `eval $CMD | tee $file.log`;
	 print $TFM."\n";
}
