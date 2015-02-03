#!/usr/bin/perl
# Copyright (c) 2001 by David Chiang. All rights reserved.
# modif to cope with conll files by DjamSeddah (2013)
# usage: lines 2 3 < test.mrg  or lines -c 2 3 < test.conll
 
use strict;
my $KK="\n";
if ($ARGV[0] eq "-c"){
	$KK="\n\n";
	shift @ARGV;
}elsif($ARGV[0] eq '-mada'){
       $KK="--------------\nSENTENCE BREAK\n--------------\n";
       shift @ARGV;
}

my $CUTOFF;
if ($ARGV[0] eq "-K"){
	$CUTOFF=$ARGV[1] or die "cut-off lenght not given.\n";
	shift @ARGV; #lame I know, but that case was inserted way after the rest..
	shift @ARGV;  
}


open FICIN,"<$ARGV[0]" or die "[get_cutoffed_sent.pl] problem with $ARGV[0] or no file given\n";
my @lines2skip=<FICIN>;
chomp @lines2skip;
my %H= map { $_=~s/^\s*([0-9]+)\s*$/$1/; $_ => 1 } @lines2skip;

#print join("__", keys %H),"__ICI\n";

#die;		
$/=$KK;


my $i = 1;
my $skipped=0;
my $total=0;
while (<STDIN>) {
#       if ($_=~/^#/){print "$_";} # print comment
		my $len=&get_lenght($_);
        if ($len>$CUTOFF) {
                 
				 #print  &get_lenght($_),"\n";
				 print "$i\n";
				 $skipped++;
        }else{
        		  $total=$total+$len;
#        		  print STDERR "$i\n";	
                  
         }	
	$i++;
}

print STDERR "$skipped sentences skipped\n";
my $perc=($skipped/$i)*100;
print STDERR "$perc \% of sentences removed ($skipped / $i)\n";
my $avg_lenght=$total/$i;
print STDERR "Avg = ".$avg_lenght."\n";

sub get_lenght{
	my $sent=shift;
	my $count= () = $sent =~ /\n/g;
	return $count;
}             
