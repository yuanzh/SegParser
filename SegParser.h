/*
 * SegParser.h
 *
 *  Created on: Mar 19, 2014
 *      Author: yuanz
 */

#ifndef SEGPARSER_H_
#define SEGPARSER_H_

#include <boost/multi_array.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>
#include "DependencyPipe.h"
#include "decoder/DevelopmentThread.h"
#include "Parameters.h"
#include "Options.h"
#include "decoder/DependencyDecoder.h"

namespace segparser {

using namespace std;
using namespace boost;

class Parameters;
class DependencyDecoder;
class DevelopmentThread;

class TestClass {
public:
	int a;
	TestClass() {
		a = 2;
	}
};

typedef boost::shared_ptr<TestClass> test_ptr;

class SegParser {
public:
	vector<test_ptr>* ptr;
	TestClass* a;
	pthread_mutex_t mutex;
	//multi_array<TestClass*, 2>* ptr;
	//multi_array<int, 2>* score;

	SegParser(DependencyPipe* pipe, Options* options);
	virtual ~SegParser();
	void train(vector<inst_ptr>& il);
	void trainingIter(vector<inst_ptr>& goldList, vector<inst_ptr>& predList, int iter);
	void checkDevStatus(int iter);

	void outputWeight(ofstream& fout, int type, Parameters* params);
	void outputWeight(string fStr);
	void loadModel(string file);
	void saveModel(string file, Parameters* params);

	void closeDecoder();

	DependencyPipe* pipe;
	DependencyDecoder* decoder;
	Parameters* parameters;
	Parameters* devParams;
	DevelopmentThread* dt;
	Options* options;
	SegParser* pruner;

private:
	int devTimes;
	double bestLabAcc;
	double bestUlabAcc;
	int restartIter;

};

} /* namespace segparser */
#endif /* SEGPARSER_H_ */
