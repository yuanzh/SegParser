#!/usr/local/bin/python

import sys
import string

rootDeprel = u'ROOT' # the dependency relation for the root
emptyFeatsString = u'_' # if no morphological features exist (only PoS)
featsJoiner      = u'|' # to join morphological features into one string
emptyProjColumnString = u'_' # if no PHEAD or PDEPREL available

class NonTerminal:
    def __init__(self,constLabel,features,deprel):
        self.constLabel = constLabel
        self.features   = features
        self.head       = {} # a dictionary of references to the lexical heads
        self.deprel     = deprel
        self.children   = []

    def getLexHead(self,head_type):
        if not self.head.has_key(head_type): # does not have this head type
            # this can happen if a proper head child could not be found
            # according to the normal head rules and the default rules
            # have been applied, resulting e.g. in an NP being the
            # head of a finite clause
            head_type = 'head' # take default head type
        return self.head[head_type]


class Terminal:
    def __init__(self, id, form, lemma, cpostag, postag, feats, deprel,
                 phead = emptyProjColumnString, pdeprel = emptyProjColumnString):
        self.id       = id
        self.form     = form
        self.lemma    = lemma
        self.cpostag  = cpostag
        self.postag   = postag
        self.feats    = feats
        self.deprel   = deprel
        self.phead    = phead
        self.pdeprel  = pdeprel
        # initially, a terminal links to itself;
        # needed for recursive percolation of lexical heads
        self.head     = self

    def getLexHead(self,head_type):
        # the head_type is irrelevant:
        # terminals only have one head
        return self.head

class CorpusError(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return `self.value`


def processOptionsBlanks(options):
    """turn string of column widths (e.g. '3|10|10|5')
    into list (e.g. [3,10,10,5])"""

    if options.blanks:
        list = options.blanks.split('|')
        if len(list) != len(options.output):
            print >>sys.stderr, ("Value to blanks option does not \
have same number of elements as output columns chosen:\n\
%s != %s" % (list,options.output))
            sys.exit(-1)
        for i in range(len(list)):
            try:
                int = string.atoi(list[i])
            except ValueError:
                print >>sys.stderr, ("Non-integer value in blanks option: %s" %
                                     list[i])
                sys.exit(-1)
            else:
                list[i] = int
        options.blanks = list


# obsolete: just use '... = dict.fromkeys(list)' instead
# thanks to EM for ponting this out
#def turnListIntoHash(list):
#    hash = {}
#    for i in range(len(list)):
#       hash[list[i]] = 1
#    return hash


def handleProblem(infile, problem_type, msg, options):
    "depending on options: raise exception or just warn or stay silent"
    if options.discard_problems.has_key(problem_type):
        raise CorpusError, msg
    else:
        if not options.silence_warnings.has_key(problem_type):
            print >>sys.stderr, ("%s: Warning: %s" %
                                 (infile, msg)).encode(options.encoding)


def addOptions(parser):
    # what types of problems can occur during conversion;
    # list can be used to selectively silence warnings
    # or discard sentences (or files, with 'XML') that
    # have those problems
    problems_types = [ 'XML',        # error in XML parsing
                       'cycle',      # cycles in dependency structure
                       'label',      # wrong POS/constituent/function label
                       'index',      # index of head points to non-existent token
                       'punct',      # problem in reattaching children of punctuation
                       'whitespace', # missing or superfluous whitespace
                       'discontinuity', # problem relating to annotation of discontinuity
                       'ambiguity',  # ambiguous structure annotated in treebank
                       'tree',       # problem with structure of tree (not discontinuity)
                       'head_table', # cannot find head child
                       'other'       # anything else
                       ]

    parser.add_option('-b', '--blanks',
                      dest='blanks',
                      action='store',
                      metavar='FORMAT',
                      default='',
                      help="""
                      use variable number of blanks as
                      output column separator (default is tab);
                      expects argument FORMAT of form: i|j|k|...
                      where i,j,k etc. are integer>0, indicating the minimum
                      width of that column (there must be as many integers as
                      columns requested in the output)
                      """
                      )

    parser.add_option('-c', '--condition',
                      dest='condition',
                      action='store',
                      metavar='CONDITION',
                      default='',
                      help="""use only those files/extracts/sentences that
                      fulfill CONDITION (e.g. <743 or >=743); useful for
                      splitting into training and test set"""
                      )

    parser.add_option('-d', '--discard_problems',
                      dest='discard_problems',
                      choices=problems_types,
                      action='append',
                      default = [],
                      help="""discard sentence (or file, for XML problems) that
                      exhibits certain problems (default is fix, not discard);
                      possible choices:"""+' '.join(problems_types)
                      )

    parser.add_option('-e', '--encoding',
                      dest='encoding',
                      action='store',
                      default='utf-8',
                      help="output character encoding (default is utf-8)")

    parser.add_option('-f', '--file',
                      dest='file',
                      action='store_true',
                      default=False,
                      help="""write output to file, replacing original
                      suffix by .conll (default is to standard output)"""
                      )

    parser.add_option('-o', '--output',
                      dest='output',
                      choices=['id','form','lemma','cpostag','postag',
                               'feats','head','deprel','phead','pdeprel'],
                      action='append',
                      default = [],
                      help="""print named column in output, in order
                      specified on command line(default is none);
                      possible choices:
                      'id','form','lemma','cpostag','postag',
                      'feats','head','deprel','phead','pdeprel'"""
                      )

    parser.add_option('-s', '--silence_warnings',
                      dest='silence_warnings',
                      choices=problems_types,
                      action='append',
                      default = [],
                      help="""don't warn about certain types of conversion
                      problems (default is to warn about every problem);
                      possible choices:"""+' '.join(problems_types)
                      )

    parser.add_option('-p', '--punctuation',
                      dest='punctuation',
                      action='store_true',
                      default=False,
                      help='links words linking to punctuation to punctuation\'s head instead'
                      )



def checkCycles(infile, options, token_list, rootFunction):
    for i in range(1,len(token_list)):
        head_path = { i: 1 }
        j = i
        while j != 0:
            j = token_list[ j ]['head']
            if head_path.has_key(j): # cycle found!
                # raise exception or just warn or stay silent
                msg = (u"Cycle detected at token %d (%s)" %
                       (j, token_list[ j ]['form']))
                handleProblem(infile, 'cycle', msg, options)
                # break cycle by linking token to root
                token_list[ j ]['head']   = 0
                token_list[ j ]['deprel'] = rootFunction
                break
            else:
                head_path[j] = 1


def checkCycles_tmp2(infile, options, token_list, rootFunction):
    for i in range(1,len(token_list)):
        head_path = { i: 1 }
        j = i
        while j != 0:
            j = token_list[ j ].head
            if head_path.has_key(j): # cycle found!
                # raise exception or just warn or stay silent
                msg = (u"Cycle detected at token %d (%s)" %
                       (j, token_list[ j ].form))
                handleProblem(infile, 'cycle', msg, options)
                # break cycle by linking token to root
                token_list[ j ].head   = 0
                token_list[ j ].deprel = rootFunction
                break
            else:
                head_path[j] = 1

def checkCyclesPhead(infile, options, token_list, rootFunction):
    for i in range(1,len(token_list)):
        head_path = { i: 1 }
        j = i
        while j != 0 and token_list[ j ].phead != emptyProjColumnString:
            # if PHEAD column contains dummy value, just stop checking

            j = token_list[ j ].phead
            if head_path.has_key(j): # cycle found!
                # raise exception or just warn or stay silent
                msg = (u"PHEAD cycle detected at token %d (%s)" %
                       (j, token_list[ j ].form))
                handleProblem(infile, 'cycle', msg, options)
                # break cycle by linking token to root
                token_list[ j ].phead   = 0
                token_list[ j ].pdeprel = rootFunction
                break
            else:
                head_path[j] = 1


def attachPunctHigh(infile, options, token_list, punctuationPos,
                    punctuationFunction, rootFunction):
    """
    Reattach punctuation as high as possible,
    change deprel to value punctuationFunction.
    """

    for i in range(1,len(token_list)):
        token1 = token_list[ i ]
        if token1['postag'] == punctuationPos:
            punc  = token1

            # find highest attachment point
            highest = 0
            head_path = {}
            if i>1:
                j=i-1
                while token_list[ j ]['head'] != 0:
                    if head_path.has_key(j):
                        # raise exception or just warn or stay silent
                        msg = (u"Cycle detected at token %d (%s)" %
                               (j, token_list[ j ]['form']))
                        handleProblem(infile, 'cycle', msg, options)
                        # break cycle by linking token to root
                        token_list[ j ]['head']   = 0
                        token_list[ j ]['deprel'] = rootFunction
                        break
                    head_path[j] = 1
                    j = token_list[ j ]['head']
                highest = j
            if i<len(token_list)-1:
                j=i+1
                while token_list[ j ]['head'] != 0:
                    if head_path.has_key(j):
                        if head_path[j] == 2:
                            # raise exception or just warn or stay silent
                            msg = (u"Cycle detected at token %d (%s)" %
                                   (j, token_list[ j ]['form']))
                            handleProblem(infile, 'cycle', msg, options)
                            # break cycle by linking token to root
                            token_list[ j ]['head']   = 0
                            token_list[ j ]['deprel'] = rootFunction
                            break
                        elif head_path[j] == 1:
                            # was also on other path
                            break
                    head_path[j] = 2
                    j=token_list[ j ]['head']
                highest = j

            # make punctuation link to highest
            punc['head']   = highest
            if highest == 0:
                punc['deprel'] = rootFunction
            else:
                punc['deprel'] = punctuationFunction

    return token_list


def printSentences(sent_list, options, outstream):
    """
    print all sentences in sent_list;
    tokens are dictionaries
    """
    # ??? should format string be unicode string regardless of options.encoding?

    format = []
    for j in range(len(options.output)):     # for each column
        if options.blanks:
            width = options.blanks[j]        # pad with blanks
            if j < len(options.output)-1:           # non-last column
                format_string = u'%'+`width`+u's '  # e.g. u"%-15s "
            else:                                   # last column
                format_string = u'%'+`width`+u's'   # e.g. u"%-15s"
        else:                                # separate by tab
            if j < len(options.output)-1:           # non-last column
                format_string = u'%s\t'
            else:                                   # last column
                format_string = u'%s'
        format.append(format_string)

    for sent in sent_list:                                  # for each sentence
        word_count = 0
        for i in range(1,len(sent)):                        # for each token
            token = sent[i]
            word_count += 1
            for j in range(len(options.output)):            # for each column
                column_name = options.output[j]
                if column_name == 'id':
                    output_string = format[j] % word_count
                else:
                    value = token[column_name]              # get value for column
                    if column_name == 'feats':
                        if value == []:                     # if no features:
                            value = emptyFeatsString        # use default value
                        else:
                            value = featsJoiner.join(value) # else: join
                    output_string = format[j] % value       # format string
                outstream.write(output_string.encode(options.encoding)) # print
            outstream.write("\n")  # newline at end of token
        outstream.write("\n")      # extra newline at end of token


def printSentences_tmp2(sent_list, options, outstream):
    """
    print all sentences in sent_list;
    tokens are class instances
    """
    # ??? should format string be unicode string regardless of options.encoding?

    format = []
    for j in range(len(options.output)):     # for each column
        if options.blanks:
            width = options.blanks[j]        # pad with blanks
            if j < len(options.output)-1:           # non-last column
                format_string = u'%'+`width`+u's '  # e.g. u"%-15s "
            else:                                   # last column
                format_string = u'%'+`width`+u's'   # e.g. u"%-15s"
        else:                                # separate by tab
            if j < len(options.output)-1:           # non-last column
                format_string = u'%s\t'
            else:                                   # last column
                format_string = u'%s'
        format.append(format_string)

    for sent in sent_list:                                  # for each sentence
        word_count = 0
        for i in range(1,len(sent)):                        # for each token
            token = sent[i]
            word_count += 1
            for j in range(len(options.output)):            # for each column
                column_name = options.output[j]
                if column_name == 'id':
                    output_string = format[j] % word_count  # format string
                    # ??? check that word count is same as ID?
                else:
                    value = getattr(token,column_name)     # get value for column
                    if column_name == 'feats':
                        if value == []:                     # if no features:
                            value = emptyFeatsString        # use default value
                        else:
                            value = featsJoiner.join(value) # else: join
                    output_string = format[j] % value       # format string
                outstream.write(output_string.encode(options.encoding)) # print
            outstream.write("\n")  # newline at end of token
        outstream.write("\n")      # extra newline at end of token
