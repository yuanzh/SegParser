/*
 * HillClimbingDecoder.cpp
 *
 *  Created on: May 1, 2014
 *      Author: yuanz
 */

#include "HillClimbingDecoder.h"
#include <float.h>

namespace segparser {

void* hillClimbingThreadFunc(void* instance) {
	HillClimbingDecoder* data = (HillClimbingDecoder*)instance;

	pthread_t selfThread = pthread_self();
	int selfid = -1;
	for (int i = 0; i < data->thread; ++i)
		if (selfThread == data->threadID[i]) {
			selfid = i;
			break;
		}
	assert(selfid != -1);

	data->debug("pending.", selfid);
	bool jobsDone = false;

	while (true) {
		pthread_mutex_lock(&data->taskMutex[selfid]);

		while (data->taskDone[selfid]) {
			 pthread_cond_wait(&data->taskStartCond[selfid], &data->taskMutex[selfid]);
		}

		if (data->threadExit[selfid]) {
			// jobs done
			data->taskDone[selfid] = true;
			jobsDone = true;
		}

		pthread_mutex_unlock(&data->taskMutex[selfid]);

		if (jobsDone) {
			break;
		}

		DependencyInstance pred = *data->pred;			// copy the instance
		DependencyInstance* gold = data->gold;
		FeatureExtractor* fe = data->fe;				// shared fe

		Random r(data->options->seed + 2 + selfid);		// set different seed for different thread

		int maxIter = 1000;

		//if (selfid == 0)
		//	cout << "start sampling" << endl;

		// begin sampling
		bool done = false;
		int iter = 0;
		for (iter = 0; iter < maxIter && !done; ++iter) {

			// sample seg/pos
			for (int i = 1; i < pred.numWord; ++i) {
				data->sampleSegPos1O(&pred, gold, fe, i, r);
			}

			CacheTable* cache = fe->getCacheTable(&pred);
			boost::shared_ptr<CacheTable> tmpCache = boost::shared_ptr<CacheTable>(new CacheTable());
			if (!cache) {
				cache = tmpCache.get();		// temporary cache for this run
				tmpCache->initCacheTable(fe->type, &pred, fe->pfe.get(), data->options);
			}

			// sample a new tree from first order
			int len = pred.getNumSeg();
			vector<bool> toBeSampled(len);
			int id = 1;		// skip root
			for (int i = 1; i < pred.numWord; ++i) {
				SegInstance& segInst = pred.word[i].getCurrSeg();

				for (int j = 0; j < segInst.size(); ++j) {
					if (data->options->heuristicDep) {
						if (j == segInst.inNode)
							toBeSampled[id] = true;
						else
							toBeSampled[id] = false;
					}
					else {
						toBeSampled[id] = true;
					}
					id++;
				}
			}
			assert(id == len);

			data->randomWalkSampler(&pred, gold, fe, cache, toBeSampled, r);
			pred.buildChild();

			// improve tree by hill climbing
			// improve pos
			{
   	      		vector<HeadIndex> idx(len);
   	      		HeadIndex root(0, 0);
   	     		int id = data->getBottomUpOrder(&pred, root, idx, idx.size() - 1);
   	     		assert(id == 0);

   	     		for (unsigned int y = 1; y < idx.size(); ++y) {
   	     			HeadIndex& m = idx[y];
   	     			bool posChanged = data->findOptPos(&pred, gold, m, fe, cache);
   	     			if (posChanged) {
   	     				// update cache table
   	     				cache = fe->getCacheTable(&pred);
   	     			}
  	     		}

   	     		if (fe->pfe) {
   	     			// has pruner, need to sample the tree again
					if (!cache) {
						tmpCache = boost::shared_ptr<CacheTable>(new CacheTable());
						cache = tmpCache.get();		// temporary cache for this run
						tmpCache->initCacheTable(fe->type, &pred, fe->pfe.get(), data->options);
					}

					id = 1;		// skip root
					for (int i = 1; i < pred.numWord; ++i) {
						SegInstance& segInst = pred.word[i].getCurrSeg();

						for (int j = 0; j < segInst.size(); ++j) {
							if (toBeSampled[id]) {
								segInst.element[j].dep.setIndex(-1, 0);
							}
							id++;
						}
					}

					data->randomWalkSampler(&pred, gold, fe, cache, toBeSampled, r);
   	     			pred.buildChild();
  	     		}
			}

			if (!cache) {
				tmpCache = boost::shared_ptr<CacheTable>(new CacheTable());
				cache = tmpCache.get();		// temporary cache for this run
				tmpCache->initCacheTable(fe->type, &pred, fe->pfe.get(), data->options);
			}

			bool change = true;
			int loop = 0;
			//if (selfid == 0)
			//	data->debug("aaa: ", selfid);
			while (change && loop < 100) {
				change = false;
				loop++;		// avoid dead loop

   	      		vector<HeadIndex> idx(len);
   	      		HeadIndex root(0, 0);
   	     		int id = data->getBottomUpOrder(&pred, root, idx, idx.size() - 1);
   	     		assert(id == 0);

   	     		for (unsigned int y = 1; y < idx.size(); ++y) {
   	     			HeadIndex& m = idx[y];

   	     			// find optimum pos
   	     			//bool posChanged = data->findOptPos(&pred, gold, m, fe, cache);
   	     			//if (posChanged) {
   	     				// update cache table
   	     			//	cache = fe->getCacheTable(&pred);
   	     			//}
   	     			//change = change || posChanged;

   	     			// find the optimum head (cost augmented)
   	     			//HeadIndex o = pred.getElement(m).dep;
   	     			bool depChanged = data->findOptHead(&pred, gold, m, fe, cache);
   	     			//if (c && selfid == 0) {
   	     			//	cout << m << " " << o << " " << pred.getElement(m).dep << endl;
   	     			//}
   	     			change = change || depChanged;
				}
			}

			// update best solution
			double currScore = fe->getScore(&pred);
			if (gold) {
				for (int i = 1; i < pred.numWord; ++i)
					currScore += fe->parameters->wordError(gold->word[i], pred.word[i]);
			}

			//double tmps = 0.0;

			pthread_mutex_lock(&data->updateMutex);

			if (data->unChangeIter >= data->convergeIter)
				done = true;

			if (currScore > data->bestScore + 1e-6) {
				data->bestScore = currScore;
				data->best.copyInfoFromInst(&pred);
				if (!done)
					data->unChangeIter = 0;
			}
			else {
				data->unChangeIter++;
			}
			//tmps = data->bestScore;

			pthread_mutex_unlock(&data->updateMutex);
		}

		//if (selfid == 0)
		//	cout << "finish sampling. iter: " << iter << endl;

		pthread_mutex_lock(&data->taskMutex[selfid]);

		data->taskDone[selfid] = true;
		pthread_cond_signal(&data->taskDoneCond[selfid]);

		pthread_mutex_unlock(&data->taskMutex[selfid]);

	}

	pthread_exit(NULL);
	return NULL;
}

HillClimbingDecoder::HillClimbingDecoder(Options* options, int thread, int convergeIter) : DependencyDecoder(options), thread(thread), convergeIter(convergeIter) {
	cout << "converge iter: " << convergeIter << endl;
}

HillClimbingDecoder::~HillClimbingDecoder() {
}

void HillClimbingDecoder::initialize() {
	threadID.resize(thread);
	taskMutex.resize(thread);
	taskStartCond.resize(thread);
	taskDoneCond.resize(thread);
	taskDone.resize(thread);
	threadExit.resize(thread);

	updateMutex = PTHREAD_MUTEX_INITIALIZER;
	for (int i = 0; i < thread; ++i) {
		threadID[i] = -1;
		taskMutex[i] = PTHREAD_MUTEX_INITIALIZER;
		taskStartCond[i] = PTHREAD_COND_INITIALIZER;
		taskDoneCond[i] = PTHREAD_COND_INITIALIZER;
		taskDone[i] = true;
		threadExit[i] = false;

		int rc = pthread_create(&threadID[i], NULL, hillClimbingThreadFunc, (void*)this);

		if (rc) {
			cout << i << " " << rc << endl;
			ThrowException("Create train thread failed.");
		}
	}
}

void HillClimbingDecoder::shutdown() {
	for (int i = 0; i < thread; ++i) {
		// send start signal
		pthread_mutex_lock(&taskMutex[i]);

		assert(taskDone[i]);
		threadExit[i] = true;
		taskDone[i] = false;
		pthread_cond_signal(&taskStartCond[i]);

		pthread_mutex_unlock(&taskMutex[i]);
	}

	for (int i = 0; i < thread; ++i) {
		// wait thread
		pthread_join(threadID[i], NULL);
	}
}

void HillClimbingDecoder::debug(string msg, int id) {
	pthread_mutex_lock(&updateMutex);

	cout << "Thread " << id << ": " << msg << endl;
	cout.flush();

	pthread_mutex_unlock(&updateMutex);
}

void HillClimbingDecoder::startTask(DependencyInstance* pred, DependencyInstance* gold, FeatureExtractor* fe) {
	this->pred = pred;
	this->gold = gold;
	this->fe = fe;

	initInst(this->pred, fe);

	bestScore = -DBL_MAX;
	best.copyInfoFromInst(pred);
	unChangeIter = 0;

	for (int i = 0; i < thread; ++i) {
		// send start signal
		pthread_mutex_lock(&taskMutex[i]);

		assert(taskDone[i]);
		taskDone[i] = false;
		pthread_cond_signal(&taskStartCond[i]);

		pthread_mutex_unlock(&taskMutex[i]);
	}
}

void HillClimbingDecoder::waitAndGetResult(DependencyInstance* inst) {
	for (int i = 0; i < thread; ++i) {
		pthread_mutex_lock(&taskMutex[i]);
		while (!taskDone[i]) {
			 pthread_cond_wait(&taskDoneCond[i], &taskMutex[i] );
		}

		pthread_mutex_unlock(&taskMutex[i]);
	}

	best.loadInfoToInst(inst);
}

void HillClimbingDecoder::decode(DependencyInstance* inst, DependencyInstance* gold, FeatureExtractor* fe) {
	assert(fe->thread == thread);

	//double goldScore = fe->getScore(gold);

	startTask(inst, NULL, fe);
	waitAndGetResult(inst);
	inst->constructConversionList();
	inst->setOptSegPosCount();
	inst->buildChild();
/*
	cout << "aaa: score: " << bestScore << "\tgold score:" << goldScore << endl;

	FeatureVector fv;
	fe->pipe->createFeatureVector(inst, &fv);
	double predScore = fe->parameters->getScore(&fv);

	gold->fv.clear();
	fe->pipe->createFeatureVector(gold, &gold->fv);
	double goldScore2 = fe->parameters->getScore(&gold->fv);

	cout << "bbb: score: " << predScore << "\tgold score:" << goldScore2 << endl;

	if (abs(goldScore - goldScore2) > 1e-6) {
		for (int i = 1; i < gold->numWord; ++i) {
			cout << gold->word[i].currSegCandID;
			FeatureVector segfv;
			fe->pipe->createSegFeatureVector(gold, i, &segfv);
			double score = fe->parameters->getScore(&segfv);
			FeatureVector segfv2;
			fe->getSegFv(gold, i, &segfv2);
			double score2 = fe->parameters->getScore(&segfv2);
			cout << " " << score << " " << score2 << endl;
		}
		exit(0);
	}

	fv.clear();
	fe->getFv(inst, &fv);
	predScore = fe->parameters->getScore(&fv);

	fv.clear();
	fe->getFv(gold, &fv);
	goldScore = fe->parameters->getScore(&fv);

	cout << "ddd: score: " << predScore << "\tgold score:" << goldScore << endl;
	*/
}

void HillClimbingDecoder::train(DependencyInstance* gold, DependencyInstance* pred, FeatureExtractor* fe, int trainintIter) {
	assert(fe->thread == thread);

	startTask(pred, gold, fe);
	//startTask(pred, NULL, fe);
	waitAndGetResult(pred);
	pred->constructConversionList();
	pred->setOptSegPosCount();
	pred->buildChild();

	FeatureVector oldFV;
	fe->getFv(pred, &oldFV);
	double oldScore = fe->parameters->getScore(&oldFV);

	FeatureVector& newFV = gold->fv;
	double newScore = fe->parameters->getScore(&newFV);

	double err = 0.0;
	for (int i = 1; i < pred->numWord; ++i) {
		err += fe->parameters->wordError(gold->word[i], pred->word[i]);
	}
	if (newScore - oldScore < err) {
		FeatureVector diffFV;		// make a copy
		diffFV.concat(&newFV);
		diffFV.concatNeg(&oldFV);
		if (err - (newScore - oldScore) > 1e-4) {
			fe->parameters->update(gold, pred, &diffFV, err - (newScore - oldScore), fe, updateTimes);
		}
	}
	updateTimes++;
}

bool HillClimbingDecoder::findOptPos(DependencyInstance* pred, DependencyInstance* gold, HeadIndex& m, FeatureExtractor* fe, CacheTable* cache) {
	//if (options->heuristicDep && m.hSeg != pred->word[m.hWord].getCurrSeg().inNode) {
	//	return false;
	//}

	SegElement & ele = pred->getElement(m);
	if (ele.candPosNum() == 1)
		return false;

	double bestScore = fe->getPartialPosScore(pred, m, cache);

	if (gold) {
		// add loss
		bestScore += fe->parameters->wordError(gold->word[m.hWord], pred->word[m.hWord]);
	}
	int bestPos = ele.currPosCandID;
	int oldPos = ele.currPosCandID;

	for (int i = 0; i < ele.candPosNum(); ++i) {
		if (i == oldPos)
			continue;
		updatePos(pred->word[m.hWord], ele, i);
		double score = fe->getPartialPosScore(pred, m, cache);
		if (gold) {
			// add loss
			score += fe->parameters->wordError(gold->word[m.hWord], pred->word[m.hWord]);
		}

		if (score > bestScore + 1e-6) {
			bestScore = score;
			bestPos = i;
		}
	}
	updatePos(pred->word[m.hWord], ele, bestPos);
	return bestPos != oldPos;
}

bool HillClimbingDecoder::findOptHead(DependencyInstance* pred, DependencyInstance* gold, HeadIndex& m, FeatureExtractor* fe, CacheTable* cache) {
	if (options->heuristicDep && m.hSeg != pred->word[m.hWord].getCurrSeg().inNode) {
		return false;
	}

	// get cache table
	//CacheTable* cache = fe->getCacheTable(pred);
	assert(!cache || cache->numSeg == pred->getNumSeg());

	// get pruned list
	vector<bool> isPruned = move(fe->isPruned(pred, m, cache));
	int segID = -1;

	SegElement& predSegEle = pred->getElement(m);
	HeadIndex oldDep = predSegEle.dep;
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
			else {
				assert(hw != m.hWord || hs != m.hSeg);
			}

			HeadIndex h(hw, hs);
			if (isAncestor(pred, m, h))		// loop
				continue;

			if (options->proj && !isProj(pred, h, m))
				continue;

			HeadIndex oldH = predSegEle.dep;
			predSegEle.dep = h;
			pred->updateChildList(h, oldH, m);
			double score = fe->getPartialDepScore(pred, m, cache);
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
	HeadIndex oldH = predSegEle.dep;
	predSegEle.dep = bestDep;
	pred->updateChildList(bestDep, oldH, m);

	return bestDep != oldDep;
}

} /* namespace segparser */
