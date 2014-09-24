/*
 * SampleRankDecoder.cpp
 *
 *  Created on: Apr 8, 2014
 *      Author: yuanz
 */

#include "SampleRankDecoder.h"
#include <float.h>
#include "../util/Random.h"

namespace segparser {

SampleRankDecoder::SampleRankDecoder(Options* options) : DependencyDecoder(options) {
	r.setSeed(seed);
}

SampleRankDecoder::~SampleRankDecoder() {
}

double SampleRankDecoder::getInitTemp(DependencyInstance* inst, FeatureExtractor* fe) {
	// get cache table
	CacheTable* cache = fe->getCacheTable(inst);

	double maxWorse = 0.0;
	for (int mw = 1; mw < inst->numWord; ++mw) {
		SegInstance& modSeg = inst->word[mw].getCurrSeg();
		for (int ms = 0; ms < modSeg.size(); ++ms) {
			HeadIndex m(mw, ms);
			double currScore = fe->getPartialDepScore(inst, m, cache);

			// get pruned list
			vector<bool> isPruned = move(fe->isPruned(inst, m, cache));
			int segID = -1;

			// build score
			HeadIndex oldDep = modSeg.element[ms].dep;
			for (int hw = 0; hw < inst->numWord; ++hw) {
				SegInstance& headSeg = inst->word[hw].getCurrSeg();
				for (int hs = 0; hs < headSeg.size(); ++hs) {
					segID++;
					if (isPruned[segID]) {
						continue;
					}

					HeadIndex h(hw, hs);
					if (isAncestor(inst, m, h))		// loop
						continue;

					if (options->proj && !isProj(inst, h, m))
						continue;

					HeadIndex oldH = modSeg.element[ms].dep;
					modSeg.element[ms].dep = h;
					inst->updateChildList(h, oldH, m);

	        		double score = fe->getPartialDepScore(inst, m, cache);
	        		if (score - currScore < maxWorse) {
	        			maxWorse = score - currScore;
	        		}
				}
			}

			HeadIndex oldH = modSeg.element[ms].dep;
			modSeg.element[ms].dep = oldDep;
			inst->updateChildList(oldDep, oldH, m);
		}
	}
	if (abs(maxWorse) < 1e-4) {
		//System.out.println("init temp bug");
		return 10.0;
	}
	else
		return maxWorse / log(0.7);

}

bool SampleRankDecoder::accept(double newScore, double oldScore,
		bool useAnnealingTransit, double old2newProb, double new2oldProb,
		double temperature, Random& r) {
	if (useAnnealingTransit) {
		if (newScore >= oldScore)
			return true;
		double accRatio = exp((newScore - oldScore) / temperature);
		if (r.nextDouble() < accRatio)
			return true;
		else
			return false;
	}
	else {
		double accRatio = exp((newScore + log(new2oldProb) - oldScore - log(old2newProb)) / temperature);
		if (r.nextDouble() < accRatio)
			return true;
		else
			return false;
	}
}

void SampleRankDecoder::decode(DependencyInstance* inst, DependencyInstance* gold, FeatureExtractor* fe) {
	//fe->pipe->createFeatureVector(gold, &gold->fv);
	//double goldScore2 = fe->parameters->getScore(&gold->fv);
	//double goldScore = fe->getScore(gold);

	// initialize to remove any gold information
	initInst(inst, fe);


	Random dr(seed);

	double initTemperature = getInitTemp(inst, fe);

	double coolingRate = 0.95;
	int maxIter = 300;
	int miniIter = 20;
	int restart = 2;

	bool useAnnealingTransit = true;		// if true, don't need to compute original prob

	double bestScore = -DBL_MAX;
	VariableInfo origVar(inst);
	VariableInfo bestVar(inst);

	for (int res = 0; res < restart; ++res) {
	   	// recover the starting
	   	origVar.loadInfoToInst(inst);
	   	inst->constructConversionList();
	   	inst->setOptSegPosCount();
	   	inst->buildChild();

	   	double T = initTemperature;
	   	double currScore = 0.0;

   		for (int iter = 0; iter < maxIter; ++iter) {
   			bool change = false;
   			for (int z = 0; z < miniIter; ++z) {
   	      		vector<HeadIndex> idx(inst->getNumSeg());
   	      		HeadIndex root(0, 0);
   	     		int id = getBottomUpOrder(inst, root, idx, idx.size() - 1);
   	     		assert(id == 0);

   	     		vector<bool> changedWord(inst->numWord);		// record which word has been sampled
   	     		for (unsigned int y = 1; y < idx.size(); ++y) {
   	     			HeadIndex& m = idx[y];
   	     			if (changedWord[m.hWord])
   	     				continue;

   	     			changedWord[m.hWord] = true;
   	     			VariableInfo oldVar(inst);

   	     			// 1. old seg/pos prob
   	     			double oldPosSegProb = useAnnealingTransit ? 1.0 : getSegPosProb(inst->word[m.hWord]);

   	     			// 2. old dep prob
   	     			double oldDepProb = useAnnealingTransit ? 1.0 : getMHDepProb(inst, NULL, fe, m.hWord);

   	     			// 3. get old score
   	     			//double oldScore = getScore(inst, fe, m.hWord);
   	     			double oldScore = fe->getScore(inst);

   	     			// 4. sample a new seg/pos and get prob
   	     			double newPosSegProb = sampleSegPos(inst, NULL, m.hWord, dr);

   	     			// 5. sample a new deps and get prob
  	     			double newDepProb = sampleMHDepProb(inst, NULL, fe, m.hWord, dr);

  	     			inst->buildChild();

  	     			if (oldVar.isChanged(inst)) {
   	   	     			// 6. get new sore
   	   	     			//double newScore = getScore(inst, fe, m.hWord);
   	     				double newScore = fe->getScore(inst);

   	   	     			//cout << "old: " << oldScore << " " << "new: " << newScore << endl;

   	   	     			// 7. accept/reject
   	   					if (!accept(newScore, oldScore, useAnnealingTransit,
   	   							newPosSegProb * newDepProb, oldPosSegProb * oldDepProb, T, dr)) {
   	   						// not accept
   	   						oldVar.loadInfoToInst(inst);
   	   					   	inst->constructConversionList();
   	   					   	inst->setOptSegPosCount();
   	   					   	inst->buildChild();
   	   					}
   	   					else {
   	   						// accept
   	   						change = true;
   	   					}
   	     			}
   	     		}
   			} 	// end of mini iter

   			currScore = fe->getScore(inst);
   			if (currScore > bestScore) {
   				bestScore = currScore;
   				bestVar.copyInfoFromInst(inst);
   			}

   			//cout << "iter: " << iter << endl;

   			if (!change) {
   				break;
   			}
   			T = T * coolingRate;

   			if (T < 1e-3)
   				break;
   		}
	}
	bestVar.loadInfoToInst(inst);
	inst->constructConversionList();
	inst->setOptSegPosCount();
	inst->buildChild();

	//cout << "score: " << bestScore << "\tgold score:" << goldScore2 << endl;
}

void SampleRankDecoder::train(DependencyInstance* gold, DependencyInstance* pred, FeatureExtractor* fe, int trainingIter) {

	if (trainingIter % options->restartIter == 1)
		initInst(pred, fe);

	DependencyInstance oldPred = *pred;		// auxiliary instance
	oldPred.constructConversionList();
	oldPred.setOptSegPosCount();
	oldPred.buildChild();

	vector<HeadIndex> idx(pred->getNumSeg());
	HeadIndex root(0, 0);
	int id = getBottomUpOrder(pred, root, idx, idx.size() - 1);
	assert(id == 0);

	vector<bool> changedWord(pred->numWord);		// record which word has been sampled
	for (unsigned int y = 1; y < idx.size(); ++y) {
		HeadIndex& m = idx[y];
		if (changedWord[m.hWord])
			continue;

		changedWord[m.hWord] = true;
		//VariableInfo oldVar(pred);

		// 1. old seg/pos prob
		double oldPosSegProb = getSegPosProb(pred->word[m.hWord]);

		// 2. old dep prob
		double oldDepProb = getMHDepProb(pred, NULL, fe, m.hWord);

		// 3. get old score
		double oldLoss = 0.0;
		//SegInstance& oldSegInst = pred->word[m.hWord].getCurrSeg();
		//for (int i = 0; i < oldSegInst.size(); ++i)
		//	oldLoss += fe->parameters->elementError(gold->word[m.hWord], pred->word[m.hWord], i);
		oldLoss = fe->parameters->wordError(gold->word[m.hWord], pred->word[m.hWord]);
		//cout << gold->word[m.hWord].getCurrSeg() << endl << pred->word[m.hWord].getCurrSeg() << endl << oldLoss << endl << endl;
		FeatureVector oldFV;
		fe->getFv(pred, &oldFV);		// can be more efficient
		double oldScore = fe->parameters->getScore(&oldFV);

		// 4. sample a new seg/pos and get prob
		double newPosSegProb = sampleSegPos(pred, NULL, m.hWord, r);

		// 5. sample a new deps and get prob
		double newDepProb = sampleMHDepProb(pred, NULL, fe, m.hWord, r);

		pred->buildChild();

		// 6. get new sore
		double newLoss = 0.0;
		//SegInstance& newSegInst = pred->word[m.hWord].getCurrSeg();
		//for (int i = 0; i < newSegInst.size(); ++i)
		//	newLoss += fe->parameters->elementError(gold->word[m.hWord], pred->word[m.hWord], i);
		newLoss = fe->parameters->wordError(gold->word[m.hWord], pred->word[m.hWord]);
		FeatureVector newFV;
		fe->getFv(pred, &newFV);
		double newScore = fe->parameters->getScore(&newFV);

		//cout << "old: " << oldScore << " " << oldLoss << " " << oldPosSegProb << " " << oldDepProb << endl;
		//cout << "new: " << newScore << " " << newLoss << " " << newPosSegProb << " " << newDepProb << endl;
		if (oldLoss > newLoss + 1e-6 && newScore - oldScore < oldLoss - newLoss) {
			// update toward new tree
			newFV.concatNeg(&oldFV);
			fe->parameters->update(pred, &oldPred, &newFV, (oldLoss - newLoss) - (newScore - oldScore), fe, updateTimes);
		}
		else if (oldLoss + 1e-6 < newLoss && oldScore - newScore < newLoss - oldLoss) {
			// update toward old tree
			oldFV.concatNeg(&newFV);
			fe->parameters->update(&oldPred, pred, &oldFV, (newLoss - oldLoss) - (oldScore - newScore), fe, updateTimes);
		}

		// 7. accept/reject
		if (!accept(newScore, oldScore, false,
				newPosSegProb * newDepProb, oldPosSegProb * oldDepProb, 1.0, r)) {
			// not accept
			//oldVar.loadInfoToInst(pred);
			copyVarInfo(pred, &oldPred);
			pred->constructConversionList();
			pred->setOptSegPosCount();
			pred->buildChild();
		}
		else {
			// accept
			copyVarInfo(&oldPred, pred);
			oldPred.constructConversionList();
			oldPred.setOptSegPosCount();
			oldPred.buildChild();
		}
	}

	updateTimes++;

	if (options->updateGold) {
		assert(gold->fv.binaryIndex.size() > 0);

		FeatureVector oldFV;
		fe->getFv(pred, &oldFV);
		double oldScore = fe->parameters->getScore(&oldFV);

		FeatureVector& newFV = gold->fv;
		double newScore = fe->parameters->getScore(&newFV);

		double err = 0.0;
		for (int i = 0; i < pred->numWord; ++i) {
			err += fe->parameters->wordError(gold->word[i], pred->word[i]);
		}
		if (newScore - oldScore < err) {
			FeatureVector diffFV;		// make a copy
			diffFV.concat(&newFV);
			diffFV.concatNeg(&oldFV);

			if (err - (newScore - oldScore) > 1e-4) {
				fe->parameters->update(gold, pred, &diffFV, err - (newScore - oldScore), fe, updateTimes);
				updateTimes++;
			}
		}
	}
}

void SampleRankDecoder::copyVarInfo(DependencyInstance* orig, DependencyInstance* curr) {
	assert(orig->numWord == curr->numWord);
	for (int wordID = 0; wordID < curr->numWord; ++wordID) {
		orig->word[wordID].currSegCandID = curr->word[wordID].currSegCandID;

		SegInstance& origInst = orig->word[wordID].getCurrSeg();
		SegInstance& currInst = curr->word[wordID].getCurrSeg();

		assert(origInst.size() == currInst.size());

		for (int i = 0; i < currInst.size(); ++i) {
			origInst.element[i].currPosCandID = currInst.element[i].currPosCandID;
			origInst.element[i].dep = currInst.element[i].dep;
		}
	}
}

} /* namespace segparser */
