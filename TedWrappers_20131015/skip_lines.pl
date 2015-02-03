#!/usr/bin/perl
# Copyright (c) 2001 by David Chiang. All rights reserved.
# modif to cope with conll files by DjamSeddah (2013)
# usage: lines 2 3 < test.mrg  or lines -c 2 3 < test.conll
 
use strict;
use Data::Dumper;
use Getopt::Long;

# to generate the small test sets version
# will add the option later
my @test5knoskip=qw/145 436 149 285 223 291 409 486 319/;
my @dev5knoskip=qw/126 337 166 338 203 157 388 493 238/;

# now those are the right ones (la putain de sa mère !!)
my @test5k=qw/153 436 152 285 223 291 409 319 /;
my @dev5k=qw/130 337 169 338 209 161 388 493 256 /;


# ARABIC= 145  HEBREW 223
my @lang=qw/ARABIC BASQUE FRENCH GERMAN HEBREW HUNGARIAN KOREAN POLISH SWEDISH/;
my %data=();
my $i=0;
foreach my $l (@lang){
	$data{$l}{dev}=$dev5k[$i];
	$data{$l}{test}=$test5k[$i];
	$i++;
}
#print Dumper(\%data);



my $KK="\n";
if ($ARGV[0] eq "-c"){
	$KK="\n\n";
	shift @ARGV;
}elsif($ARGV[0] eq '-mada'){
       $KK="--------------\nSENTENCE BREAK\n--------------\n";
       shift @ARGV;
}
my $fiveK=0;
my $pref="test";
my $lang="";
if ($ARGV[0] eq "-5k"){
	if(defined $ARGV[1]){
		$lang=uc $ARGV[1];
		shift @ARGV;
		}else{
			die "-5k must be followed by a language (Arabic,French..)\n";
		}
	$fiveK=1;
	shift @ARGV;
}



open FICIN,"<$ARGV[0]" or die "[skip_lines.pl] problem with $ARGV[0] or no file given\n";
my @lines2skip=<FICIN>;
chomp @lines2skip;
my %H= map { $_=~s/^\s*([0-9]+)\s*$/$1/; $_ => 1 } @lines2skip;

#print join("__", keys %H),"__ICI\n";

#die;		
$/=$KK;


my $i = 1; #line read, even if skipped
my $j=1;  #line effectively output 
my $skipped=0;
LOOP: while (<STDIN>) {
#       if ($_=~/^#/){print "$_";} # print comment
        if (!exists $H{$i}){
			if ( ($fiveK == 1) && ($j >$data{$lang}{$pref}) ){
					# we simply exit
					last LOOP; # lame and all but is there a simplest way to exit ?
			}
			print "$_";
			$j++;     
        }else{
                  $skipped++;
         }	
	$i++;
}
OUT:
print STDERR "$skipped sentences skipped\n";
print STDERR "$lang = $data{$lang}{$pref} sentences\n" if ($fiveK == 1);
             
__END__
