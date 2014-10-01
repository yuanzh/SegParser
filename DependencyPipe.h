/*
 * DependencyPipe.h
 *
 *  Created on: Apr 4, 2014
 *      Author: yuanz
 */

#ifndef DEPENDENCYPIPE_H_
#define DEPENDENCYPIPE_H_

#include "Options.h"
#include "DependencyInstance.h"
#include "util/Alphabet.h"
#include "util/FeatureAlphabet.h"
#include <unordered_set>

namespace segparser {

using namespace std;

class DependencyPipe {
public:
	DependencyPipe(Options* options);
	virtual ~DependencyPipe();

	void loadCoarseMap(string& file);
	void setAndCheckOffset();
	void buildDictionary(string& goldfile);
	void closeAlphabets();
	void createAlphabet(string& goldfile);
	vector<inst_ptr> createInstances(string goldFile);

	int findRightNearestChildID(vector<HeadIndex>& child, HeadIndex id);
	void createFeatureVector(DependencyInstance* inst, FeatureVector* fv);
	int getBinnedDistance(int x);
	void createArcFeatureVector(DependencyInstance* inst, HeadIndex& headIndex, HeadIndex& modIndex, FeatureVector* fv);
	void createTripsFeatureVector(DependencyInstance* inst, HeadIndex& par, HeadIndex& ch1, HeadIndex& ch2, FeatureVector* fv);
	void createSibsFeatureVector(DependencyInstance* inst, HeadIndex& ch1, HeadIndex& ch2, bool isST, FeatureVector* fv);
	void createGPCFeatureVector(DependencyInstance* inst, HeadIndex& gp, HeadIndex& par, HeadIndex& c, FeatureVector* fv);
	void createPos1OFeatureVector(DependencyInstance* inst, HeadIndex& m, FeatureVector* fv);
	void createPosHOFeatureVector(DependencyInstance* inst, HeadIndex& m, FeatureVector* fv);
	void createSegFeatureVector(DependencyInstance* inst, int wordid, FeatureVector* fv);
	void createNonProjFeatureVector(DependencyInstance* inst, FeatureVector* fv);
	void addCode(int type, long code, double val, FeatureVector* fv);
	void addCode(int type, long code, FeatureVector* fv);

	FeatureAlphabet* dataAlphabet;

	// dictionary
	Alphabet* typeAlphabet;
	Alphabet* posAlphabet;			// pos
	Alphabet* lexAlphabet;			// lemma, word
	unordered_set<string> suffixList;

	unordered_map<string, string> coarseMap;

	// encoder
	FeatureEncoder* fe;
private:
	Options* options;

	void buildSuffixList();
};

} /* namespace segparser */
#endif /* DEPENDENCYPIPE_H_ */
