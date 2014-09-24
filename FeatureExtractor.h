/*
 * CacheTable.h
 *
 *  Created on: Apr 2, 2014
 *      Author: yuanz
 */

#ifndef FEATUREEXTRACTOR_H_
#define FEATUREEXTRACTOR_H_

#include <unordered_map>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "util/FeatureVector.h"
#include "DependencyInstance.h"
#include "SegParser.h"
#include "Options.h"
#include "DependencyPipe.h"
#include "Parameters.h"

namespace segparser {

using namespace std;

class SegParser;
class Parameters;

class CacheItem {
public:
	FeatureVector fv;
	double score;
	int flag;

	CacheItem() {
		score = 0.0;
		flag = 123;
	}
};

typedef boost::shared_ptr<CacheItem> item_ptr;

/***
 * CacheTable always uses segIndex while FeatureExtractor always uses word/seg Index.
 * DependencyInstance is responsible for the conversion
 */

class PrunerFeatureExtractor;

class CacheTable {
public:
	CacheTable();
	virtual ~CacheTable();

	void initCacheTable(int _type, DependencyInstance* inst, PrunerFeatureExtractor* pfe, Options* options);

	bool isPruned(int h, int m);
	int arc2ID(int h, int m);

	int numSeg;			// length based on seg
	int numWord;
	int type;

	int nuparcs;						// number of un-pruned arcs, include gold

	vector<item_ptr> arc;		// first order cache [h][m]
	vector<item_ptr> trips;		// second order [dep id][sib]
	vector<item_ptr> sibs;		// [mod][sib][2]
	vector<item_ptr> gpc;		// [dep id][child]
	vector<item_ptr> posho;		// pos feature [hid]

private:
	vector<int> arc2id;					// map (h->m) arc to an id in [0, nuparcs-1]
	vector<bool> pruned;				// whether a (h->m) arc is pruned, not necessarily include gold
};

class FeatureExtractor {
public:
	FeatureExtractor();
	FeatureExtractor(DependencyInstance* inst, SegParser* parser, Parameters* params, int thread);
	virtual ~FeatureExtractor();

	CacheTable* getCacheTable(DependencyInstance* s);

	double getPartialDepScore(DependencyInstance* s, HeadIndex& x, CacheTable* cache);
	double getPartialBigramDepScore(DependencyInstance* s, HeadIndex& x, HeadIndex& y, CacheTable* cache);
	double getPartialPosScore(DependencyInstance* s, HeadIndex& x, CacheTable* cache);
	double getScore(DependencyInstance* s);
	double getScore(DependencyInstance* s, CacheTable* cache);
	void getPartialFv(DependencyInstance* s, HeadIndex& x, FeatureVector* fv);
	void getFv(DependencyInstance* s, FeatureVector* fv);

	vector<bool> isPruned(DependencyInstance* s, HeadIndex& m, CacheTable* cache);

	int numWord;
	int type;
	int thread;

	//DependencyInstance* inst;		so risky to add this variable in multi-thread scenario. Other variables are read-only
	DependencyPipe* pipe;
	Parameters* parameters;
	SegParser* pruner;
	boost::shared_ptr<PrunerFeatureExtractor> pfe;

	void (*getArcFv)(FeatureExtractor*, DependencyInstance*, HeadIndex&, HeadIndex&, FeatureVector*, CacheTable*);
	double (*getArcScore)(FeatureExtractor*, DependencyInstance*, HeadIndex&, HeadIndex&, CacheTable*);

	void (*getSibsFv)(FeatureExtractor*, DependencyInstance*, HeadIndex&, HeadIndex&, bool, FeatureVector*, CacheTable*);
	double (*getSibsScore)(FeatureExtractor*, DependencyInstance*, HeadIndex&, HeadIndex&, bool, CacheTable*);

	void (*getTripsFv)(FeatureExtractor*, DependencyInstance*, HeadIndex&, HeadIndex&, HeadIndex&, FeatureVector*, CacheTable*);
	double (*getTripsScore)(FeatureExtractor*, DependencyInstance*, HeadIndex&, HeadIndex&, HeadIndex&, CacheTable*);

	void (*getGPCFv)(FeatureExtractor*, DependencyInstance*, HeadIndex&, HeadIndex&, HeadIndex&, FeatureVector*, CacheTable*);
	double (*getGPCScore)(FeatureExtractor*, DependencyInstance*, HeadIndex&, HeadIndex&, HeadIndex&, CacheTable*);

	void (*getPosHOFv)(FeatureExtractor*, DependencyInstance*, HeadIndex&, FeatureVector*, CacheTable*);
	double (*getPosHOScore)(FeatureExtractor*, DependencyInstance*, HeadIndex&, CacheTable*);

	// pre-computed
	void getPos1OFv(DependencyInstance* inst, HeadIndex& m, FeatureVector* fv);
	double getPos1OScore(DependencyInstance* inst, HeadIndex& m);
	void getSegFv(DependencyInstance* inst, int wordid, FeatureVector* fv);
	double getSegScore(DependencyInstance* inst, int worid);

	vector<CacheTable> optSegCacheMap;		// cache for optimal seg for every word with different POS
	vector<CacheTable> subOptSegCacheMap;	// cache for sub-optimal seg for one word with optimal POS

	// cache not related to seg/pos choices
	vector<item_ptr> seg1o;		// seg feature [wordid]
	vector<item_ptr> pos1o;		// pos feature [segid]

protected:
	void constructCacheMap(DependencyInstance* s);
	void initCacheMap(DependencyInstance* s);

	// feature functions and pointers
	static void getArcFvUnsafe(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& h, HeadIndex& m, FeatureVector* fv, CacheTable* cache);
	static void getArcFvAtomic(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& h, HeadIndex& m, FeatureVector* fv, CacheTable* cache);
	static double getArcScoreUnsafe(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& h, HeadIndex& m, CacheTable* cache);
	static double getArcScoreAtomic(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& h, HeadIndex& m, CacheTable* cache);

	static void getSibsFvUnsafe(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& ch1, HeadIndex& ch2, bool isSt, FeatureVector* fv, CacheTable* cache);
	static void getSibsFvAtomic(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& ch1, HeadIndex& ch2, bool isSt, FeatureVector* fv, CacheTable* cache);
	static double getSibsScoreUnsafe(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& ch1, HeadIndex& ch2, bool isSt, CacheTable* cache);
	static double getSibsScoreAtomic(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& ch1, HeadIndex& ch2, bool isSt, CacheTable* cache);

	static void getTripsFvUnsafe(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& par, HeadIndex& ch1, HeadIndex& ch2, FeatureVector* fv, CacheTable* cache);
	static void getTripsFvAtomic(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& par, HeadIndex& ch1, HeadIndex& ch2, FeatureVector* fv, CacheTable* cache);
	static double getTripsScoreUnsafe(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& par, HeadIndex& ch1, HeadIndex& ch2, CacheTable* cache);
	static double getTripsScoreAtomic(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& par, HeadIndex& ch1, HeadIndex& ch2, CacheTable* cache);

	static void getGPCFvUnsafe(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& gp, HeadIndex& par, HeadIndex& c, FeatureVector* fv, CacheTable* cache);
	static void getGPCFvAtomic(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& gp, HeadIndex& par, HeadIndex& c, FeatureVector* fv, CacheTable* cache);
	static double getGPCScoreUnsafe(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& gp, HeadIndex& par, HeadIndex& c, CacheTable* cache);
	static double getGPCScoreAtomic(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& gp, HeadIndex& par, HeadIndex& c, CacheTable* cache);

	static void getPosHOFvUnsafe(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& m, FeatureVector* fv, CacheTable* cache);
	static void getPosHOFvAtomic(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& m, FeatureVector* fv, CacheTable* cache);
	static double getPosHOScoreUnsafe(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& m, CacheTable* cache);
	static double getPosHOScoreAtomic(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& m, CacheTable* cache);

	void setAtomic(int thread);
	bool atomic;							// whether the load/store need atomic operation

	// cache map
	vector<int> optSegCacheStPos;			// start position in the cache map for each seg

	vector<int> subOptSegCacheStPos;		// start position in the cache map for each word

	vector<int> seg1oStPos;		// [word]->segcand
	vector<int> pos1oStPos2d;	// [word][segcand]->segid
	vector<int> pos1oStPos3d;	// [word][segcand][segid]->poscand

	int getSeg1OCachePos(int wordid, int segCandID);
	int getPos1OCachePos(int wordid, int segCandID, int segid, int posCandID);

	// others
	Options* options;
};

class PrunerFeatureExtractor : public segparser::FeatureExtractor {
public:
	CacheTable prunerCache;

	PrunerFeatureExtractor();
	void init(DependencyInstance* inst, SegParser* pruner, int thread);
	void prune(DependencyInstance* inst, HeadIndex& m, vector<bool>& pruned);
};

} /* namespace segparser */
#endif /* FEATUREEXTRACTOR_H_ */
