#!/usr/bin/perl


#use strict;


#------------------------------------------------------------
# Sentence        TED       Exact              #Spans                              TED
# ID   Length    Accuracy    match       test    gold    gen           Distance           Normalization
#                         gold   gen                              L1      L2   L1 - L2
#_____________________________________________________________________________________________________
my @td=qw(LEN ACC EX_gold Ex_gen Spans_test Spans_gold Spans_gen Dist_L1 Dist_L2 Dist_L1-L2 Norm file);        

while(<>){
      chomp;
      my $line=$_;
      $line=~s/AVG:\s+//g;
      $line=~s/,/./g;
      my @res=split(/\s+/,$line);
      my %Hres=();
      my $i=0;
      foreach my $el (@res){
        my $key=$td[$i++];
           $Hres{$key}=($el);     
           #print "$key\t$el\n";
      }             
	#print qw(ACC EX_gold Ex_gen Norm Spans_test Spans_gold Spans_gen Dist_L1 Dist_L2 Dist_L1-L2);
      foreach my $key (qw(ACC EX_gold Ex_gen Norm Spans_test Spans_gold Spans_gen Dist_L1 Dist_L2 Dist_L1-L2)){
          print "$key: $Hres{$key}\t";
      }
      print "file: $Hres{file}\n";
}
