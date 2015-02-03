/*
 * Parameters.cpp
 *
 *  Created on: Apr 6, 2014
 *      Author: yuanz
 */

#include "Parameters.h"
#include <boost/multi_array.hpp>
#include "util/SerializationUtils.h"

namespace segparser {

Parameters::Parameters(int size, Options* options)
	: size(size), options(options){
	parameters.clear();
	total.clear();
	parameters.resize(size, 0.0);
	total.resize(size, 0.0);
}

Parameters::~Parameters() {
}

void Parameters::copyParams(Parameters* param) {
	parameters = param->parameters;
	total = param->total;
	size = param->size;
	options = param->options;
}

void Parameters::averageParams(double avVal) {
	std::cout << "update time: " << avVal << std::endl;
	for (int j = 0; j < size; ++j)
		parameters[j] -= (avVal == 0 ? 0 : total[j] / avVal);
}

double Parameters::numError(DependencyInstance* gold, DependencyInstance* pred) {
	ThrowException("should not be here");
	double e = 0.0;

	for (int i = 1; i < gold->numWord; ++i) {
		SegInstance& goldSeg = gold->word[i].getCurrSeg();
		SegInstance& predSeg = pred->word[i].getCurrSeg();

		if (gold->word[i].currSegCandID != pred->word[i].currSegCandID) {
			e += 1.5 * predSeg.size();
		}
		else {
			// compare match element
			for (int j = 0; j < predSeg.size(); ++j) {
				SegElement& goldEle = goldSeg.element[j];
				SegElement& predEle = predSeg.element[j];

				if (goldEle.currPosCandID != predEle.currPosCandID) {
					e += 1.0;
				}
				else if (goldEle.dep != predEle.dep) {
					e += 1.0;
				}
				else if (goldEle.labid != predEle.labid) {
					e += 0.5;
				}
			}
		}

	}
	return e;
}

double Parameters::elementError(WordInstance& gold, WordInstance& pred, int segid) {
	double e = 0.0;

	if (gold.currSegCandID != pred.currSegCandID) {
		e += 2.0;		// this value should not matter...
	}
	else {
		SegElement& goldEle = gold.getCurrSeg().element[segid];
		SegElement& predEle = pred.getCurrSeg().element[segid];

		if (goldEle.currPosCandID != predEle.currPosCandID) {
			e += 1.0;		// this value should not matter...
		}
		else if (goldEle.dep != predEle.dep) {
			e += 1.0;
		}
		else if (goldEle.labid != predEle.labid) {
			e += 0.5;
		}

	}

	return e;
}

double Parameters::wordError(WordInstance& gold, WordInstance& pred) {
	double e = 0.0;

	if (gold.currSegCandID != pred.currSegCandID) {
		e += 1.0 * (gold.getCurrSeg().size() + pred.getCurrSeg().size());
	}
	else {
		assert(gold.getCurrSeg().size() == pred.getCurrSeg().size());

		for (int i = 0; i < gold.getCurrSeg().size(); ++i) {
			SegElement& goldEle = gold.getCurrSeg().element[i];
			SegElement& predEle = pred.getCurrSeg().element[i];

			assert(goldEle.labid == predEle.labid);

			if (goldEle.currPosCandID != predEle.currPosCandID) {
				e += 1.0;
			}
			else if (goldEle.dep != predEle.dep) {
				e += 1.0;
			}
			else if (goldEle.labid != predEle.labid) {
				e += 0.5;
			}
		}
	}
	return e;
}

double Parameters::wordDepError(WordInstance& gold, WordInstance& pred) {
	double e = 0.0;

	if (gold.currSegCandID != pred.currSegCandID) {
		e += 1.0 * (gold.getCurrSeg().size() + pred.getCurrSeg().size());
	}
	else {
		assert(gold.getCurrSeg().size() == pred.getCurrSeg().size());

		for (int i = 0; i < gold.getCurrSeg().size(); ++i) {
			SegElement& goldEle = gold.getCurrSeg().element[i];
			SegElement& predEle = pred.getCurrSeg().element[i];

			assert(goldEle.labid == predEle.labid);

			if (goldEle.currPosCandID != predEle.currPosCandID) {
				e += 1.0;
			}
			else if (goldEle.dep != predEle.dep) {
				e += 1.0;
			}
			else if (goldEle.labid != predEle.labid) {
				e += 0.5;
			}
		}
	}

	return e;
}

void Parameters::update(DependencyInstance* target, DependencyInstance* curr,
		FeatureVector* diffFv, double loss, FeatureExtractor* fe, int upd) {
	// upd start from 0

	//double e = numError(gold, pred);
	//double loss = e - diffScore;

	if (loss < 1e-4)
		return;

	double l2norm = diffFv->dotProduct(diffFv);
	if (l2norm <= 1e-6)
		return;

	double alpha = loss/l2norm;

	if (alpha > options->regC)
		alpha = options->regC;

	if (alpha > 0) {
		// update theta
		for (unsigned int i = 0; i < diffFv->binaryIndex.size(); ++i) {
			parameters[diffFv->binaryIndex[i]] += alpha;
			total[diffFv->binaryIndex[i]] += upd * alpha;
		}
		for (unsigned int i = 0; i < diffFv->negBinaryIndex.size(); ++i) {
			parameters[diffFv->negBinaryIndex[i]] -= alpha;
			total[diffFv->negBinaryIndex[i]] -= upd * alpha;
		}
		for (unsigned int i = 0; i < diffFv->normalIndex.size(); ++i) {
			double val = min(2.0, max(-2.0, diffFv->normalValue[i]));
			parameters[diffFv->normalIndex[i]] += alpha * val;
			total[diffFv->normalIndex[i]] += upd * alpha * val;
		}
	}
}

double Parameters::getScore(FeatureVector* fv) {
	double score = 0.0;
	for (unsigned int i = 0; i < fv->binaryIndex.size(); ++i) {
		score += parameters[fv->binaryIndex[i]];
	}
	for (unsigned int i = 0; i < fv->negBinaryIndex.size(); ++i) {
		score -= parameters[fv->negBinaryIndex[i]];
	}
	for (unsigned int i = 0; i < fv->normalIndex.size(); ++i) {
		score += parameters[fv->normalIndex[i]] * fv->normalValue[i];
	}
	return score;
}

void Parameters::writeParams(FILE* fs) {
	CHECK(WriteInteger(fs, size));
	CHECK(WriteDoubleArray(fs, parameters));
}

void Parameters::readParams(FILE* fs) {
	CHECK(ReadInteger(fs, &size));
	CHECK(ReadDoubleArray(fs, &parameters));
}

} /* namespace segparser */
