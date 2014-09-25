/*
 * HillClimbingDecoder.h
 *
 *  Created on: May 1, 2014
 *      Author: yuanz
 */

#ifndef HILLCLIMBINGDECODER_H_
#define HILLCLIMBINGDECODER_H_

#include "DependencyDecoder.h"
#include <pthread.h>

namespace segparser {

class HillClimbingDecoder: public segparser::DependencyDecoder {
public:
	HillClimbingDecoder(Options* options, int thread, int convergeIter);
	virtual ~HillClimbingDecoder();

	void initialize();
	void shutdown();
	void startTask(DependencyInstance* pred, DependencyInstance* gold, FeatureExtractor* fe);
	void waitAndGetResult(DependencyInstance* inst);
	void decode(DependencyInstance* inst, DependencyInstance* gold, FeatureExtractor* fe);
	void train(DependencyInstance* gold, DependencyInstance* pred, FeatureExtractor* fe, int trainintIter);
	double findOptHead(DependencyInstance* pred, DependencyInstance* gold, HeadIndex& m, FeatureExtractor* fe, CacheTable* cache);
	double findOptBigramHead(DependencyInstance* pred, DependencyInstance* gold, HeadIndex& m, HeadIndex& n, FeatureExtractor* fe, CacheTable* cache);
	double findOptPos(DependencyInstance* pred, DependencyInstance* gold, HeadIndex& m, FeatureExtractor* fe, CacheTable* cache);
	double findOptSeg(DependencyInstance* pred, DependencyInstance* gold, HeadIndex& m, FeatureExtractor* fe, CacheTable* cache);

	void debug(string msg, int id);

	vector<pthread_t> threadID;
	vector<pthread_mutex_t> taskMutex;
	vector<pthread_cond_t> taskStartCond;
	vector<pthread_cond_t> taskDoneCond;
	vector<bool> taskDone;
	vector<bool> threadExit;

	double bestScore;
	VariableInfo best;
	int unChangeIter;		// converge criteria
	pthread_mutex_t updateMutex;

	DependencyInstance* pred;
	DependencyInstance* gold;
	FeatureExtractor* fe;

	int thread;

	int convergeIter;

	int earlyStopIter;
	bool samplePos;
	bool sampleSeg;
};

} /* namespace segparser */
#endif /* HILLCLIMBINGDECODER_H_ */
