/*
 * SerializationUtils.cpp
 *
 *  Created on: Jan 21, 2014
 *      Author: yuanz
 */

#include "SerializationUtils.h"
#include <cstring>

bool WriteString(FILE *fs, const std::string& data) {
	const char *buffer = data.c_str();
	unsigned int length = strlen(buffer);
	if (1 != fwrite(&length, sizeof(int), 1, fs)) return false;
	if (length != fwrite(buffer, sizeof(char), length, fs)) return false;
	return true;
}

bool WriteBool(FILE *fs, bool value) {
	if (1 != fwrite(&value, sizeof(bool), 1, fs)) return false;
	return true;
}

bool WriteInteger(FILE *fs, int value) {
	if (1 != fwrite(&value, sizeof(int), 1, fs)) return false;
	return true;
}

bool WriteUINT64(FILE *fs, uint64_t value) {
	if (1 != fwrite(&value, sizeof(uint64_t), 1, fs)) return false;
	return true;
}

bool WriteDouble(FILE *fs, double value) {
	if (1 != fwrite(&value, sizeof(double), 1, fs)) return false;
	return true;
}

bool WriteStringIntegerMap(FILE *fs, const std::unordered_map<std::string, int>& map) {
	if (1 != WriteInteger(fs, map.size()))
		return false;
	for (auto kv : map) {
		if (1 != WriteString(fs, kv.first))
			return false;
		if (1 != WriteInteger(fs, kv.second))
			return false;
	}
	return true;
}

bool WriteUINT64IntegerMap(FILE *fs, const std::unordered_map<uint64_t, int>& map) {
	if (1 != WriteInteger(fs, map.size()))
		return false;
	for (auto kv : map) {
		if (1 != WriteUINT64(fs, kv.first))
			return false;
		if (1 != WriteInteger(fs, kv.second))
			return false;
	}
	return true;
}

bool WriteDoubleArray(FILE *fs, const std::vector<double>& arr) {
	if (1 != WriteInteger(fs, arr.size()))
		return false;
	for (unsigned int i = 0; i < arr.size(); ++i) {
		if (1 != WriteDouble(fs, arr[i]))
			return false;
	}
	return true;
}

bool ReadString(FILE *fs, std::string *data) {
	unsigned int length;
	if (1 != fread(&length, sizeof(int), 1, fs)) return false;
	char *buffer = new char[length + 1];
	if (length != fread(buffer, sizeof(char), length, fs)) return false;
	buffer[length] = '\0';
	*data = buffer;
	delete[] buffer;
	return true;
}

bool ReadBool(FILE *fs, bool *value) {
	if (1 != fread(value, sizeof(bool), 1, fs)) return false;
	return true;
}

bool ReadInteger(FILE *fs, int *value) {
	if (1 != fread(value, sizeof(int), 1, fs)) return false;
	return true;
}

bool ReadUINT64(FILE *fs, uint64_t *value) {
	if (1 != fread(value, sizeof(uint64_t), 1, fs)) return false;
	return true;
}

bool ReadDouble(FILE *fs, double *value) {
	if (1 != fread(value, sizeof(double), 1, fs)) return false;
	return true;
}

bool ReadStringIntegerMap(FILE *fs, std::unordered_map<std::string, int>* map) {
	int size = 0;
	if (1 != ReadInteger(fs, &size))
		return false;
	for (int i = 0; i < size; ++i) {
		std::string key;
		int value;
		if (1 != ReadString(fs, &key))
			return false;
		if (1 != ReadInteger(fs, &value))
			return false;
		(*map)[key] = value;
	}
	return true;
}

bool ReadUINT64IntegerMap(FILE *fs, std::unordered_map<uint64_t, int>* map) {
	int size = 0;
	if (1 != ReadInteger(fs, &size))
		return false;
	for (int i = 0; i < size; ++i) {
		uint64_t key;
		int value;
		if (1 != ReadUINT64(fs, &key))
			return false;
		if (1 != ReadInteger(fs, &value))
			return false;
		(*map)[key] = value;
	}
	return true;
}

bool ReadDoubleArray(FILE* fs, std::vector<double>* arr) {
	int size = 0;
	if (1 != ReadInteger(fs, &size))
		return false;
	arr->clear();
	arr->reserve(size);
	for (int i = 0; i < size; ++i) {
		double value;
		if (1 != ReadDouble(fs, &value))
			return false;
		arr->push_back(value);
	}
	return true;
}


