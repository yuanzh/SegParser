/*
 * CacheTable.cpp
 *
 *  Created on: Apr 2, 2014
 *      Author: yuanz
 */

#include "FeatureExtractor.h"
#include "assert.h"
#include "util/StringUtils.h"
#include <float.h>
#include <algorithm>
#include <functional>
#include <array>

namespace segparser {

CacheTable::CacheTable() {
}

CacheTable::~CacheTable() {
}

void CacheTable::initCacheTable(int _type, DependencyInstance* inst, PrunerFeatureExtractor* pfe, Options* options) {

	type = _type;
	numSeg = inst->getNumSeg();
	numWord = inst->numWord;

	arc2id.resize(numSeg * numSeg);
	pruned.resize(numSeg * numSeg);

	int size = arc2id.size();
	for (int i = 0; i < size; ++i) {
		arc2id[i] = -1;
		pruned[i] = true;
	}

	if (pfe) {
		//ThrowException("not implemented yet");
		size = 0;
		for (int mw = 1; mw < inst->numWord; ++mw) {
			SegInstance& segInst = inst->word[mw].getCurrSeg();

			for (int ms = 0; ms < segInst.size(); ++ms) {
				int modIndex = inst->wordToSeg(mw, ms);

				HeadIndex m(mw, ms);
				vector<bool> tmpPruned;
				pfe->prune(inst, m, tmpPruned);

				int p = 0;
				for (int hw = 0; hw < inst->numWord; ++hw) {
					SegInstance& headSeg = inst->word[hw].getCurrSeg();
					for (int hs = 0; hs < headSeg.size(); ++hs) {
						if (hw == m.hWord && hs == m.hSeg)
							continue;

						int headIndex = inst->wordToSeg(hw, hs);
						int pos = headIndex * numSeg + modIndex;
						if (!tmpPruned[p]) {
							pruned[pos] = false;
							arc2id[pos] = size;
							size++;
						}
						p++;
					}
				}
				assert(p == (int)tmpPruned.size());
			}
		}
	}
	else {
		size = 0;
		for (int mw = 1; mw < inst->numWord; ++mw) {
			SegInstance& segInst = inst->word[mw].getCurrSeg();

			for (int ms = 0; ms < segInst.size(); ++ms) {
				int modIndex = inst->wordToSeg(mw, ms);

				HeadIndex m(mw, ms);
				for (int hw = 0; hw < inst->numWord; ++hw) {
					SegInstance& headSeg = inst->word[hw].getCurrSeg();
					for (int hs = 0; hs < headSeg.size(); ++hs) {
						if (hw == m.hWord && hs == m.hSeg)
							continue;

						int headIndex = inst->wordToSeg(hw, hs);
						int pos = headIndex * numSeg + modIndex;
						pruned[pos] = false;
						arc2id[pos] = size;
						size++;
					}
				}
			}
		}
	}

	nuparcs = size;
	arc.resize(size);
	//if (pfe && numSeg > 100)
	//	cout << "ratio: " << (double) size / (numSeg * (numSeg - 1)) << endl;
	if (options->useCS) {
		sibs.resize(numSeg * numSeg * 2);
		trips.resize(size * numSeg);
	}
	if (options->useGP) {
		gpc.resize(size * numSeg);
	}
	if (options->useSP) {
		posho.resize(numSeg);
	}
}

bool CacheTable::isPruned(int h, int m)
{
	return pruned[h * numSeg + m];
}

int CacheTable::arc2ID(int h, int m)
{
	return arc2id[h * numSeg + m];
}

PrunerFeatureExtractor::PrunerFeatureExtractor() {
}

void PrunerFeatureExtractor::init(DependencyInstance* inst, SegParser* pruner, int thread) {
	this->thread = thread;
	pipe = pruner->pipe;
	parameters = pruner->parameters;
	options = pruner->options;
	this->pruner = NULL;

	numWord = inst->numWord;
	type = pipe->typeAlphabet->size();

	setAtomic(thread);

	prunerCache.initCacheTable(type, inst, NULL, options);
}


void PrunerFeatureExtractor::prune(DependencyInstance* inst, HeadIndex& m, vector<bool>& pruned) {
	// no cache here because cache is pre-computed

	vector<double> score;
	double maxScore = -DBL_MAX;
	SegElement& ele = inst->getElement(m);
	HeadIndex oldDep = ele.dep;
	for (int hw = 0; hw < inst->numWord; ++hw) {
		SegInstance& headSeg = inst->word[hw].getCurrSeg();
		for (int hs = 0; hs < headSeg.size(); ++hs) {
			if (hw == m.hWord && hs == m.hSeg)
				continue;

			HeadIndex h(hw, hs);
			ele.dep = h;
			double s = getArcScore(this, inst, h, m, NULL);
			score.push_back(s);
			if (s > maxScore + 1e-6) {
				maxScore = s;
			}
		}
	}

	vector<double> scoreSort;
	for (unsigned int i = 0; i < score.size(); ++i) {
		if (score[i] > maxScore + log(options->pruneThresh)) {
			scoreSort.push_back(score[i]);
		}
	}

	double thresh = maxScore + log(options->pruneThresh);
	if ((int)scoreSort.size() > options->maxHead) {
		// too many candidate head
		partial_sort(scoreSort.begin(), scoreSort.begin() + options->maxHead, scoreSort.end(), std::greater<double>());
		assert(scoreSort[0] > scoreSort[options->maxHead - 1] - 1e-6);
		thresh = scoreSort[options->maxHead - 1] - 1e-6;
	}

	for (unsigned int i = 0; i < score.size(); ++i) {
		if (score[i] > thresh) {
			pruned.push_back(false);
		}
		else {
			pruned.push_back(true);
		}
	}
	ele.dep = oldDep;
}

FeatureExtractor::FeatureExtractor() {
}

FeatureExtractor::FeatureExtractor(DependencyInstance* inst, SegParser* parser, Parameters* params, int thread)
	: thread(thread), pipe(parser->pipe), parameters(params), pruner(parser->pruner), options(parser->options){
	numWord = inst->numWord;
	type = pipe->typeAlphabet->size();

	constructCacheMap(inst);
	initCacheMap(inst);
	setAtomic(thread);

	if (pruner) {
		pfe = boost::shared_ptr<PrunerFeatureExtractor>(new PrunerFeatureExtractor());
		pfe->init(inst, pruner, thread);
	}
}

FeatureExtractor::~FeatureExtractor() {
}

void FeatureExtractor::constructCacheMap(DependencyInstance* s) {
	// for optimal seg
	int size = 1;	// optimal seg and pos
	optSegCacheStPos.push_back(size);
	for (int i = 0; i < s->numWord; ++i) {
		SegInstance& segInst = s->word[i].candSeg[0];	// optimal seg
		for (int j = 0; j < segInst.size(); ++j) {
			size += segInst.element[j].candPos.size() - 1;
			optSegCacheStPos.push_back(size);
		}
	}
	optSegCacheMap.resize(size);

	// for sub-optimal seg
	size = 0;
	subOptSegCacheStPos.push_back(0);
	for (int i = 0; i < s->numWord; ++i) {
		size += s->word[i].candSeg.size() - 1;
		subOptSegCacheStPos.push_back(size);
	}
	subOptSegCacheMap.resize(size);

	// for pos1o and seg1o
	int size1d = 0;
	int size2d = 0;
	int size3d = 0;
	seg1oStPos.push_back(size1d);
	pos1oStPos2d.push_back(size2d);
	pos1oStPos3d.push_back(size3d);
	for (int i = 0; i < s->numWord; ++i) {
		size1d += s->word[i].candSeg.size();
		seg1oStPos.push_back(size1d);

		for (unsigned int j = 0; j < s->word[i].candSeg.size(); ++j) {
			SegInstance& segInst = s->word[i].candSeg[j];

			size2d += segInst.size();
			pos1oStPos2d.push_back(size2d);

			for (int k = 0; k < segInst.size(); ++k) {
				size3d += segInst.element[k].candPosNum();
				pos1oStPos3d.push_back(size3d);
			}
		}
	}
	seg1o.resize(size1d);
	pos1o.resize(size3d);
}

void FeatureExtractor::initCacheMap(DependencyInstance* s) {
	// need to recover, so copy variable info
	VariableInfo origVar(s);

	// for optimal seg/pos
	for (int i = 0; i < s->numWord; ++i) {
		s->word[i].currSegCandID = 0;
		SegInstance& segInst = s->word[i].getCurrSeg();
		for (int j = 0; j < segInst.size(); ++j) {
			segInst.element[j].currPosCandID = 0;
		}
	}
	s->constructConversionList();
	CacheTable& cache = optSegCacheMap[0];
	cache.initCacheTable(type, s, pfe.get(), options);

	// optimal seg, sub-optimal pos
	int id = 1;
	for (int i = 0; i < s->numWord; ++i) {
		WordInstance& word = s->word[i];
		SegInstance& segInst = word.getCurrSeg();

		for (int j = 0; j < segInst.size(); ++j) {
			for (int k = 1; k < segInst.element[j].candPosNum(); ++k) {
				segInst.element[j].currPosCandID = k;
				// don't need to re-construct conversion list
				CacheTable& cache = optSegCacheMap[id];
				cache.initCacheTable(type, s, pfe.get(), options);
				id++;
			}
			segInst.element[j].currPosCandID = 0;
		}
	}

	// sub-optimal seg, optimal pos
	id = 0;
	for (int i = 0; i < s->numWord; ++i) {
		WordInstance& word = s->word[i];

		for (unsigned int j = 1; j < word.candSeg.size(); ++j) {
			word.currSegCandID = j;
			SegInstance& segInst = word.getCurrSeg();
			for (int k = 0; k < segInst.size(); ++k)
				segInst.element[k].currPosCandID = 0;

			CacheTable& cache = subOptSegCacheMap[id];
			s->constructConversionList();
			cache.initCacheTable(type, s, pfe.get(), options);

			id++;
		}
		word.currSegCandID = 0;
		SegInstance& segInst = word.getCurrSeg();
		for (int k = 0; k < segInst.size(); ++k)
			segInst.element[k].currPosCandID = 0;
	}

	// build 1o seg/pos feature
	int segid = 0;
	int posid = 0;
	for (int i = 0; i < s->numWord; ++i) {
		WordInstance& word = s->word[i];

		for (unsigned int j = 0; j < word.candSeg.size(); ++j) {
			word.currSegCandID = j;
			assert(segid == getSeg1OCachePos(i, j));
			item_ptr tmp_ptr = item_ptr(new CacheItem());
			pipe->createSegFeatureVector(s, i, &tmp_ptr->fv);
			tmp_ptr->score = parameters->getScore(&tmp_ptr->fv);
			seg1o[segid] = tmp_ptr;
			segid++;

			SegInstance& segInst = word.getCurrSeg();
			for (int k = 0; k < segInst.size(); ++k) {
				SegElement& ele = segInst.element[k];

				for (int l = 0; l < ele.candPosNum(); ++l) {
					ele.currPosCandID = l;
					assert(posid == getPos1OCachePos(i, j, k, l));
					HeadIndex m(i, k);
					item_ptr tmp_ptr = item_ptr(new CacheItem());
					pipe->createPos1OFeatureVector(s, m, &tmp_ptr->fv);
					tmp_ptr->score = parameters->getScore(&tmp_ptr->fv);
					pos1o[posid] = tmp_ptr;

					posid++;
				}
			}
		}
	}
	assert(segid == (int)seg1o.size());
	assert(posid == (int)pos1o.size());

	// recover
   	origVar.loadInfoToInst(s);
   	s->constructConversionList();
   	s->setOptSegPosCount();
   	//s->buildChild();
}

CacheTable* FeatureExtractor::getCacheTable(DependencyInstance* s) {
	if (s->optSegCount == s->numWord) {
		int totalSeg = 0;			// total number of segments in the sentence
		int totalOptPos = 0;		// number of segs with optimal pos
		int subOptSeg = -1;
		int subOptPos = -1;

		for (int i = 0; i < s->numWord; ++i) {
			WordInstance& word = s->word[i];
			SegInstance& segInst = word.getCurrSeg();

			if (word.optPosCount < segInst.size()) {
				for (int j = 0; j < segInst.size(); ++j)
					if (!segInst.element[j].isOptPos()) {
						subOptSeg = totalSeg + j;
						subOptPos = segInst.element[j].currPosCandID;
						break;
					}
			}

			totalSeg += segInst.size();
			totalOptPos += word.optPosCount;
		}

		assert(totalSeg == s->getNumSeg());

		if (totalSeg == totalOptPos) {
			return &optSegCacheMap[0];
		}
		else if (totalOptPos == totalSeg - 1) {
			int pos = optSegCacheStPos[subOptSeg] + subOptPos - 1;
			return &optSegCacheMap[pos];
		}
		else {
			return NULL;
		}
	}
	else if (s->optSegCount == s->numWord - 1) {
		int subOptWord = 0;
		for (;subOptWord < s->numWord; ++subOptWord)
			if (!s->word[subOptWord].isOptSeg())
				break;
		assert(subOptWord < s->numWord);

		int totalSeg = 0;			// total number of segments in the sentence
		int totalOptPos = 0;		// number of segs with optimal pos

		for (int i = 0; i < s->numWord; ++i) {
			WordInstance& word = s->word[i];
			SegInstance& segInst = word.getCurrSeg();
			totalSeg += segInst.size();
			totalOptPos += word.optPosCount;
		}

		assert(totalSeg == s->getNumSeg());

		if (totalSeg == totalOptPos) {
			// all pos are optimal
			int pos = subOptSegCacheStPos[subOptWord] + s->word[subOptWord].currSegCandID - 1;
			return &subOptSegCacheMap[pos];
		}
		else {
			return NULL;
		}
	}
	return NULL;
}

int FeatureExtractor::getSeg1OCachePos(int wordid, int segCandID) {
	return seg1oStPos[wordid] + segCandID;
}

int FeatureExtractor::getPos1OCachePos(int wordid, int segCandID, int segid, int posCandID) {
	int d1 = seg1oStPos[wordid] + segCandID;
	int d2 = pos1oStPos2d[d1] + segid;
	int d3 = pos1oStPos3d[d2] + posCandID;
	//return pos1oStPos3d[pos1oStPos2d[seg1oStPos[wordid] + segCandID] + segid] + posCandID;
	return d3;
}

void FeatureExtractor::setAtomic(int thread) {
	atomic = thread != 1;

	getArcFv = atomic ? &getArcFvAtomic : &getArcFvUnsafe;
	getArcScore = atomic ? &getArcScoreAtomic : &getArcScoreUnsafe;
	getSibsFv = atomic ? &getSibsFvAtomic : &getSibsFvUnsafe;
	getSibsScore = atomic ? &getSibsScoreAtomic : &getSibsScoreUnsafe;
	getTripsFv = atomic ? &getTripsFvAtomic : &getTripsFvUnsafe;
	getTripsScore = atomic ? &getTripsScoreAtomic : &getTripsScoreUnsafe;
	getGPCFv = atomic ? &getGPCFvAtomic : &getGPCFvUnsafe;
	getGPCScore = atomic ? &getGPCScoreAtomic : &getGPCScoreUnsafe;
	getPosHOFv = atomic ? &getPosHOFvAtomic : &getPosHOFvUnsafe;
	getPosHOScore = atomic ? &getPosHOScoreAtomic : &getPosHOScoreUnsafe;
}

//-------------------------------------------

void FeatureExtractor::getArcFvUnsafe(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& h, HeadIndex& m,
		FeatureVector* fv, CacheTable* cache) {
	assert(fe->thread == 1);
	int id = -1;
	if (cache) {
		int headIndex = inst->wordToSeg(h);
		int modIndex = inst->wordToSeg(m);

		id = cache->arc2ID(headIndex, modIndex);
	}
	if (id >= 0) {
		//assert(id >= 0);
		int pos = id;
		assert(pos < (int)cache->arc.size());
		if (!cache->arc[pos]) {
			item_ptr tmp_ptr = item_ptr(new CacheItem());
			fe->pipe->createArcFeatureVector(inst, h, m, &tmp_ptr->fv);
			tmp_ptr->score = fe->parameters->getScore(&tmp_ptr->fv);
			cache->arc[pos] = tmp_ptr;
		}
		if (fv)
			fv->concat(&cache->arc[pos]->fv);
	}
	else {
		if (fv)
			fe->pipe->createArcFeatureVector(inst, h, m, fv);
	}
}

void FeatureExtractor::getArcFvAtomic(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& h, HeadIndex& m,
		FeatureVector* fv, CacheTable* cache) {
	assert(fe->thread != 1);
	int id = -1;
	if (cache) {
		int headIndex = inst->wordToSeg(h);
		int modIndex = inst->wordToSeg(m);

		id = cache->arc2ID(headIndex, modIndex);
	}
	if (id >= 0) {
		//assert(id >= 0);

		int pos = id;
		item_ptr tmp_ptr = atomic_load(&cache->arc[pos]);
		if (!tmp_ptr) {
			tmp_ptr = item_ptr(new CacheItem());
			fe->pipe->createArcFeatureVector(inst, h, m, &tmp_ptr->fv);
			tmp_ptr->score = fe->parameters->getScore(&tmp_ptr->fv);
			atomic_store(&cache->arc[pos], tmp_ptr);
		}
		if (fv) {
			tmp_ptr = atomic_load(&(cache->arc[pos]));
			fv->concat(&tmp_ptr->fv);
		}
	}
	else {
		if (fv)
			fe->pipe->createArcFeatureVector(inst, h, m, fv);
	}
}

double FeatureExtractor::getArcScoreUnsafe(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& h, HeadIndex& m, CacheTable* cache) {
	assert(fe->thread == 1);
	double score = 0.0;
	int id = -1;
	if (cache) {
		int headIndex = inst->wordToSeg(h);
		int modIndex = inst->wordToSeg(m);

		id = cache->arc2ID(headIndex, modIndex);
	}
	if (id >= 0) {
		//assert(id >= 0);

		int pos = id;
		if (!cache->arc[pos]) {
			getArcFvUnsafe(fe, inst, h, m, NULL, cache);
		}
		score += cache->arc[pos]->score;
	}
	else {
		FeatureVector fv;
		fe->pipe->createArcFeatureVector(inst, h, m, &fv);
		score = fe->parameters->getScore(&fv);
	}
	return score;
}

double FeatureExtractor::getArcScoreAtomic(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& h, HeadIndex& m, CacheTable* cache) {
	assert(fe->thread != 1);
	double score = 0.0;
	int id = -1;
	if (cache) {
		int headIndex = inst->wordToSeg(h);
		int modIndex = inst->wordToSeg(m);

		id = cache->arc2ID(headIndex, modIndex);
	}
	if (id >= 0) {
		//assert(id >= 0);

		int pos = id;
		item_ptr tmp_ptr = atomic_load(&cache->arc[pos]);
		if (!tmp_ptr) {
			getArcFvAtomic(fe, inst, h, m, NULL, cache);
		}
		tmp_ptr = atomic_load(&(cache->arc[pos]));
		score += tmp_ptr->score;
	}
	else {
		FeatureVector fv;
		fe->pipe->createArcFeatureVector(inst, h, m, &fv);
		score = fe->parameters->getScore(&fv);
	}
	return score;
}

//-------------------------------------------

void FeatureExtractor::getSibsFvUnsafe(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& ch1, HeadIndex& ch2, bool isSt,
		FeatureVector* fv, CacheTable* cache) {
	assert(fe->thread == 1);
	if (cache) {
		int ch1Idx = inst->wordToSeg(ch1);
		int ch2Idx = inst->wordToSeg(ch2);

		int pos = (ch1Idx * cache->numSeg + ch2Idx) * 2 + isSt;
		assert(pos < (int)cache->sibs.size());
		if (!cache->sibs[pos]) {
			item_ptr tmp_ptr = item_ptr(new CacheItem());
			fe->pipe->createSibsFeatureVector(inst, ch1, ch2, isSt, &tmp_ptr->fv);
			tmp_ptr->score = fe->parameters->getScore(&tmp_ptr->fv);
			cache->sibs[pos] = tmp_ptr;
		}
		if (fv)
			fv->concat(&cache->sibs[pos]->fv);
	}
	else {
		if (fv)
			fe->pipe->createSibsFeatureVector(inst, ch1, ch2, isSt, fv);
	}
}

void FeatureExtractor::getSibsFvAtomic(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& ch1, HeadIndex& ch2, bool isSt,
		FeatureVector* fv, CacheTable* cache) {
	assert(fe->thread != 1);
	if (cache) {
		int ch1Idx = inst->wordToSeg(ch1);
		int ch2Idx = inst->wordToSeg(ch2);

		int pos = (ch1Idx * cache->numSeg + ch2Idx) * 2 + isSt;
		assert(pos < (int)cache->sibs.size());
		item_ptr tmp_ptr = atomic_load(&cache->sibs[pos]);
		if (!tmp_ptr) {
			tmp_ptr = item_ptr(new CacheItem());
			fe->pipe->createSibsFeatureVector(inst, ch1, ch2, isSt, &tmp_ptr->fv);
			tmp_ptr->score = fe->parameters->getScore(&tmp_ptr->fv);
			atomic_store(&cache->sibs[pos], tmp_ptr);
		}
		if (fv) {
			tmp_ptr = atomic_load(&(cache->sibs[pos]));
			fv->concat(&tmp_ptr->fv);
		}
	}
	else {
		if (fv)
			fe->pipe->createSibsFeatureVector(inst, ch1, ch2, isSt, fv);
	}
}

double FeatureExtractor::getSibsScoreUnsafe(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& ch1, HeadIndex& ch2, bool isSt, CacheTable* cache) {
	assert(fe->thread == 1);
	double score = 0.0;
	if (cache) {
		int ch1Idx = inst->wordToSeg(ch1);
		int ch2Idx = inst->wordToSeg(ch2);

		int pos = (ch1Idx * cache->numSeg + ch2Idx) * 2 + isSt;
		assert(pos < (int)cache->sibs.size());
		if (!cache->sibs[pos]) {
			getSibsFvUnsafe(fe, inst, ch1, ch2, isSt, NULL, cache);
		}
		score += cache->sibs[pos]->score;
	}
	else {
		FeatureVector fv;
		fe->pipe->createSibsFeatureVector(inst, ch1, ch2, isSt, &fv);
		score = fe->parameters->getScore(&fv);
	}
	return score;
}

double FeatureExtractor::getSibsScoreAtomic(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& ch1, HeadIndex& ch2, bool isSt, CacheTable* cache) {
	assert(fe->thread != 1);
	double score = 0.0;
	if (cache) {
		int ch1Idx = inst->wordToSeg(ch1);
		int ch2Idx = inst->wordToSeg(ch2);

		int pos = (ch1Idx * cache->numSeg + ch2Idx) * 2 + isSt;
		assert(pos < (int)cache->sibs.size());
		item_ptr tmp_ptr = atomic_load(&cache->sibs[pos]);
		if (!tmp_ptr) {
			getSibsFvAtomic(fe, inst, ch1, ch2, isSt, NULL, cache);
		}
		tmp_ptr = atomic_load(&(cache->sibs[pos]));
		score += tmp_ptr->score;
	}
	else {
		FeatureVector fv;
		fe->pipe->createSibsFeatureVector(inst, ch1, ch2, isSt, &fv);
		score = fe->parameters->getScore(&fv);
	}
	return score;
}

//-------------------------------------------

void FeatureExtractor::getTripsFvUnsafe(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& par, HeadIndex& ch1, HeadIndex& ch2,
		FeatureVector* fv, CacheTable* cache) {
	assert(fe->thread == 1);
	int id = -1;
	if (cache) {
		int parIdx = inst->wordToSeg(par);
		int ch2Idx = inst->wordToSeg(ch2);

		id = cache->arc2ID(parIdx, ch2Idx);
	}
	if (id >= 0) {
		//assert(id >= 0);
		int ch1Idx = inst->wordToSeg(ch1);

		int pos = id * cache->numSeg + ch1Idx;
		assert(pos < (int)cache->trips.size());
		if (!cache->trips[pos]) {
			item_ptr tmp_ptr = item_ptr(new CacheItem());
			fe->pipe->createTripsFeatureVector(inst, par, ch1, ch2, &tmp_ptr->fv);
			tmp_ptr->score = fe->parameters->getScore(&tmp_ptr->fv);
			cache->trips[pos] = tmp_ptr;
		}
		if (fv)
			fv->concat(&cache->trips[pos]->fv);
	}
	else {
		if (fv)
			fe->pipe->createTripsFeatureVector(inst, par, ch1, ch2, fv);
	}
}

void FeatureExtractor::getTripsFvAtomic(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& par, HeadIndex& ch1, HeadIndex& ch2,
		FeatureVector* fv, CacheTable* cache) {
	assert(fe->thread != 1);
	int id = -1;
	if (cache) {
		int parIdx = inst->wordToSeg(par);
		int ch2Idx = inst->wordToSeg(ch2);

		id = cache->arc2ID(parIdx, ch2Idx);
	}
	if (id >= 0) {
		//assert(id >= 0);
		int ch1Idx = inst->wordToSeg(ch1);

		int pos = id * cache->numSeg + ch1Idx;
		assert(pos < (int)cache->trips.size());
		item_ptr tmp_ptr = atomic_load(&cache->trips[pos]);
		if (!tmp_ptr) {
			tmp_ptr = item_ptr(new CacheItem());
			fe->pipe->createTripsFeatureVector(inst, par, ch1, ch2, &tmp_ptr->fv);
			tmp_ptr->score = fe->parameters->getScore(&tmp_ptr->fv);
			atomic_store(&cache->trips[pos], tmp_ptr);
		}
		if (fv) {
			tmp_ptr = atomic_load(&(cache->trips[pos]));
			fv->concat(&tmp_ptr->fv);
		}
	}
	else {
		if (fv)
			fe->pipe->createTripsFeatureVector(inst, par, ch1, ch2, fv);
	}
}

double FeatureExtractor::getTripsScoreUnsafe(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& par, HeadIndex& ch1, HeadIndex& ch2, CacheTable* cache) {
	assert(fe->thread == 1);
	double score = 0.0;
	int id = -1;
	if (cache) {
		int parIdx = inst->wordToSeg(par);
		int ch2Idx = inst->wordToSeg(ch2);

		id = cache->arc2ID(parIdx, ch2Idx);
	}
	if (id >= 0) {
		//assert(id >= 0);
		int ch1Idx = inst->wordToSeg(ch1);

		int pos = id * cache->numSeg + ch1Idx;
		assert(pos < (int)cache->trips.size());
		if (!cache->trips[pos]) {
			getTripsFvUnsafe(fe, inst, par, ch1, ch2, NULL, cache);
		}
		score += cache->trips[pos]->score;
	}
	else {
		FeatureVector fv;
		fe->pipe->createTripsFeatureVector(inst, par, ch1, ch2, &fv);
		score = fe->parameters->getScore(&fv);
	}
	return score;
}

double FeatureExtractor::getTripsScoreAtomic(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& par, HeadIndex& ch1, HeadIndex& ch2, CacheTable* cache) {
	assert(fe->thread != 1);
	double score = 0.0;
	int id = -1;
	if (cache) {
		int parIdx = inst->wordToSeg(par);
		int ch2Idx = inst->wordToSeg(ch2);

		id = cache->arc2ID(parIdx, ch2Idx);
	}
	if (id >= 0) {

		//assert(id >= 0);
		int ch1Idx = inst->wordToSeg(ch1);

		int pos = id * cache->numSeg + ch1Idx;
		assert(pos < (int)cache->trips.size());
		item_ptr tmp_ptr = atomic_load(&cache->trips[pos]);
		if (!tmp_ptr) {
			getTripsFvAtomic(fe, inst, par, ch1, ch2, NULL, cache);
		}
		tmp_ptr = atomic_load(&(cache->trips[pos]));
		score += tmp_ptr->score;
	}
	else {
		FeatureVector fv;
		fe->pipe->createTripsFeatureVector(inst, par, ch1, ch2, &fv);
		score = fe->parameters->getScore(&fv);
	}
	return score;
}

//-------------------------------------------

void FeatureExtractor::getGPCFvUnsafe(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& gp, HeadIndex& par, HeadIndex& c,
		FeatureVector* fv, CacheTable* cache) {
	assert(fe->thread == 1);
	int id = -1;
	if (cache) {
		int gpIdx = inst->wordToSeg(gp);
		int parIdx = inst->wordToSeg(par);

		id = cache->arc2ID(gpIdx, parIdx);
	}
	if (id >= 0) {
		//assert(id >= 0);
		int cIdx = inst->wordToSeg(c);

		int pos = id * cache->numSeg + cIdx;
		assert(pos < (int)cache->gpc.size());
		if (!cache->gpc[pos]) {
			item_ptr tmp_ptr = item_ptr(new CacheItem());
			fe->pipe->createGPCFeatureVector(inst, gp, par, c, &tmp_ptr->fv);
			tmp_ptr->score = fe->parameters->getScore(&tmp_ptr->fv);
			cache->gpc[pos] = tmp_ptr;
		}
		if (fv)
			fv->concat(&cache->gpc[pos]->fv);
	}
	else {
		if (fv)
			fe->pipe->createGPCFeatureVector(inst, gp, par, c, fv);
	}
}

void FeatureExtractor::getGPCFvAtomic(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& gp, HeadIndex& par, HeadIndex& c,
		FeatureVector* fv, CacheTable* cache) {
	assert(fe->thread != 1);
	int id = -1;
	if (cache) {
		int gpIdx = inst->wordToSeg(gp);
		int parIdx = inst->wordToSeg(par);

		id = cache->arc2ID(gpIdx, parIdx);
	}
	if (id >= 0) {
		//assert(id >= 0);
		int cIdx = inst->wordToSeg(c);

		int pos = id * cache->numSeg + cIdx;
		assert(pos < (int)cache->gpc.size());
		item_ptr tmp_ptr = atomic_load(&cache->gpc[pos]);
		if (!tmp_ptr) {
			tmp_ptr = item_ptr(new CacheItem());
			fe->pipe->createGPCFeatureVector(inst, gp, par, c, &tmp_ptr->fv);
			tmp_ptr->score = fe->parameters->getScore(&tmp_ptr->fv);
			atomic_store(&cache->gpc[pos], tmp_ptr);
		}
		if (fv) {
			tmp_ptr = atomic_load(&(cache->gpc[pos]));
			fv->concat(&tmp_ptr->fv);
		}
	}
	else {
		if (fv)
			fe->pipe->createGPCFeatureVector(inst, gp, par, c, fv);
	}
}

double FeatureExtractor::getGPCScoreUnsafe(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& gp, HeadIndex& par, HeadIndex& c, CacheTable* cache) {
	assert(fe->thread == 1);
	double score = 0.0;
	int id = -1;
	if (cache) {
		int gpIdx = inst->wordToSeg(gp);
		int parIdx = inst->wordToSeg(par);

		id = cache->arc2ID(gpIdx, parIdx);
	}
	if (id >= 0) {
		//assert(id >= 0);
		int cIdx = inst->wordToSeg(c);

		int pos = id * cache->numSeg + cIdx;
		assert(pos < (int)cache->gpc.size());
		if (!cache->gpc[pos]) {
			getGPCFvUnsafe(fe, inst, gp, par, c, NULL, cache);
		}
		score += cache->gpc[pos]->score;
	}
	else {
		FeatureVector fv;
		fe->pipe->createGPCFeatureVector(inst, gp, par, c, &fv);
		score = fe->parameters->getScore(&fv);
	}
	return score;
}

double FeatureExtractor::getGPCScoreAtomic(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& gp, HeadIndex& par, HeadIndex& c, CacheTable* cache) {
	assert(fe->thread != 1);
	double score = 0.0;
	int id = -1;
	if (cache) {
		int gpIdx = inst->wordToSeg(gp);
		int parIdx = inst->wordToSeg(par);

		id = cache->arc2ID(gpIdx, parIdx);
	}
	if (id >= 0) {
		//assert(id >= 0);
		int cIdx = inst->wordToSeg(c);

		int pos = id * cache->numSeg + cIdx;
		assert(pos < (int)cache->gpc.size());
		item_ptr tmp_ptr = atomic_load(&cache->gpc[pos]);
		if (!tmp_ptr) {
			getGPCFvAtomic(fe, inst, gp, par, c, NULL, cache);
		}
		tmp_ptr = atomic_load(&(cache->gpc[pos]));
		score += tmp_ptr->score;
	}
	else {
		FeatureVector fv;
		fe->pipe->createGPCFeatureVector(inst, gp, par, c, &fv);
		score = fe->parameters->getScore(&fv);
	}
	return score;
}

//-------------------------------------------

void FeatureExtractor::getPosHOFvUnsafe(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& m, FeatureVector* fv, CacheTable* cache) {
	assert(fe->thread == 1);
	if (cache) {
		int pos = inst->wordToSeg(m);
		if (!cache->posho[pos]) {
			item_ptr tmp_ptr = item_ptr(new CacheItem());
			fe->pipe->createPosHOFeatureVector(inst, m, false, &tmp_ptr->fv);
			tmp_ptr->score = fe->parameters->getScore(&tmp_ptr->fv);
			cache->posho[pos] = tmp_ptr;
		}
		if (fv)
			fv->concat(&cache->posho[pos]->fv);
	}
	else {
		if (fv)
			fe->pipe->createPosHOFeatureVector(inst, m, false, fv);
	}
}

void FeatureExtractor::getPosHOFvAtomic(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& m, FeatureVector* fv, CacheTable* cache) {
	assert(fe->thread != 1);
	if (cache) {
		int pos = inst->wordToSeg(m);
		item_ptr tmp_ptr = atomic_load(&cache->posho[pos]);
		if (!tmp_ptr) {
			tmp_ptr = item_ptr(new CacheItem());
			fe->pipe->createPosHOFeatureVector(inst, m, false, &tmp_ptr->fv);
			tmp_ptr->score = fe->parameters->getScore(&tmp_ptr->fv);
			atomic_store(&cache->posho[pos], tmp_ptr);
		}
		if (fv) {
			tmp_ptr = atomic_load(&(cache->posho[pos]));
			fv->concat(&tmp_ptr->fv);
		}
	}
	else {
		if (fv)
			fe->pipe->createPosHOFeatureVector(inst, m, false, fv);
	}
}

double FeatureExtractor::getPosHOScoreUnsafe(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& m, CacheTable* cache) {
	assert(fe->thread == 1);
	double score = 0.0;
	if (cache) {
		int pos = inst->wordToSeg(m);
		if (!cache->posho[pos]) {
			getPosHOFvUnsafe(fe, inst, m, NULL, cache);
		}
		score = cache->posho[pos]->score;
	}
	else {
		FeatureVector fv;
		fe->pipe->createPosHOFeatureVector(inst, m, false, &fv);
		score = fe->parameters->getScore(&fv);
	}
	return score;
}

double FeatureExtractor::getPosHOScoreAtomic(FeatureExtractor* fe, DependencyInstance* inst, HeadIndex& m, CacheTable* cache) {
	assert(fe->thread != 1);
	double score = 0.0;
	if (cache) {
		int pos = inst->wordToSeg(m);
		item_ptr tmp_ptr = atomic_load(&cache->posho[pos]);
		if (!tmp_ptr) {
			getPosHOFvAtomic(fe, inst, m, NULL, cache);
		}
		tmp_ptr = atomic_load(&(cache->posho[pos]));
		score = tmp_ptr->score;
	}
	else {
		FeatureVector fv;
		fe->pipe->createPosHOFeatureVector(inst, m, false, &fv);
		score = fe->parameters->getScore(&fv);
	}
	return score;
}

//-------------------------------------------

void FeatureExtractor::getSegFv(DependencyInstance* inst, int wordid, FeatureVector* fv) {
	int pos = getSeg1OCachePos(wordid, inst->word[wordid].currSegCandID);
	assert(pos < (int)seg1o.size() && seg1o[pos]);
	if (fv) {
		fv->concat(&seg1o[pos]->fv);
	}
}

double FeatureExtractor::getSegScore(DependencyInstance* inst, int wordid) {
	int pos = getSeg1OCachePos(wordid, inst->word[wordid].currSegCandID);
	assert(pos < (int)seg1o.size() && seg1o[pos]);
	return seg1o[pos]->score;
}

//-------------------------------------------

void FeatureExtractor::getPos1OFv(DependencyInstance* inst, HeadIndex& m, FeatureVector* fv) {
	int pos = getPos1OCachePos(m.hWord, inst->word[m.hWord].currSegCandID, m.hSeg, inst->getElement(m).currPosCandID);
	assert(pos1o[pos]);
	if (fv) {
		fv->concat(&pos1o[pos]->fv);
	}
}

double FeatureExtractor::getPos1OScore(DependencyInstance* inst, HeadIndex& m) {
	int pos = getPos1OCachePos(m.hWord, inst->word[m.hWord].currSegCandID, m.hSeg, inst->getElement(m).currPosCandID);
	assert(pos < (int)pos1o.size() && pos1o[pos]);
	return pos1o[pos]->score;
}

//-------------------------------------------

double FeatureExtractor::getPartialDepScore(DependencyInstance* s, HeadIndex& x, CacheTable* cache) {
	// get score;
	double score = 0.0;

	HeadIndex& h = s->getElement(x).dep;
	score += getArcScore(this, s, h, x, cache);

	int mid = 0;
	int xid = s->wordToSeg(x);

	for (int i = 0; i < s->numWord; ++i) {
		SegInstance& segInst = s->word[i].getCurrSeg();

		for (int j = 0; j < segInst.size(); ++j) {
			HeadIndex m(i, j);
			HeadIndex h = segInst.element[j].dep;
			int hid = s->wordToSeg(h);

			vector<HeadIndex>& child = segInst.element[j].child;
			int aid = pipe->findRightNearestChildID(child, m);

			// right children
			HeadIndex prev = m;
			int previd = mid;
			for (unsigned int k = aid; k < child.size(); ++k) {
				HeadIndex curr = child[k];
				int currid = s->wordToSeg(curr);
				if (options->useCS) {
					if (xid >= previd && xid <= currid) {
						score += getTripsScore(this, s, m, prev, curr, cache);
						score += getSibsScore(this, s, prev, curr, prev == m, cache);
					}
				}

				prev = curr;
				previd = currid;
			}

			// left children
			prev = m;
			previd = mid;
			for (int k = aid - 1; k >= 0; --k) {
				HeadIndex curr = child[k];
				int currid = s->wordToSeg(curr);
				if (options->useCS) {
					if (xid >= currid && xid <= previd) {
						score += getTripsScore(this, s, m, prev, curr, cache);
						score += getSibsScore(this, s, prev, curr, prev == m, cache);
					}
				}

				prev = curr;
				previd = currid;
			}

			if (i > 0 && h.hWord >= 0) {
				HeadIndex gp = s->getElement(h).dep;
				if (options->useGP && gp.hWord >= 0) {
					if (xid == mid || xid == hid) {
						score += getGPCScore(this, s, gp, h, m, cache);
					}
				}
			}

			mid++;
		}
	}

	if (options->useHO) {
		FeatureVector fv;
		pipe->createPartialHighOrderFeatureVector(s, x, false, &fv);
		score += parameters->getScore(&fv);
	}

	return score;
}

double FeatureExtractor::getPartialBigramDepScore(DependencyInstance* s, HeadIndex& x, HeadIndex& y, CacheTable* cache) {
	// get score;
	double score = 0.0;

	HeadIndex& h = s->getElement(x).dep;
	assert(h == s->getElement(y).dep);

	score += getArcScore(this, s, h, x, cache);
	score += getArcScore(this, s, h, y, cache);

	int mid = 0;
	int xid = s->wordToSeg(x);

	for (int i = 0; i < s->numWord; ++i) {
		SegInstance& segInst = s->word[i].getCurrSeg();

		for (int j = 0; j < segInst.size(); ++j) {
			HeadIndex m(i, j);
			HeadIndex h = segInst.element[j].dep;
			int hid = s->wordToSeg(h);

			vector<HeadIndex>& child = segInst.element[j].child;
			int aid = pipe->findRightNearestChildID(child, m);

			// right children
			HeadIndex prev = m;
			int previd = mid;
			for (unsigned int k = aid; k < child.size(); ++k) {
				HeadIndex curr = child[k];
				int currid = s->wordToSeg(curr);
				if (options->useCS) {
					if (xid + 1 >= previd && xid <= currid) {
						score += getTripsScore(this, s, m, prev, curr, cache);
						score += getSibsScore(this, s, prev, curr, prev == m, cache);
					}
				}

				prev = curr;
				previd = currid;
			}

			// left children
			prev = m;
			previd = mid;
			for (int k = aid - 1; k >= 0; --k) {
				HeadIndex curr = child[k];
				int currid = s->wordToSeg(curr);
				if (options->useCS) {
					if (xid + 1 >= currid && xid <= previd) {
						score += getTripsScore(this, s, m, prev, curr, cache);
						score += getSibsScore(this, s, prev, curr, prev == m, cache);
					}
				}

				prev = curr;
				previd = currid;
			}

			if (i > 0 && h.hWord >= 0) {
				HeadIndex gp = s->getElement(h).dep;
				if (options->useGP && gp.hWord >= 0) {
					if (xid == mid || xid == hid || xid + 1 == mid || xid + 1 == hid) {
						score += getGPCScore(this, s, gp, h, m, cache);
					}
				}
			}

			mid++;
		}
	}

	if (options->useHO) {
		FeatureVector fv;
		pipe->createPartialHighOrderFeatureVector(s, x, true, &fv);
		score += parameters->getScore(&fv);
	}

	return score;
}

double FeatureExtractor::getPartialPosScore(DependencyInstance* s, HeadIndex& x, CacheTable* cache) {
	// change POS of x
	// get score;
	double score = 0.0;

	int mid = 0;
	int xid = s->wordToSeg(x);
	for (int i = 0; i < s->numWord; ++i) {
		SegInstance& segInst = s->word[i].getCurrSeg();

		for (int j = 0; j < segInst.size(); ++j) {
			HeadIndex m(i, j);
			HeadIndex h = segInst.element[j].dep;
			int hid = s->wordToSeg(h);

			if (i > 0 && h.hWord >= 0) {
				int small = hid < mid ? hid : mid;
				int large = hid > mid ? hid : mid;
				if (xid >= small - 1 && xid <= large + 1) {
					score += getArcScore(this, s, h, m, cache);
				}

				if (options->useSP) {
					score += getPos1OScore(s, m);
					score += getPosHOScore(this, s, m, cache);
				}
			}

			vector<HeadIndex>& child = segInst.element[j].child;
			int aid = pipe->findRightNearestChildID(child, m);

			// right children
			HeadIndex prev = m;
			int previd = mid;
			for (unsigned int k = aid; k < child.size(); ++k) {
				HeadIndex curr = child[k];
				int currid = s->wordToSeg(curr);
				if (options->useCS) {
					if ((xid >= mid - 1 && xid <= mid + 1)
							|| (xid >= previd - 1 && xid <= previd + 1)
							|| (xid >= currid - 1 && xid <= currid + 1)) {
						score += getTripsScore(this, s, m, prev, curr, cache);
						score += getSibsScore(this, s, prev, curr, prev == m, cache);
					}
				}

				prev = curr;
				previd = currid;
			}

			// left children
			prev = m;
			previd = mid;
			for (int k = aid - 1; k >= 0; --k) {
				HeadIndex curr = child[k];
				int currid = s->wordToSeg(curr);
				if (options->useCS) {
					if ((xid >= mid - 1 && xid <= mid + 1)
							|| (xid >= previd - 1 && xid <= previd + 1)
							|| (xid >= currid - 1 && xid <= currid + 1)) {
						score += getTripsScore(this, s, m, prev, curr, cache);
						score += getSibsScore(this, s, prev, curr, prev == m, cache);
					}
				}

				prev = curr;
				previd = currid;
			}

			if (i > 0 && h.hWord >= 0) {
				HeadIndex gp = s->getElement(h).dep;
				int gpid = s->wordToSeg(gp);
				if (options->useGP && gp.hWord >= 0) {
					if ((xid >= mid - 1 && xid <= mid + 1)
							|| (xid >= hid - 1 && xid <= hid + 1)
							|| (xid >= gpid - 1 && xid <= gpid + 1)) {
						score += getGPCScore(this, s, gp, h, m, cache);
					}
				}
			}

			mid++;
		}

	}
	assert(mid == s->getNumSeg());

	if (options->useHO) {
		FeatureVector fv;
		pipe->createPartialPosHighOrderFeatureVector(s, x, &fv);
		score += parameters->getScore(&fv);
	}

	return score;
}

double FeatureExtractor::getScore(DependencyInstance* s, CacheTable* cache) {

	// get score;
	double score = 0.0;

	for (int i = 0; i < s->numWord; ++i) {
		SegInstance& segInst = s->word[i].getCurrSeg();

		for (int j = 0; j < segInst.size(); ++j) {
			HeadIndex m(i, j);
			HeadIndex& h = segInst.element[j].dep;

			if (i > 0 && h.hWord >= 0) {
				score += getArcScore(this, s, h, m, cache);
				if (options->useSP) {
					score += getPos1OScore(s, m);
					score += getPosHOScore(this, s, m, cache);
				}
			}

			vector<HeadIndex>& child = segInst.element[j].child;
			int aid = pipe->findRightNearestChildID(child, m);

			// right children
			HeadIndex prev = m;
			for (unsigned int k = aid; k < child.size(); ++k) {
				HeadIndex curr = child[k];
				if (options->useCS) {
					score += getTripsScore(this, s, m, prev, curr, cache);
					score += getSibsScore(this, s, prev, curr, prev == m, cache);
				}

				prev = curr;
			}

			// left children
			prev = m;
			for (int k = aid - 1; k >= 0; --k) {
				HeadIndex curr = child[k];
				if (options->useCS) {
					score += getTripsScore(this, s, m, prev, curr, cache);
					score += getSibsScore(this, s, prev, curr, prev == m, cache);
				}

				prev = curr;
			}

			if (i > 0 && h.hWord >= 0) {
				HeadIndex gp = s->getElement(h).dep;
				if (options->useGP && gp.hWord >= 0) {
					score += getGPCScore(this, s, gp, h, m, cache);
				}
			}
		}

		if (options->useSP && i > 0) {
			score += getSegScore(s, i);
		}
	}

	if (options->useHO) {
		FeatureVector fv;
		pipe->createHighOrderFeatureVector(s, &fv);
		score += parameters->getScore(&fv);
	}

	return score;
}

double FeatureExtractor::getScore(DependencyInstance* s) {
	// get cache table
	CacheTable* cache = getCacheTable(s);
	return getScore(s, cache);
}

void FeatureExtractor::getPartialFv(DependencyInstance* s, HeadIndex& x, FeatureVector* fv) {
	// get cache table
	ThrowException("should not go here");

	CacheTable* cache = getCacheTable(s);

	HeadIndex& h = s->getElement(x).dep;
	getArcFv(this, s, h, x, fv, cache);
}

void FeatureExtractor::getFv(DependencyInstance* s, FeatureVector* fv) {
	// get cache table
	CacheTable* cache = getCacheTable(s);

	for (int i = 0; i < s->numWord; ++i) {
		SegInstance& segInst = s->word[i].getCurrSeg();

		for (int j = 0; j < segInst.size(); ++j) {
			HeadIndex m(i, j);
			HeadIndex h = segInst.element[j].dep;

			if (i > 0 && h.hWord >= 0) {
				getArcFv(this, s, h, m, fv, cache);
				if (options->useSP) {
					getPos1OFv(s, m, fv);
					getPosHOFv(this, s, m, fv, cache);
				}
			}

			vector<HeadIndex>& child = segInst.element[j].child;
			int aid = pipe->findRightNearestChildID(child, m);

			// right children
			HeadIndex prev = m;
			for (unsigned int k = aid; k < child.size(); ++k) {
				HeadIndex curr = child[k];
				if (options->useCS) {
					getTripsFv(this, s, m, prev, curr, fv, cache);
					getSibsFv(this, s, prev, curr, prev == m, fv, cache);
				}

				prev = curr;
			}

			// left children
			prev = m;
			for (int k = aid - 1; k >= 0; --k) {
				HeadIndex curr = child[k];
				if (options->useCS) {
					getTripsFv(this, s, m, prev, curr, fv, cache);
					getSibsFv(this, s, prev, curr, prev == m, fv, cache);
				}

				prev = curr;
			}

			if (i > 0 && h.hWord >= 0) {
				HeadIndex gp = s->getElement(h).dep;
				if (options->useGP && gp.hWord >= 0) {
					getGPCFv(this, s, gp, h, m, fv, cache);
				}
			}
		}

		if (options->useSP && i > 0) {
			getSegFv(s, i, fv);
		}
	}

	if (options->useHO) {
		pipe->createHighOrderFeatureVector(s, fv);
	}
}

vector<bool> FeatureExtractor::isPruned(DependencyInstance* s, HeadIndex& m, CacheTable* cache) {
	// get the list every time, for efficiency
	vector<bool> pruned;

	if (cache) {
		pruned.resize(cache->numSeg);
		int id = 0;

		int modIndex = s->wordToSeg(m);
		int headIndex = 0;
		for (int hw = 0; hw < s->numWord; ++hw) {
			SegInstance& headSeg = s->word[hw].getCurrSeg();
			for (int hs= 0; hs < headSeg.size(); ++hs) {
				pruned[id] = cache->isPruned(headIndex, modIndex);
				headIndex++;
				id++;
			}
		}
		assert(id == cache->numSeg);
	}
	else {
		if (pruner) {
			//ThrowException("isPruned: not implemented yet");
			vector<bool> tmpPruned;
			pfe->prune(s, m, tmpPruned);

			int p = 0;
			for (int hw = 0; hw < s->numWord; ++hw) {
				SegInstance& headSeg = s->word[hw].getCurrSeg();
				for (int hs = 0; hs < headSeg.size(); ++hs) {
					if (hw != m.hWord || hs != m.hSeg) {
						if (!tmpPruned[p]) {
							pruned.push_back(false);
						}
						else {
							pruned.push_back(true);
						}
						p++;
					}
					else {
						pruned.push_back(true);
					}
				}
			}
		}
		else {
			for (int hw = 0; hw < s->numWord; ++hw) {
				SegInstance& headSeg = s->word[hw].getCurrSeg();
				for (int hs= 0; hs < headSeg.size(); ++hs) {
					if (hw != m.hWord || hs != m.hSeg)
						pruned.push_back(false);
					else
						pruned.push_back(true);
				}
			}
		}
	}

	return pruned;	// move(pruned)
}

} /* namespace segparser */
