/*
 * Constant.h
 *
 *  Created on: Mar 27, 2014
 *      Author: yuanz
 */

#ifndef CONSTANT_H_
#define CONSTANT_H_

#include <string>

namespace segparser {

class DependencyInstance;

using namespace std;

struct DecodingMode {
	enum types {
		SampleRank = 0,
		HillClimb,
		Exact,
	};
};

struct PossibleLang {
	enum types {
		Arabic = 0,
		SPMRL,
		Chinese,
		Count,
	};

	static const string langString[PossibleLang::Count];
};

struct SpecialPos {
	enum types {
		C = 0, P, PNX, V, AJ, N, OTHER, COUNT,
	};
};

struct ConstPosLex {
	enum types {
		UNSEEN = 0,
		START,
		MID,
		END,
		QUOTE,
		LRB,
		RRB,
	};
};

struct ConstLab {
	enum types {
		UNSEEN = 0,
		NOTYPE,
	};
};

#define BINNED_BUCKET 8
#define MAX_CHILD_NUM 5
#define MAX_SPAN_LENGTH 5
#define MAX_LEN_DIFF 4
#define MAX_FEATURE_NUM 7

} /* namespace segparser */
#endif /* CONSTANT_H_ */
