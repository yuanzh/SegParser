/*
 * StringUtils.cpp
 *
 *  Created on: Jan 21, 2014
 *      Author: yuanz
 */

#include <iostream>
#include "StringUtils.h"
#include <assert.h>

// Split string str on any delimiting character in delim, and write the result
// as a vector of strings.
void StringSplit(const string &str,
		const string &delim,
		vector<string> *results) {
	size_t cutAt;
	string tmp = str;
	size_t len = delim.size();

	while ((cutAt = tmp.find(delim)) != tmp.npos) {
		if(cutAt > 0) {
			results->push_back(tmp.substr(0,cutAt));
		}
		tmp = tmp.substr(cutAt+len);
	}
	if(tmp.length() > 0) results->push_back(tmp);
}

// Deletes any head in the string "line" after the first occurrence of any
// non-delimiting character (e.g. whitespaces).
void TrimLeft(const string &delim, string *line) {
	size_t cutAt = line->find_first_not_of(delim);
	if (cutAt == line->npos) {
		*line = "";
	} else {
		*line = line->substr(cutAt);
	}
}

// Deletes any tail in the string "line" after the last occurrence of any
// non-delimiting character (e.g. whitespaces).
void TrimRight(const string &delim, string *line) {
	size_t cutAt = line->find_last_not_of(delim);
	if (cutAt == line->npos) {
		*line = "";
	} else {
		*line = line->substr(0, cutAt+1);
	}
}

// Trims left and right (see above).
void Trim(const string &delim, string *line) {
	TrimLeft(delim, line);
	TrimRight(delim, line);
}

void ThrowException(const string& msg) {
	cerr << msg << endl;
	exit(-1);
}

int ChineseStringLength(const string& str) {
	int p = 0;
	int len = 0;
	while (str.find("ASC/", p) != string::npos) {
		len++;
		p = str.find("ASC/", p) + 4;
	}
	return len;
}

string GetChineseChar(const string& str, int k) {
	int p = 0;
	for (int i = 0; i < k; ++i) {
		assert(str.find("ASC/", p) != string::npos);
		p = str.find("ASC/", p) + 4;
	}

	int st = str.find("ASC/", p);
	int en = str.find("ASC/", st + 4);

	return str.substr(st, (en == (int)string::npos ? string::npos : en - st));
}


