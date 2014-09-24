/*
 * DependencyDecoder.h
 *
 *  Created on: Apr 8, 2014
 *      Author: yuanz
 */

#ifndef DEPENDENCYDECODER_H_
#define DEPENDENCYDECODER_H_

#include "../Options.h"
#include "../DependencyInstance.h"
#include "../SegParser.h"
#include "../util/StringUtils.h"
#include "../util/Random.h"
#include "../FeatureExtractor.h"

namespace segparser {

class FeatureExtractor;
class CacheTable;

class DependencyDecoder {
public:
	DependencyDecoder(Options* options);
	virtual ~DependencyDecoder();

	static DependencyDecoder* createDependencyDecoder(Options* options, int mode, int thread, bool isTrain);

	virtual void initialize() {

	}

	virtual void shutdown() {

	}

	virtual void decode(DependencyInstance* inst, DependencyInstance* gold, FeatureExtractor* fe) {
		ThrowException("should not go virtual decode function");
	}

	virtual void train(DependencyInstance* gold, DependencyInstance* pred, FeatureExtractor* fe, int trainintIter) {
		ThrowException("should not go virtual train function");
	}

	void prune(DependencyInstance* inst, HeadIndex& m, FeatureExtractor* fe, vector<bool>& pruned);

	int getUpdateTimes() {
		return updateTimes;
	}

	void resetUpdateTimes() {
		updateTimes = 0;
	}

	void initInst(DependencyInstance* inst, FeatureExtractor* fe);
	void removeGoldInfo(DependencyInstance* inst);

	int seed;
	Options* options;

	double thresh;

	int failTime;

	int getBottomUpOrder(DependencyInstance* inst, HeadIndex& arg, vector<HeadIndex>& idx, int id);
	double sampleSeg1O(DependencyInstance* inst, DependencyInstance* gold, FeatureExtractor* fe, int wordID, Random& r);
	double samplePos1O(DependencyInstance* inst, DependencyInstance* gold, FeatureExtractor* fe, int wordID, Random& r);
	bool randomWalkSampler(DependencyInstance* pred, DependencyInstance* gold, FeatureExtractor* fe, CacheTable* cache,
			vector<bool>& toBeSampled, Random& r, double T);

	double sampleTime;
	double climbTime;
protected:
	int updateTimes;

	bool isAncestor(DependencyInstance* s, HeadIndex& h, HeadIndex m);
	bool isProj(DependencyInstance* s, HeadIndex& h, HeadIndex& m);
	int samplePoint(vector<double>& prob, Random& r);
	void convertScoreToProb(vector<double>& score);
	double getSegPosProb(WordInstance& word);
	void getFirstOrderVec(DependencyInstance* inst, DependencyInstance* gold,
			FeatureExtractor* fe, HeadIndex& m, CacheTable* cache, bool treeConstraint, vector<HeadIndex>& candH, vector<double>& score);
	double getMHDepProb(DependencyInstance* inst, DependencyInstance* gold, FeatureExtractor* fe, int wordID);
	double sampleSegPos(DependencyInstance* inst, DependencyInstance* gold, int wordID, Random& r);
	double sampleMHDepProb(DependencyInstance* inst, DependencyInstance* gold, FeatureExtractor* fe, int wordID, Random& r);
	void updateSeg(DependencyInstance* inst, WordInstance& word, int newSeg);
	void updatePos(WordInstance& word, SegElement& ele, int newPos);

	void cycleErase(DependencyInstance* inst, HeadIndex i, vector<bool>& toBeSampled);
	void updateSeg(DependencyInstance* pred, DependencyInstance* gold, HeadIndex& m,
			int newSeg, int oldSeg, int baseOptSeg, int baseOptPos, vector<int>& oldPos, vector<HeadIndex>& oldHeadIndex,
			vector<HeadIndex>& relatedChildren, vector<int>& relatedOldParent);
};

} /* namespace segparser */
#endif /* DEPENDENCYDECODER_H_ */
