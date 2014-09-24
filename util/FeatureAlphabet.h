/*
 * FeatureAlphabet.h
 *
 *  Created on: Mar 28, 2014
 *      Author: yuanz
 */

#ifndef FEATUREALPHABET_H_
#define FEATUREALPHABET_H_

#include <unordered_map>
#include <string>
#include "../FeatureEncoder.h"

namespace segparser {

using namespace std;

class FeatureAlphabet {
public:
	FeatureAlphabet(int capacity);
	FeatureAlphabet();
	virtual ~FeatureAlphabet();

	int lookupIndex (const string& entry, bool addIfNotPresent);
	int lookupIndex (const string& entry);
	unordered_map<uint64_t, int>* getMap(int type);
	int lookupIndex(const int type, const uint64_t entry, bool addIfNotPresent);
	int lookupIndex(unordered_map<uint64_t, int>& intmap, const uint64_t entry, bool addIfNotPresent);
	int lookupIndex(const int type, const uint64_t entry);
	int size();
	void stopGrowth();
	void writeObject (FILE* fs);
	void readObject (FILE* fs);

	unordered_map<uint64_t, int> arcMap;
	unordered_map<uint64_t, int> secondOrderMap;
	unordered_map<uint64_t, int> thirdOrderMap;
	unordered_map<uint64_t, int> highOrderMap;

private:
	int numEntries;
	bool growthStopped;

	unordered_map<uint64_t, int>* table[TemplateType::COUNT];
};

} /* namespace segparser */
#endif /* FEATUREALPHABET_H_ */
