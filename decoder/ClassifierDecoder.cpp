/*
 * ClassifierDecoder.cpp
 *
 *  Created on: May 7, 2014
 *      Author: yuanz
 */

#include "ClassifierDecoder.h"
#include <float.h>

namespace segparser {

ClassifierDecoder::ClassifierDecoder(Options* options) : DependencyDecoder(options) {
}

ClassifierDecoder::~ClassifierDecoder() {
}

void ClassifierDecoder::decode(DependencyInstance* inst, DependencyInstance* gold, FeatureExtractor* fe) {
	ThrowException("no need to decode");
}

void ClassifierDecoder::train(DependencyInstance* gold, DependencyInstance* pred, FeatureExtractor* fe, int trainintIter) {
	// check the gold and pred have same seg/pos
	for (int i = 1; i < pred->numWord; ++i) {
		assert(gold->word[i].currSegCandID == pred->word[i].currSegCandID);

		SegInstance& goldInst = gold->word[i].getCurrSeg();
		SegInstance& predInst = pred->word[i].getCurrSeg();

		assert(goldInst.size() == predInst.size());
		for (int j = 0; j < predInst.size(); ++j) {
			assert(goldInst.element[j].currPosCandID == predInst.element[j].currPosCandID);
		}
	}

	CacheTable* cache = fe->getCacheTable(pred);
	boost::shared_ptr<CacheTable> tmpCache = boost::shared_ptr<CacheTable>(new CacheTable());
	if (!cache) {
		cache = tmpCache.get();		// temporary cache for this run
		tmpCache->initCacheTable(fe->type, pred, NULL, options);
	}

	for (int mw = 1; mw < pred->numWord; ++mw) {
		SegInstance& segInst = pred->word[mw].getCurrSeg();
		SegInstance& goldInst = gold->word[mw].getCurrSeg();

		for (int ms = 0; ms < segInst.size(); ++ms) {
			if (options->heuristicDep && ms != pred->word[mw].getCurrSeg().inNode) {
				continue;
			}

			HeadIndex m(mw, ms);
			findOptHead(pred, gold, m, fe, cache);

			if (segInst.element[ms].dep != goldInst.element[ms].dep) {
				FeatureVector newFV;
				fe->getArcFv(fe, gold, goldInst.element[ms].dep, m, &newFV, cache);
				double newScore = fe->parameters->getScore(&newFV);

				FeatureVector oldFV;
				fe->getArcFv(fe, pred, segInst.element[ms].dep, m, &oldFV, cache);
				double oldScore = fe->parameters->getScore(&oldFV);

				newFV.concatNeg(&oldFV);

				double err = fe->parameters->wordError(gold->word[mw], pred->word[mw]);

				if (err - (newScore - oldScore) > 1e-4) {
					fe->parameters->update(gold, pred, &newFV, err - (newScore - oldScore), fe, updateTimes);
					updateTimes++;
				}
			}
		}
	}
}

void ClassifierDecoder::findOptHead(DependencyInstance* pred, DependencyInstance* gold, HeadIndex& m, FeatureExtractor* fe, CacheTable* cache) {

	assert(cache && cache->numSeg == pred->getNumSeg());

	// get pruned list
	vector<bool> isPruned = move(fe->isPruned(pred, m, cache));
	int segID = -1;

	SegElement& predSegEle = pred->getElement(m);
	double bestScore = -DBL_MAX;
	HeadIndex bestDep(-1, 0);

	for (int hw = 0; hw < pred->numWord; ++hw) {
		SegInstance& segInst = pred->word[hw].getCurrSeg();

		for (int hs = 0; hs < segInst.size(); ++hs) {
			segID++;
			if (isPruned[segID]) {
				continue;
			}

			if (options->heuristicDep)
				assert(hw != m.hWord && hs == segInst.outNode);
			else
				assert(hw != m.hWord || hs != m.hSeg);

			HeadIndex h(hw, hs);

			predSegEle.dep = h;
			FeatureVector fv;
			fe->getArcFv(fe, pred, h, m, &fv, cache);
			double score = fe->parameters->getScore(&fv);
			if (gold) {
				// add loss
				score += fe->parameters->wordError(gold->word[m.hWord], pred->word[m.hWord]);
			}

			if (score > bestScore + 1e-6) {
				bestScore = score;
				bestDep = h;
			}
		}
	}
	assert(segID == (int)isPruned.size() - 1);
	predSegEle.dep = bestDep;
}

} /* namespace segparser */
