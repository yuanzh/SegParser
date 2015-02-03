#!/usr/bin/python
# Author : Reut Tsarfaty, July 2013
# ligth modifs: Djame Seddah
# +modif to support ptb's lattice files 
import sys

if sys.argv[1] == "-ptb":
	ptb=1
else:
	ptb=0




prev_tok = ""
out_line = ""
first=1
for line in sys.stdin:
   line = line.strip().split()
   if not line:
      #out_line +=  "\t".join([token,form])
      if out_line:
        print out_line
      else:
        print "\n"
      prev_tok = ""
      out_line = ""
      #print "\n"
      continue
      
   if ptb == -1:  #this code is bogus, the ptb hebrew files lacks the lemma field
   		start, end, form, lemma, cpos, fpos, feats, token = line
   else:
   		start, end, form = line[0:3]
   		token = line[-1]
   	
   if prev_tok == token:
        out_line += "".join([":",form])
        prev_tok = token
   else:
        if first==1: #lame modif to avoid first line void
           first=0
        else:
           print out_line
        out_line = ""
        out_line +=  "\t".join([token,form])
        prev_tok = token
print "\n"
          
       



