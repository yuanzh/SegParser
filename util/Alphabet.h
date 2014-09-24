/*
 * Alphabet.h
 *
 *  Created on: Mar 28, 2014
 *      Author: yuanz
 */

#ifndef ALPHABET_H_
#define ALPHABET_H_

#include <string>
#include <vector>
#include <unordered_map>

namespace segparser {

using namespace std;

class Alphabet {
public:
	Alphabet();
	Alphabet(int capacity);
	virtual ~Alphabet();

	int lookupIndex (const string& entry, bool addIfNotPresent);
	int lookupIndex (const string& entry);
	void toArray(vector<string>& key);
	int size();
	void stopGrowth();

	void writeObject (FILE* fs);
	void readObject (FILE* fs);

private:
	unordered_map<string, int> map;
	int numEntries;
	bool growthStopped;
};

} /* namespace segparser */
#endif /* ALPHABET_H_ */
