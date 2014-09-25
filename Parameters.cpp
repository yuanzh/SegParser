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
	//tmpParams.clear();
	total.clear();
	parameters.resize(size, 0.0);
	//tmpParams.resize(size, 0.0);
	total.resize(size, 0.0);
}

Parameters::~Parameters() {
}

void Parameters::copyParams(Parameters* param) {
	parameters = param->parameters;
	//tmpParams = param->tmpParams;
	total = param->total;
	size = param->size;
	options = param->options;
}

void Parameters::averageParams(double avVal) {
	std::cout << "update time: " << avVal << std::endl;
	for (int j = 0; j < size; ++j)
		parameters[j] -= (avVal == 0 ? 0 : total[j] / avVal);
	//for (int j = 0; j < size; ++j)
	//	parameters[j] = tmpParams[j] - (avVal == 0 ? 0 : total[j] / avVal);
}

int Parameters::maxMatch(SegInstance& gold, SegInstance& pred, vector<int>& match) {
	ThrowException("should not go here max match");

	// match record the matching from gold to pred, size = goldSegNum, -1 means no match
	int goldSegNum = gold.size();
	int predSegNum = pred.size();

	boost::multi_array<int, 2> opt(boost::extents[goldSegNum][predSegNum]);
	for (int i = 0; i < goldSegNum; ++i)
		opt[i][0] = gold.element[i].formid == pred.element[0].formid;

	for (int j = 0; j < predSegNum; ++j)
		opt[0][j] = gold.element[0].formid == pred.element[j].formid;

	for (int i = 1; i < goldSegNum; ++i)
		for (int j = 1; j < predSegNum; ++j)
			opt[i][j] = max((int)opt[i-1][j],
					max((int)opt[i][j-1], (int)opt[i-1][j-1] + (int)(gold.element[i].formid == pred.element[j].formid)));

	// trace back
	match.resize(goldSegNum);
	int ret = opt[goldSegNum - 1][predSegNum - 1];
	int predIndex = predSegNum - 1;
	int goldIndex = goldSegNum - 1;
	while (predIndex >= 0 || goldIndex >= 0) {
		// option 1:
		int base = predIndex > 0 ? opt[goldIndex][predIndex - 1] : 0;
		if ((int)opt[goldIndex][predIndex] == base) {
			predIndex--;
			continue;
		}

		// option 2:
		base = goldIndex > 0 ? opt[goldIndex - 1][predIndex] : 0;
		if ((int)opt[goldIndex][predIndex] == base) {
			match[goldIndex] = -1;
			goldIndex--;
			continue;
		}

		// option 3:
		base = goldIndex > 0 && predIndex > 0 ? opt[goldIndex - 1][predIndex - 1] : 0;
		if (gold.element[goldIndex].formid == pred.element[predIndex].formid
				&& (int)opt[goldIndex][predIndex] == base + 1) {
			match[goldIndex] = predIndex;
			goldIndex--;
			predIndex--;
			continue;
		}

		ThrowException("trace back bug");
	}

	for (; goldIndex >= 0; goldIndex--)
		match[goldIndex] = -1;

	return ret;
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
					e += 1.5;
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
			e += 1.5;		// this value should not matter...
		}
		else if (goldEle.dep != predEle.dep) {
			e += 1.0;
		}
		else if (goldEle.labid != predEle.labid) {
			e += 0.5;
		}

	}

/*
	if (gold.currSegCandID == pred.currSegCandID) {
		SegElement& goldEle = gold.getCurrSeg().element[segid];
		SegElement& predEle = pred.getCurrSeg().element[segid];

		if (goldEle.dep != predEle.dep) {
			e = 1.0;
		}

	}
*/
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
				e += 1.5;
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
				e += 1.5;
			}
			else if (goldEle.dep != predEle.dep) {
				e += 1.0;
			}
			else if (goldEle.labid != predEle.labid) {
				e += 0.5;
			}
		}
	}

/*
	if (gold.currSegCandID == pred.currSegCandID) {
		for (int i = 0; i < gold.getCurrSeg().size(); ++i) {
			SegElement& goldEle = gold.getCurrSeg().element[i];
			SegElement& predEle = pred.getCurrSeg().element[i];

			if (goldEle.dep != predEle.dep) {
				e += 1.0;
			}
		}
	}
*/
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
			//tmpParams[diffFv->binaryIndex[i]] += alpha;
			total[diffFv->binaryIndex[i]] += upd * alpha;
		}
		for (unsigned int i = 0; i < diffFv->negBinaryIndex.size(); ++i) {
			parameters[diffFv->negBinaryIndex[i]] -= alpha;
			//tmpParams[diffFv->negBinaryIndex[i]] -= alpha;
			total[diffFv->negBinaryIndex[i]] -= upd * alpha;
		}
		for (unsigned int i = 0; i < diffFv->normalIndex.size(); ++i) {
			double val = min(2.0, max(-2.0, diffFv->normalValue[i]));
			parameters[diffFv->normalIndex[i]] += alpha * val;
			//tmpParams[diffFv->normalIndex[i]] += alpha * val;
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
