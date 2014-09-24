/*
 * SampleRankDecoder.h
 *
 *  Created on: Apr 8, 2014
 *      Author: yuanz
 */

#ifndef SAMPLERANKDECODER_H_
#define SAMPLERANKDECODER_H_

#include "DependencyDecoder.h"

namespace segparser {

class SampleRankDecoder: public segparser::DependencyDecoder {
public:
	SampleRankDecoder(Options* options);
	virtual ~SampleRankDecoder();

	void decode(DependencyInstance* inst, DependencyInstance* gold, FeatureExtractor* fe);
	void train(DependencyInstance* gold, DependencyInstance* pred, FeatureExtractor* fe, int trainintIter);
private:
    Random r;

    double getInitTemp(DependencyInstance* inst, FeatureExtractor* fe);
	bool accept(double newScore, double oldScore, bool useAnnealingTransit, double old2newProb, double new2oldProb,
			double temperature, Random& r);
	void copyVarInfo(DependencyInstance* orig, DependencyInstance* curr);
};

} /* namespace segparser */
#endif /* SAMPLERANKDECODER_H_ */
