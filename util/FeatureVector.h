/*
 * FeatureVector.h
 *
 *  Created on: Mar 28, 2014
 *      Author: yuanz
 */

#ifndef FEATUREVECTOR_H_
#define FEATUREVECTOR_H_

#include <vector>

namespace segparser {

using namespace std;

class FeatureVector {
public:
	vector<int> binaryIndex;
	vector<int> negBinaryIndex;
	vector<int> normalIndex;
	vector<double> normalValue;

	FeatureVector();
	virtual ~FeatureVector();

	void clear();
	void add(int index, double value);
	void addBinary(int index);
	void addNegBinary(int index);
	void concat(FeatureVector* fv);
	void concatNeg(FeatureVector* fv);
	double dotProduct(FeatureVector* fv);
	double dotProduct(vector<double>& param);

	static void initVec(unsigned int _rows);
	void output();
private:
	static vector<double> dpVec;
	static unsigned int rows;
};

} /* namespace segparser */
#endif /* FEATUREVECTOR_H_ */
