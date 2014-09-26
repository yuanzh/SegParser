/*
 * DependencyReader.h
 *
 *  Created on: Mar 27, 2014
 *      Author: yuanz
 */

#ifndef DEPENDENCYREADER_H_
#define DEPENDENCYREADER_H_

#include <fstream>
#include "../Options.h"
#include "../DependencyInstance.h"

namespace segparser {

using namespace std;

class DependencyReader {
public:
	DependencyReader();
	DependencyReader(Options* options, string file);
	virtual ~DependencyReader();

	void startReading(Options* options, string file);
	void startReading(string file);
	void close();
	inst_ptr nextInstance();

	bool hasCandidate;
	bool isTrain;

private:
	ifstream fin;
	Options* options;

	HeadIndex parseHeadIndex(string str);
	void addGoldSegElement(WordInstance* word, string form, string lemma, string pos,
			string morphStr, int segid, int hwordid, int hsegid, string lab);
	void addGoldSegToCand(WordInstance* word);
	void normalizeProb(WordInstance* word);
	void addSegCand(WordInstance* word, string str);
	string normalize(string s);
	void concatSegStr(WordInstance* word);
};

} /* namespace segparser */
#endif /* DEPENDENCYREADER_H_ */
