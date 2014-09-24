/*
 * StringUtils.h
 *
 *  Created on: Jan 21, 2014
 *      Author: yuanz
 */

#ifndef STRINGUTILS_H_
#define STRINGUTILS_H_

#include <string>
#include <vector>

using namespace std;

extern void StringSplit(const string &str,
                        const string &delim,
                        vector<string> *results);

extern void TrimLeft(const string &delim, string *line);

extern void TrimRight(const string &delim, string *line);

extern void Trim(const string &delim, string *line);

extern void ThrowException(const string& msg);

extern int ChineseStringLength(const string& str);

extern string GetChineseChar(const string& str, int k);

#endif /* STRINGUTILS_H_ */
