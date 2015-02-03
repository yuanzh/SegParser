#!/usr/bin/perl

# script that clean treebank and treebank output
# for use with tedeval
# Djame Seddah

use strict;


use constant {
		# for conll data
        ID      => 0,
        FORM => 1,   
        LEMMA => 2,  
        CPOS => 3,   
        FPOS => 4,   
        FEAT => 5,   
        HEAD => 6,   
        DEPREL => 7, 
        PHEAD => 8,  
        PDEPREL => 9,
        SOURCETOKEN => 10,
        # for morfette data
        FORMM =>0,   
        LEMMAM=> 1,  
        FEATM=> 2    
};

my $kk=0;
if ($ARGV[0] eq "-pass"){
        $kk=1;
}


while(<>){
		chomp;
		my $line=$_;
		if ($line=~/^\s*$/){ print "\n"; next;}
		if ($kk ==1){ print "$line\n"; next;} # just for debogging' sake (like do nothing)
		my @FC=split(/\t/,$line);
		foreach my $field (LEMMA,CPOS,FPOS,FEAT,DEPREL){
			$FC[DEPREL]=~s/^(.+)\|.+/$1/; # beware, destructive operation. Tree won't be able to deprojectivize (we strip some information)
			$FC[$field]=~s/^[_|-]+$/dummy/;
			$FC[$field]=~s/\:/<column>/g; # that one I like...
			$FC[$field]=~s/-(..)B-/$1B/;
		}
		$FC[FEAT]="_";
		$FC[FORM]=~s/\:/<column>/g; # for fuck's sake putain..
		$FC[FORM]=~s/-(..)B-/$1B/;
		# we print the first 8 
		print join("\t",@FC[0..7]),"\t";
		print join("\t",@FC[6..7]);
		if (defined $FC[SOURCETOKEN]){
                 print "\t",$FC[SOURCETOKEN];
		}
		print "\n"; 
}


