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

	bool trainPruner;

	int learningMode;
	int testingMode;

	// parameter
	int numIters;
	int maxHead;
	double pruneThresh;

	int trainSentences;
	int testSentences;
	int maxLength;			// maximum length of the sentences during *training*

	int devThread;
	int trainThread;		// only useful when hill climbing training

	int seed;
	double regC;

	// feature;
	bool useCS;			// consecutive sibling
	bool useGP;			// grandparent
	bool useHO;			// grand-sibling, tri-sibling and high order and global
	bool useSP;			// seg pos feature

	int trainConvergeIter;	// for hill climbing
	int testConvergeIter;

	bool evalPunc;
	bool useTedEval;
	bool jointSegPos;	// joint model or pipeline
	int earlyStop;		// early stop strategy in training

	bool saveBestModel;
	double bestScore;

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
