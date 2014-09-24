/*
 * Alphabet.cpp
 *
 *  Created on: Mar 28, 2014
 *      Author: yuanz
 */

#include "Alphabet.h"
#include "SerializationUtils.h"

namespace segparser {

Alphabet::Alphabet(int capacity) {
	map.reserve(capacity);
	numEntries = 0;
	growthStopped = false;
}

Alphabet::Alphabet() {
	Alphabet(10000);
}

Alphabet::~Alphabet() {
}

int Alphabet::lookupIndex (const string& entry, bool addIfNotPresent) {
	int ret = 0;
	if (map.find(entry) == map.end()) {
		if (!growthStopped && addIfNotPresent) {
			ret = numEntries + 1;		// index start from 1
			numEntries++;
			map[entry] = ret;
		}
	}
	else {
		ret = map[entry];
	}
	return ret;
}

int Alphabet::lookupIndex (const string& entry) {
	return lookupIndex (entry, true);
}

void Alphabet::toArray(vector<string>& key) {
	key.reserve(numEntries);
	for (auto kv : map) {
		key.push_back(kv.first);
	}
}

int Alphabet::size() {
	return numEntries + 1;
}

void Alphabet::stopGrowth() {
	growthStopped = true;
}

void Alphabet::writeObject (FILE* fs) {
	CHECK(WriteInteger(fs, numEntries));
	CHECK(WriteStringIntegerMap(fs, map));
	CHECK(WriteBool(fs, growthStopped));
}

void Alphabet::readObject (FILE* fs) {
	CHECK(ReadInteger(fs, &numEntries));
	CHECK(ReadStringIntegerMap(fs, &map));
	CHECK(ReadBool(fs, &growthStopped));
}

} /* namespace segparser */
