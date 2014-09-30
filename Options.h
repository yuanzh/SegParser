/*
 * Options.h
 *
 *  Created on: Mar 27, 2014
 *      Author: yuanz
 */

#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <string>

namespace segparser {

using namespace std;

class Options {
public:
public:
	// file name
	string trainFile;
	string testFile;

	string outFile;
	string modelName;

	int lang;

	// model type
	bool train;
	bool test;
	bool eval;

	bool trainPruner;

	bool proj;
	bool labeled;

	int learningMode;
	int testingMode;

	// parameter
	int numIters;
	int maxHead;
	double pruneThresh;

	int trainSentences;
	int testSentences;
	int maxLength;

	int devThread;
	int trainThread;		// only useful when hill climbing training

	int seed;
	double regC;

	bool updateGold;
	bool heuristicDep;

	// feature;
	bool useCS;			// consecutive sibling
	bool useGP;			// grandparent
	bool useGS;			// grand-sibling
	bool useTS;			// tri-sibling
	bool useHB;			// head-bigram
	bool useAS;			// arbitrary sibling
	bool useGGPC;		// grand-grand-parent;
	bool usePSC;		// sibling grand-child
	bool useHO;			// high order and global
	bool useSP;			// seg pos feature

	int trainConvergeIter;	// for hill climbing
	int testConvergeIter;
	int restartIter;	// for sample rank

	bool evalPunc;
	bool useTedEval;
	bool jointSegPos;
	int earlyStop;

	bool addLoss;

	Options();
	virtual ~Options();

	void processArguments(int argc, char** argv);
	void setPrunerOptions();
	void outputArg();

private:
	int findLang(string file);
};

} /* namespace segparser */
#endif /* OPTIONS_H_ */
