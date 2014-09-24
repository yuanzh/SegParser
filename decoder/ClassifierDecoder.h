/*
 * ClassifierDecoder.h
 *
 *  Created on: May 7, 2014
 *      Author: yuanz
 */

#ifndef CLASSIFIERDECODER_H_
#define CLASSIFIERDECODER_H_

#include "DependencyDecoder.h"

namespace segparser {

class ClassifierDecoder: public segparser::DependencyDecoder {
public:
	ClassifierDecoder(Options* options);
	virtual ~ClassifierDecoder();

	void decode(DependencyInstance* inst, DependencyInstance* gold, FeatureExtractor* fe);
	void train(DependencyInstance* gold, DependencyInstance* pred, FeatureExtractor* fe, int trainintIter);

private:
	void findOptHead(DependencyInstance* pred, DependencyInstance* gold, HeadIndex& m, FeatureExtractor* fe, CacheTable* cache);

};

} /* namespace segparser */
#endif /* CLASSIFIERDECODER_H_ */
