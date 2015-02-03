/*
 * Parameters.h
 *
 *  Created on: Apr 6, 2014
 *      Author: yuanz
 */

#ifndef PARAMETERS_H_
#define PARAMETERS_H_

#include <vector>
#include "Options.h"
#include "DependencyInstance.h"
#include "util/FeatureVector.h"
#include "FeatureExtractor.h"

namespace segparser {

using namespace std;

class FeatureExtractor;

class Parameters {
public:
	vector<double> parameters;
	vector<double> total;
	int size;

	Parameters(int size, Options* options);
	virtual ~Parameters();

	void copyParams(Parameters* param);
	void averageParams(double avVal);
	void update(DependencyInstance* gold, DependencyInstance* pred,
			FeatureVector* diffFv, double loss, FeatureExtractor* fe, int upd);
	double getScore(FeatureVector* fv);

	void writeParams(FILE* fs);
	void readParams(FILE* fs);

	double elementError(WordInstance& gold, WordInstance& pred, int segid);
	double wordError(WordInstance& gold, WordInstance& pred);
	double wordDepError(WordInstance& gold, WordInstance& pred);
private:
	Options* options;

	int maxMatch(SegInstance& gold, SegInstance& pred, vector<int>& match);
	double numError(DependencyInstance* gold, DependencyInstance* pred);
};

} /* namespace segparser */
#endif /* PARAMETERS_H_ */
