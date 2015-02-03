#!/usr/bin/perl

# script that clean treebank and treebank output
# for use with tedeval
# Djame Seddah

use strict;


my $kk=0;
if ($ARGV[0] eq "-pass"){
        $kk=1;
}


while(<>){
		chomp;
		my $line=$_;
		$line=~s/^\( /(TOP /;
		#if ($line=~/^\s*$/){ print "\n"; next;}
		if ($kk ==1){ print "$line\n"; next;} # just for debogging' sake (like do nothing)
		$line=~s/##[^#]+##//g; # removing all features
		# magical regexp from releaf.pl
		my $preterm='\(([^() \t]+)[ \t]+([^() \t]+)\)';# match (DT The) or (NC samere_en_short)
		$line=~s/$preterm/"(".&clean_all($1)." ".&clean_all($2).")"/ge;
		print "$line\n";
}


sub clean_all{
        my $string= shift;
        #$string=~s/^[_|-]+$/dummy/;
		$string=~s/\:/<column>/g; # that one I like...
		$string=~s/-(..)B-/$1B/; # probably not necessary
		return $string;	
}
