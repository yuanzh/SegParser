/*
 * FeatureAlphabet.cpp
 *
 *  Created on: Mar 28, 2014
 *      Author: yuanz
 */

#include "FeatureAlphabet.h"
#include "SerializationUtils.h"
#include "../FeatureEncoder.h"

namespace segparser {

FeatureAlphabet::FeatureAlphabet (int capacity) {
	arcMap.reserve(capacity);
	secondOrderMap.reserve(capacity);
	thirdOrderMap.reserve(capacity);
	highOrderMap.reserve(capacity);
	numEntries = 0;
	growthStopped = false;

	table[TemplateType::TArc] = &arcMap;
	table[TemplateType::TSecondOrder] = &secondOrderMap;
	table[TemplateType::TThirdOrder] = &thirdOrderMap;
	table[TemplateType::THighOrder] = &highOrderMap;
}

FeatureAlphabet::FeatureAlphabet() {
	FeatureAlphabet(10000);
}

FeatureAlphabet::~FeatureAlphabet() {
}

unordered_map<uint64_t, int>* FeatureAlphabet::getMap(int type) {

	unordered_map<uint64_t, int>* intmap = NULL;
	if (type < TemplateType::COUNT)
		intmap = table[type];
	else
		ThrowException("undefined template type");
	return intmap;
}

int FeatureAlphabet::lookupIndex(const int type, const uint64_t entry, bool addIfNotPresent) {
	unordered_map<uint64_t, int>* intmap = getMap(type);

	int ret = 0;
	if (intmap->find(entry) == intmap->end()) {
		if (!growthStopped && addIfNotPresent) {
			ret = numEntries + 1;
			numEntries++;
			(*intmap)[entry] = ret;
		}
	}
	else {
		ret = (*intmap)[entry];
	}
	return ret;
}

int FeatureAlphabet::lookupIndex(unordered_map<uint64_t, int>& intmap, const uint64_t entry, bool addIfNotPresent) {
	int ret = 0;
	if (intmap.find(entry) == intmap.end()) {
		if (!growthStopped && addIfNotPresent) {
			ret = numEntries + 1;
			numEntries++;
			intmap[entry] = ret;
		}
	}
	else {
		ret = intmap[entry];
	}
	return ret;
}

int FeatureAlphabet::lookupIndex(const int type, const uint64_t entry) {
	return lookupIndex (type, entry, true);
}

int FeatureAlphabet::size() {
	return numEntries + 1;
}

void FeatureAlphabet::stopGrowth() {
	growthStopped = true;
}

void FeatureAlphabet::writeObject (FILE* fs) {
	CHECK(WriteInteger(fs, numEntries));
	CHECK(WriteUINT64IntegerMap(fs, arcMap));
	CHECK(WriteUINT64IntegerMap(fs, secondOrderMap));
	CHECK(WriteUINT64IntegerMap(fs, thirdOrderMap));
	CHECK(WriteUINT64IntegerMap(fs, highOrderMap));
	CHECK(WriteBool(fs, growthStopped));
}

void FeatureAlphabet::readObject (FILE* fs) {
	CHECK(ReadInteger(fs, &numEntries));
	CHECK(ReadUINT64IntegerMap(fs, &arcMap));
	CHECK(ReadUINT64IntegerMap(fs, &secondOrderMap));
	CHECK(ReadUINT64IntegerMap(fs, &thirdOrderMap));
	CHECK(ReadUINT64IntegerMap(fs, &highOrderMap));
	CHECK(ReadBool(fs, &growthStopped));
}

} /* namespace segparser */
