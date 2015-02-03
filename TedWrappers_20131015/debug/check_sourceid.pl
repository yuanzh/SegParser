#!/usr/bin/perl



use strict;

my $sent=0;
my $i++;
my $j=0;
while(<>){
    chomp;
    my $line=$_;
    $j++;
    if ($line=~/^\s*$/){$sent++; $i=0; next;}
    my @FC=split(/\t/,$line);
    $i++;
    if ($FC[$#FC] !~m/^[0-9]+$/){ print "sentence $sent, token $i (line $j)\n"; exit;};

}