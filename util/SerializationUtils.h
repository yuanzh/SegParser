/*
 * SerializationUtils.h
 *
 *  Created on: Jan 21, 2014
 *      Author: yuanz
 */

#ifndef SERIALIZATIONUTILS_H_
#define SERIALIZATIONUTILS_H_

#include <stdio.h>
#include <string>
#include <stdint.h>
#include <unordered_map>
#include <vector>
#include "StringUtils.h"

extern bool WriteString(FILE *fs, const std::string& data);
extern bool WriteBool(FILE *fs, bool value);
extern bool WriteInteger(FILE *fs, int value);
extern bool WriteUINT64(FILE *fs, uint64_t value);
extern bool WriteDouble(FILE *fs, double value);
extern bool WriteStringIntegerMap(FILE *fs, const std::unordered_map<std::string, int>& map);
extern bool WriteUINT64IntegerMap(FILE *fs, const std::unordered_map<uint64_t, int>& map);
extern bool WriteDoubleArray(FILE *fs, const std::vector<double>& arr);

extern bool ReadString(FILE *fs, std::string *data);
extern bool ReadBool(FILE *fs, bool *value);
extern bool ReadInteger(FILE *fs, int *value);
extern bool ReadUINT64(FILE *fs, uint64_t *value);
extern bool ReadDouble(FILE *fs, double *value);
extern bool ReadStringIntegerMap(FILE *fs, std::unordered_map<std::string, int>* map);
extern bool ReadUINT64IntegerMap(FILE *fs, std::unordered_map<uint64_t, int>* map);
extern bool ReadDoubleArray(FILE *fs, std::vector<double>* arr);

#define CHECK(x) { if (!x) ThrowException("check bug"); }

#endif /* SERIALIZATIONUTILS_H_ */
