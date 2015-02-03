#!/usr/bin/python

# (C)Copyright 2006, TOSHIBA Corporation, all rights reserved.

'''
Script for checking whether a file is in CoNLL-X shared task format
'''

# still to do:
# check that either all or none of PHEAD and PDEPREL are dummy value
### would not work for Spanish data...
# check that PHEAD is really projective


__author__ = 'Sabine Buchholz <sabine dot buchholz at crl dot toshiba dot co dot uk>'
__version__ = '$Id: validateFormat.py,v 1.4 2006/03/08 17:30:50 sabine Exp $'

import sys
import string
import os.path
import codecs, re

try:
    import optparse
except ImportError:
    print >>sys.stderr, \
          "You need Python version 2.3 or later; your version is %s" % \
          sys.version
    raise

from SharedTaskCommon import \
     emptyProjColumnString, handleProblem, \
     checkCycles_tmp2, checkCyclesPhead, Terminal

def validate(infile,instream,outstream, options):
    global exit_status

    # initialize
    line_number = 0
    (idmax, headmax, pheadmax) = (0, 0, 0)
    token_list = ['dummy'] # the 0'th element
    rootLines = []
    error_flag = 0
    sent_start = 1

    for line in instream:
        line_number += 1
        # empty line ends sentence
        if re.compile(u'^\s*$').search(line):
            check_sentence(infile, options,
                           sent_start,
                           rootLines, token_list,
                           error_flag,
                           idmax, headmax, pheadmax)
            # re-initialize
            (idmax, headmax, pheadmax) = (0, 0, 0)
            token_list = ['dummy'] # the 0'th element
            rootLines = []
            error_flag = 0
            sent_start = line_number+1 # line where next sentence starts

        # non-empty line, i.e. token
        else:
            if options.input_sep == ' +': # if separator is spaces
                line = line.strip() # remove leading and trailing whitespace
            else:
                line = line.rstrip() # remove trailing whitespace (e.g. \r, \n)
            # split using input_sep regular expression
            fields = re.compile(options.input_sep).split(line)

            if len(fields) < minNumCols:
                msg = "%s: Error: line %d: Too few columns (%d<%d):\n\t%s" % \
                      (infile,line_number,len(fields),minNumCols,line)
                print >>sys.stderr, msg.encode(options.encoding)
                error_flag = 1
                exit_status = 1
                terminal = 'dummy'
            elif len(fields) > maxNumCols:
                msg = "%s: Error: line %d: Too many columns (%d>%d):\n\t%s" % \
                      (infile,line_number,len(fields),maxNumCols,line)
                print >>sys.stderr, msg.encode(options.encoding)
                error_flag = 1
                exit_status = 1
                terminal = 'dummy'
            else:
                (terminal, error_flag, idmax,
                 headmax, pheadmax) = check_line(infile,line_number,line,
                                                 options, rootLines,
                                                 fields,token_list,
                                                 error_flag,
                                                 idmax,headmax,pheadmax)

            token_list.append(terminal)

    if len(token_list) > 1: # more than just dummy
        # i.e. some unprocessed sentence is left
        msg = "line %d: No empty line after last sentence" % \
              (line_number)
        handleProblem(infile, 'whitespace', msg, options)

        check_sentence(infile, options,
                       sent_start,
                       rootLines, token_list,
                       error_flag,
                       idmax, headmax, pheadmax)


def check_sentence(infile, options,
                   sent_start,
                   rootLines, token_list,
                   error_flag,
                   idmax, headmax, pheadmax):
    global exit_status

    # check that there are tokens, i.e. not two
    # empty lines following each other
    if len(token_list) == 1: # just dummy
        msg = "line %d: More than one empty line separating sentences" % \
              (sent_start)
        handleProblem(infile, 'whitespace', msg, options)

    else:
        if options.datatype == 'train' or options.datatype == 'system':
            if error_flag == 0: # only check if no error occurred so far
                # check that there is at least one root
                if len(rootLines) == 0:
                    msg = "%s: Error: line %dff: no token has HEAD=0" % \
                           (infile, sent_start)
                    print >>sys.stderr, msg.encode(options.encoding)
                    error_flag = 1
                    if options.datatype == 'train':
                        exit_status = 1
                    # else: system submissions: print but accept

    ##         # check that there is exactly one root (option???)
    ##         if len(rootLines) > 1:
    ##             msg = "%s line %dff: Warning: several tokens have HEAD=0:\n\t%s" % \
    ##                   (infile, sent_start,"\n\t".join(rootLines))
    ##             print >>sys.stderr, msg.encode(options.encoding)

                # check that HEAD and PHEAD are not higher than highest ID
                if headmax > idmax:
                    msg = "%s: Error: line %dff: too big HEAD value (%d>%d)" % \
                          (infile, sent_start, headmax, idmax)
                    print >>sys.stderr, msg.encode(options.encoding)
                    error_flag = 1
                    if options.datatype == 'train':
                        exit_status = 1
                    # else: system submissions: print but accept
                if pheadmax > idmax:
                    msg = "%s: Error: line %dff: too big PHEAD value (%d>%d)" % \
                          (infile, sent_start, pheadmax, idmax)
                    print >>sys.stderr, msg.encode(options.encoding)
                    error_flag = 1
                    if options.datatype == 'train':
                        exit_status = 1
                    # else: system submissions: print but accept

        # if necessary, do punctuation checks
        if error_flag == 0: # only check if no error occurred so far
            if options.punctPostag != '':
                # a value is given, so punctuation must be checked
                punctRe = re.compile('^'+options.punctPostag+'$')
                non_punct_count = 0 # how many tokens are not punctuation
                for i in range(1,len(token_list)):
                    if not punctRe.search(token_list[i].cpostag):
                        # is not punctuation
                        non_punct_count += 1
                        headID = token_list[i].head
                        if headID != 0: # not root (cannot link to punctuation anyway)
                            if punctRe.search(token_list[headID].cpostag):
                                # links to punctuation
                                msg = "line %dff: token %d (%s) links to punctuation" % \
                                      (sent_start, i, token_list[i].form)
                                handleProblem(infile, 'punct', msg, options)
                                # this assumes that punctuation linking
                                # to punctuation is fine
                if non_punct_count == 0:
                    msg = "line %dff: only punctuation tokens in sentence" % \
                          (sent_start)
                    handleProblem(infile, 'punct', msg, options)


        # check for dependency cycles
        if options.datatype == 'train' or options.datatype == 'system':
            if error_flag == 0: # only check if no error occurred so far
                checkCycles_tmp2("%s line %dff" % (infile, sent_start),
                                 options, token_list, options.rootDeprel)
                checkCyclesPhead("%s line %dff" % (infile, sent_start),
                                 options, token_list, options.rootDeprel)


def check_line(infile,line_number,line,
               options, rootLines,
               fields,token_list,
               error_flag,
               idmax,headmax,pheadmax):
    global exit_status

    if options.datatype == 'train':
        (id, form, lemma, cpostag, postag,
         feats, head, deprel, phead, pdeprel) = fields
    elif options.datatype == 'test_blind':
        (id, form, lemma, cpostag, postag, feats) = fields
        (head, deprel, phead, pdeprel) = (u'0',emptyProjColumnString,
                                          emptyProjColumnString,emptyProjColumnString)
    elif options.datatype == 'system':
        (id, form, lemma, cpostag, postag,
         feats, head, deprel) = fields[0:8]
        if len(fields) == 8:
            (phead, pdeprel) = (emptyProjColumnString,emptyProjColumnString)
        elif len(fields) == 9:
            phead = fields[8]
        elif len(fields) == 10:
            (phead, pdeprel) = fields[8:10]

    # check that ID is integer > 0
    if id != u'0' and re.compile(u'^[0-9]+$').search(id):
        id = int(id)
        if id > idmax:
            idmax = id
        # check that ID is consecutive
        if id != len(token_list):
            msg = "%s: Error: line %d: Non-consecutive value for ID column (%d!=%d):\n\t%s" % \
                  (infile,line_number,id,len(token_list),line)
            print >>sys.stderr, msg.encode(options.encoding)
            error_flag = 1
            exit_status = 1
    else:
        msg = "%s: Error: line %d: Illegal value for ID column:\n\t%s" % \
              (infile,line_number,line)
        print >>sys.stderr, msg.encode(options.encoding)
        error_flag = 1
        exit_status = 1

    if options.datatype == 'train'  or options.datatype == 'system':
        # check that PHEAD is emptyProjColumnString or integer >= 0
        if phead != emptyProjColumnString:
            if re.compile(u'^[0-9]+$').search(phead):
                phead = int(phead)
                if phead > pheadmax:
                    pheadmax = phead
            else:
                msg = "%s: Error: line %d: Illegal value for PHEAD column:\n\t%s" % \
                      (infile,line_number,line)
                print >>sys.stderr, msg.encode(options.encoding)
                error_flag = 1
                if options.datatype == 'train':
                    exit_status = 1
                # else: system submissions: print but accept

        # check that HEAD is integer >= 0
        if re.compile(u'^[0-9]+$').search(head):
            head = int(head)
            if head > headmax:
                headmax = head
            if options.rootDeprel != '' and deprel == options.rootDeprel and head != 0:
                # check that HEAD is 0 if DEPREL is options.rootDeprel
                msg = ("line %d: HEAD is not 0:\n\t%s" % \
                       (line_number,line))
                handleProblem(infile, 'root', msg, options)
            if head == 0: # root
                # check that DEPREL is options.rootDeprel if HEAD is 0
                if options.rootDeprel != '' and deprel != options.rootDeprel:
                    msg = "line %d: DEPREL is not %s:\n\t%s" % \
                          (line_number,options.rootDeprel,line)
                    handleProblem(infile, 'root', msg, options)
                rootLines.append(line)

        else:
            msg = "%s: Error: line %d: Illegal value for HEAD column:\n\t%s" % \
                  (infile,line_number,line)
            print >>sys.stderr, msg.encode(options.encoding)
            error_flag = 1
            if options.datatype == 'train':
                exit_status = 1
            # else: system submissions: print but accept

    # check that other fields are not empty
    # (can occur with tab but not with spaces as separator)
    if (len(form)    == 0 or len(lemma)  == 0 or
        len(cpostag) == 0 or len(postag) == 0 or
        len(feats)   == 0 or len(deprel) == 0 or len(pdeprel) == 0):
        msg = "line %d: At least one column value is the empty string:\n\t%s" % \
              (line_number,line)
        handleProblem(infile, 'other', msg, options)

    # check that other fields do not contain whitespace
    ws = re.compile('\s')
    if (ws.search(form) or ws.search(lemma) or
        ws.search(cpostag) or ws.search(postag) or
        ws.search(feats) or ws.search(deprel) or ws.search(pdeprel)):
        msg = "line %d: At least one column value contains whitespace:\n\t%s" % \
              (line_number,line)
        handleProblem(infile, 'whitespace', msg, options)


    terminal = Terminal(id,form,lemma,cpostag,postag,feats,deprel, phead,pdeprel)
    terminal.head = head # change class???
    return (terminal, error_flag, idmax, headmax, pheadmax)



usage = \
"""
	%prog [options] FILES

purpose:
	checks whether files are in CoNLL-X shared task format

args:
	FILES              input files
"""

parser = optparse.OptionParser(usage, version=__version__)

problems_types = [ 'cycle',      # cycles in dependency structure
                   #'index',      # index of head points to non-existent token
                   'punct',      # problem with punctuation
                   'whitespace', # missing or superfluous whitespace
                   'root',       # problem with token linking to root
                   'other'       # anything else
                   ]

parser.add_option('-d', '--discard_problems',
                  dest='discard_problems',
                  metavar='STRING',
                  choices=problems_types,
                  action='append',
                  default = []
                  ) # needed only for checkCycles functions

parser.add_option('-e', '--encoding',
                  dest='encoding',
                  metavar='STRING',
                  action='store',
                  default='utf-8',
                  help="output character encoding (default is utf-8)")

parser.add_option('-i', '--input_separator',
                  dest='input_sep',
                  metavar='STRING',
                  action='store',
                  default='\t', # tab   ### default=' +' # spaces
                  help="""regular expression for column separator in
                  input (default is one tab, i.e. '\\t')""")

parser.add_option('-p', '--punctuation',
                  dest='punctPostag',
                  metavar='STRING',
                  action='store',
                  default='',
                  help="""use given regular expression to identify
                  punctuation (by matching with the CPOSTAG column)
                  and check that nothing links to and that a sentence
                  contains more than just punctuation (default: turned off)""")

parser.add_option('-r', '--root_deprel',
                  dest='rootDeprel',
                  metavar='STRING',
                  action='store',
                  default='', # no root specified
                  help="""designated root label: check that there is exactly
                  one token with that label and that it's HEAD is 0
                  (default: not specified)""")

parser.add_option('-s', '--silence_warnings',
                  dest='silence_warnings',
                  metavar='STRING',
                  choices=problems_types,
                  action='append',
                  default = [],
                  help="""don't warn about certain types of
                  problems (default is to warn about every problem);
                  possible choices:"""+' '.join(problems_types)
                  )

parser.add_option('-t', '--type',
                  dest='datatype',
                  metavar='STRING',
                  action='store',
                  default='train', # training data
                  help="""type of the data to be tested: train,
                  test_blind, system (default: train)""")

(options, args) = parser.parse_args()

# to enable random access to values
options.discard_problems = dict.fromkeys(options.discard_problems)
options.silence_warnings = dict.fromkeys(options.silence_warnings)

# how many columns there should be
minNumCols = 10 # default # for 'train'
maxNumCols = 10 # default # for 'train'
if options.datatype == 'test_blind':
    minNumCols = 6
    maxNumCols = 6
elif options.datatype == 'system':
    minNumCols = 8
    maxNumCols = 10
elif options.datatype != 'train':
    print >>sys.stderr, 'Incorrect value for option -t: must be train, test_blind or system'
    sys.exit(1)

exit_status = 0

if not args:
    print >>sys.stderr, 'Incorrect number of arguments'
    sys.exit(1)
else:
    for infile in args:
        print >>sys.stderr,"File: ",infile
        validate(infile,
                 codecs.open(infile, 'r', 'utf-8'), # encoding option
                 sys.stdout,
                 options)

print >>sys.stderr, 'Exit status = ',exit_status
sys.exit(exit_status)
