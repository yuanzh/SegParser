/*
 * DevelopmentThread.h
 *
 *  Created on: Apr 16, 2014
 *      Author: yuanz
 */

#ifndef DEVELOPMENTTHREAD_H_
#define DEVELOPMENTTHREAD_H_

#include <string>
#include <vector>
#include <unordered_map>
#include "../io/DependencyReader.h"
#include "../SegParser.h"

namespace segparser {

using namespace std;

class DependencyReader;
class SegParser;

class DevelopmentThread {
public:
	DevelopmentThread();
	virtual ~DevelopmentThread();

	void start(string devfile, string devoutfile, SegParser* sp, bool verbal);
	void evaluate(DependencyInstance* inst, DependencyInstance* gold);
	double computeTedEval();

	bool isDevTesting;

	string devfile;
	string devoutfile;

	DependencyReader reader;

	int currProcessID;
	int currFinishID;

	pthread_mutex_t processMutex;
	pthread_mutex_t finishMutex;

	double wordNum;
	double corrWordSegNum;
	double goldSegNum;
	double predSegNum;
	double corrSegNum;
	double corrPosNum;
	double goldDepNum;
	double predDepNum;
	double corrDepNum;

	pthread_t workThread;
	pthread_t outputThread;
	vector<pthread_t> decodeThread;
	int decodeThreadNum;

	unordered_map<int, inst_ptr> id2Pred;
	int finishThreadNum;

	SegParser* sp;
	Options* options;
	bool verbal;

private:
	bool isPunc(string& pos);
	int numSegWithoutPunc(DependencyInstance* inst);
	string normalize(string form);
};

} /* namespace segparser */
#endif /* DEVELOPMENTTHREAD_H_ */
