/*
 * FeatureVector.cpp
 *
 *  Created on: Mar 28, 2014
 *      Author: yuanz
 */

#include "FeatureVector.h"
#include <assert.h>
#include <iostream>

namespace segparser {

vector<double> FeatureVector::dpVec;
unsigned int FeatureVector::rows;

FeatureVector::FeatureVector() {
}

FeatureVector::~FeatureVector() {
}

void FeatureVector::clear() {
	binaryIndex.clear();
	negBinaryIndex.clear();
	normalIndex.clear();
	normalValue.clear();
}

void FeatureVector::add(int index, double value) {
	normalIndex.push_back(index);
	normalValue.push_back(value);
}

void FeatureVector::addBinary(int index) {
	binaryIndex.push_back(index);
}

void FeatureVector::addNegBinary(int index) {
	negBinaryIndex.push_back(index);
}

void FeatureVector::concat(FeatureVector* fv) {
	for(unsigned int i = 0; i < fv->binaryIndex.size(); ++i) {
		binaryIndex.push_back(fv->binaryIndex[i]);
	}
	for(unsigned int i = 0; i < fv->negBinaryIndex.size(); ++i) {
		negBinaryIndex.push_back(fv->negBinaryIndex[i]);
	}
	for(unsigned int i = 0; i < fv->normalIndex.size(); ++i) {
		normalIndex.push_back(fv->normalIndex[i]);
		normalValue.push_back(fv->normalValue[i]);
	}
}

void FeatureVector::concatNeg(FeatureVector* fv) {
	for(unsigned int i = 0; i < fv->binaryIndex.size(); ++i) {
		negBinaryIndex.push_back(fv->binaryIndex[i]);
	}
	for(unsigned int i = 0; i < fv->negBinaryIndex.size(); ++i) {
		binaryIndex.push_back(fv->negBinaryIndex[i]);
	}
	for(unsigned int i = 0; i < fv->normalIndex.size(); ++i) {
		normalIndex.push_back(fv->normalIndex[i]);
		normalValue.push_back(-fv->normalValue[i]);
	}
}

double FeatureVector::dotProduct(FeatureVector* fv) {
	double result = 0.0;

//	for(unsigned int i = 0; i < binaryIndex.size(); ++i) {
//		assert(dpVec[binaryIndex[i]] == 0.0);
//	}
//	for(unsigned int i = 0; i < negBinaryIndex.size(); ++i) {
//		assert(dpVec[negBinaryIndex[i]] == 0.0);
//	}
//	for(unsigned int i = 0; i < normalIndex.size(); ++i) {
//		assert(dpVec[normalIndex[i]] == 0.0);
//	}

	for(unsigned int i = 0; i < binaryIndex.size(); ++i) {
		dpVec[binaryIndex[i]] += 1.0;
	}
	for(unsigned int i = 0; i < negBinaryIndex.size(); ++i) {
		dpVec[negBinaryIndex[i]] -= 1.0;
	}
	for(unsigned int i = 0; i < normalIndex.size(); ++i) {
		double val = min(2.0, max(-2.0, normalValue[i]));
		dpVec[normalIndex[i]] += val;
	}

	for(unsigned int i = 0; i < fv->binaryIndex.size(); ++i) {
		result += dpVec[fv->binaryIndex[i]];
	}
	for(unsigned int i = 0; i < fv->negBinaryIndex.size(); ++i) {
		result -= dpVec[fv->negBinaryIndex[i]];
	}
	for(unsigned int i = 0; i < fv->normalIndex.size(); ++i) {
		double val = min(2.0, max(-2.0, fv->normalValue[i]));
		result += dpVec[fv->normalIndex[i]] * val;
	}

	for(unsigned int i = 0; i < binaryIndex.size(); ++i) {
		dpVec[binaryIndex[i]] = 0.0;
	}
	for(unsigned int i = 0; i < negBinaryIndex.size(); ++i) {
		dpVec[negBinaryIndex[i]] = 0.0;
	}
	for(unsigned int i = 0; i < normalIndex.size(); ++i) {
		dpVec[normalIndex[i]] = 0.0;
	}

	return result;
}

double FeatureVector::dotProduct(vector<double>& param) {
	// get score
	double score = 0.0;
	for(unsigned int i = 0; i < binaryIndex.size(); ++i) {
		score += param[binaryIndex[i]];
	}
	for(unsigned int i = 0; i < negBinaryIndex.size(); ++i) {
		score -= param[negBinaryIndex[i]];
	}
	for(unsigned int i = 0; i < normalIndex.size(); ++i) {
		score += param[normalIndex[i]] * normalValue[i];
	}
	return score;
}

void FeatureVector::initVec(unsigned int _rows) {
	rows = _rows;
	dpVec.resize(rows);
}

void FeatureVector::output() {
	cout << "bi: ";
	for(unsigned int i = 0; i < binaryIndex.size(); ++i) {
		cout << binaryIndex[i] << " ";
	}
	cout << endl;
	cout << "nbi: ";
	for(unsigned int i = 0; i < negBinaryIndex.size(); ++i) {
		cout << negBinaryIndex[i] << " ";
	}
	cout << endl;
	cout << "ni: ";
	for(unsigned int i = 0; i < normalIndex.size(); ++i) {
		cout << normalIndex[i] << " ";
	}
	cout << endl;
}

} /* namespace segparser */
