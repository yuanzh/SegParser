/*
 * HillClimbingDecoder.cpp
 *
 *  Created on: May 1, 2014
 *      Author: yuanz
 */

#include "HillClimbingDecoder.h"
#include <float.h>
#include "../util/Timer.h"

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
            data->debug("receive exit signal.", selfid);
			data->taskDone[selfid] = true;
			jobsDone = true;
		}

		pthread_mutex_unlock(&data->taskMutex[selfid]);

		if (jobsDone) {
			break;
		}

		DependencyInstance pred = *(data->pred);			// copy the instance
		DependencyInstance* gold = data->gold;
		FeatureExtractor* fe = data->fe;				// shared fe

		double goldScore = -DBL_MAX;
		if (gold) {
			goldScore = fe->parameters->getScore(&gold->fv);
		}

		Random r(data->options->seed + 2 + selfid);		// set different seed for different thread

		int maxIter = 100;

		//if (selfid == 0)
		//	cout << "start sampling" << endl;

		// begin sampling
		bool done = false;
		int iter = 0;
		double T = 0.25;
		for (iter = 0; iter < maxIter && !done; ++iter) {

			//if (selfid == 0)
			//	cout << "first order sampling 1" << endl;
			// sample seg/pos
			if (data->sampleSeg) {
				assert(pred.word[0].currSegCandID == 0);
				for (int i = 1; i < pred.numWord; ++i) {
					data->sampleSeg1O(&pred, gold, fe, i, r);
				}
				pred.constructConversionList();
			}

/*
			bool hitGold = true;
			if (gold) {
				for (int i = 1; i < pred.numWord; ++i) {
					if (pred.word[i].currSegCandID != gold->word[i].currSegCandID) {
						hitGold = false;
						break;
					}
				}
			}
*/
			if (data->samplePos) {
				for (int i = 1; i < pred.numWord; ++i) {
					data->samplePos1O(&pred, gold, fe, i, r);
				}
			}

			boost::shared_ptr<CacheTable> tmpCache = boost::shared_ptr<CacheTable>(new CacheTable());
			boost::shared_ptr<CacheTable> tmpSampleCache = boost::shared_ptr<CacheTable>(new CacheTable());

			CacheTable* sampleCache = fe->getCacheTable(&pred);
			if (!sampleCache) {
				sampleCache = tmpSampleCache.get();		// temporary cache for this run
				tmpSampleCache->initCacheTable(fe->type, &pred, fe->pfe.get(), data->options);
			}

			// sample a new tree from first order
			int len = pred.getNumSeg();
			vector<bool> toBeSampled(len);
			int id = 1;		// skip root
			for (int i = 1; i < pred.numWord; ++i) {
				SegInstance& segInst = pred.word[i].getCurrSeg();
				for (int j = 0; j < segInst.size(); ++j) {
					toBeSampled[id] = true;
					id++;
				}
			}
			assert(id == len);

			int miniConverge = 3;
			double currBestScore = -DBL_MAX;
			int miniUnchange = 0;
			int outloop = 0;
			VariableInfo copy;
			copy.copyInfoFromInst(&pred);
			VariableInfo currBest;

			while (miniUnchange < miniConverge && outloop < 100) {
				outloop++;

				copy.loadInfoToInst(&pred);
				pred.constructConversionList();
				pred.setOptSegPosCount();

				Timer ts;
				bool ok = data->randomWalkSampler(&pred, gold, fe, sampleCache, toBeSampled, r, T);
				if (!ok) {
					T *= 0.5;
					continue;
				}
				if (selfid == 0)
					data->sampleTime += ts.stop();

				pred.buildChild();

				int loop = 0;
	            bool change = true;
	            CacheTable* cache = sampleCache;
	            Timer tc;

	            /*
				pthread_mutex_lock(&data->updateMutex);
				data->totRuns++;
				pthread_mutex_unlock(&data->updateMutex);
				*/

				while (change && loop < 20) {
	            	change = false;
	            	loop++;

	            	// improve tree
	            	vector<HeadIndex> idx(len);
	            	HeadIndex root(0, 0);
	            	int id = data->getBottomUpOrder(&pred, root, idx, idx.size() - 1);
	            	assert(id == 0);

	            	for (unsigned int y = 1; y < idx.size(); ++y) {
	            		HeadIndex& m = idx[y];

	            		// find the optimum head (cost augmented)
	            		//double currScore = fe->getScore(&pred, cache);
	            		//if (gold) {
	            		// add loss
	            		//	currScore += fe->parameters->wordDepError(gold->word[m.hWord], pred.word[m.hWord]);
	            		//}
	            		double depChanged = data->findOptHead(&pred, gold, m, fe, cache);
	            		assert(depChanged > -1e-6);
	            		if (depChanged > 1e-6) {
	            			//double newScore = fe->getScore(&pred, cache);
	            			//if (gold) {
	            			// add loss
	            			//	newScore += fe->parameters->wordDepError(gold->word[m.hWord], pred.word[m.hWord]);
	            			//}
	            			//assert(abs(newScore - currScore - depChanged) < 1e-6);
	            			change = true;
	            		}

	            	}

	            	// improve bigram
	            	for (unsigned int i = 0; i < idx.size(); ++i) idx[i] = HeadIndex();
	            	id = data->getBottomUpOrder(&pred, root, idx, idx.size() - 1);
	            	assert(id == 0);

	            	for (unsigned int y = 1; y < idx.size(); ++y) {
	            		HeadIndex& m = idx[y];

	            		int mIndex = pred.wordToSeg(m);
	            		if (mIndex + 1 >= pred.getNumSeg()) {
	            			continue;
	            		}

	            		HeadIndex n = pred.segToWord(mIndex + 1);
	            		if (pred.getElement(m).dep != pred.getElement(n).dep) {
	            			// not same head
	            			continue;
	            		}

	            		// find the optimum head (cost augmented)
	            		//double currScore = fe->getScore(&pred, cache);
	            		//if (gold) {
	            		// add loss
	            		//	for (int i = 1; i < pred.numWord; ++i)
	            		//		currScore += fe->parameters->wordDepError(gold->word[i], pred.word[i]);
	            		//}
	            		double depChanged = data->findOptBigramHead(&pred, gold, m, n, fe, cache);
	            		assert(depChanged > -1e-6);
	            		if (depChanged > 1e-6) {
	            			//double newScore = fe->getScore(&pred, cache);
	            			//if (gold) {
	            			// add loss
	            			//	for (int i = 1; i < pred.numWord; ++i)
	            			//		newScore += fe->parameters->wordDepError(gold->word[i], pred.word[i]);
	            			//}
	            			//assert(abs(newScore - currScore - depChanged) < 1e-6);
	            			change = true;
	            		}
	            	}

	                // improve pos
	                if (data->samplePos) {
	                	vector<HeadIndex> idx(len);
	                	HeadIndex root(0, 0);
	                	int id = data->getBottomUpOrder(&pred, root, idx, idx.size() - 1);
	                	assert(id == 0);

	                	for (unsigned int y = 1; y < idx.size(); ++y) {
	                		HeadIndex& m = idx[y];
	       	     			//double currScore = fe->getScore(&pred);
	       	     			//if (gold) {
	       	     			//	currScore += fe->parameters->wordError(gold->word[m.hWord], pred.word[m.hWord]);
	       	     			//}
	                		double posChanged = data->findOptPos(&pred, gold, m, fe, cache);
	                		assert(posChanged > -1e-6);
	                		if (posChanged > 1e-6) {
	                			// update cache table
	                			cache = fe->getCacheTable(&pred);
	       	   	     			//double newScore = fe->getScore(&pred);
	           	     			//if (gold) {
	           	     			//	newScore += fe->parameters->wordError(gold->word[m.hWord], pred.word[m.hWord]);
	           	     			//}
	       	   	     			//assert(abs(newScore - currScore - posChanged) < 1e-6);
	                			change = true;
	                		}
	                	}
	                }

	    			if (!cache) {
	    				tmpCache = boost::shared_ptr<CacheTable>(new CacheTable());
	    				cache = tmpCache.get();		// temporary cache for this run
	    				tmpCache->initCacheTable(fe->type, &pred, fe->pfe.get(), data->options);
	    			}
	            }

/*
	            if (gold && hitGold) {
	                pthread_mutex_lock(&data->updateMutex);

	                data->hitGoldSegCount++;
					bool hitGoldPos = true;
					for (int i = 1; i < pred.numWord; ++i) {
						for (int j = 0; j < pred.word[i].getCurrSeg().size(); ++j) {
							if (pred.word[i].getCurrSeg().element[j].currPosCandID != gold->word[i].getCurrSeg().element[j].currPosCandID) {
								hitGoldPos = false;
								break;
							}
						}
					}
					if (hitGoldPos) {
						data->hitGoldSegPosCount++;
					}

					pthread_mutex_unlock(&data->updateMutex);

	            }
*/

				if (selfid == 0)
					data->climbTime += tc.stop();

	            if (loop >= 20) {
	            	cout << "Warning: many loops" << endl;
	            }

	            assert(cache->numSeg == pred.getNumSeg());
				double currScore = fe->getScore(&pred, cache);
				if (gold) {
					for (int i = 1; i < pred.numWord; ++i)
						currScore += fe->parameters->wordDepError(gold->word[i], pred.word[i]);
				}

				if (currScore > currBestScore + 1e-6) {
					currBestScore = currScore;
					currBest.copyInfoFromInst(&pred);
					miniUnchange = 0;
				}
				else {
					miniUnchange++;
				}
			}

            if (outloop >= 100) {
            	cout << "Warning: many out loops" << endl;
            }
    		//if (selfid == 0)
    		//	cout << "out loop: " << outloop << endl;

			pthread_mutex_lock(&data->updateMutex);

			if (data->unChangeIter >= data->convergeIter)
				done = true;
			else if (gold && data->unChangeIter >= data->earlyStopIter && data->bestScore >= goldScore - 1e-6) {
				// early stop
				done = true;
			}

			if (currBestScore > data->bestScore + 1e-6) {
				data->bestScore = currBestScore;
				currBest.loadInfoToInst(&pred);
				data->best.copyInfoFromInst(&pred);
				if (!done) {
					if (gold && data->unChangeIter >= data->earlyStopIter && data->bestScore >= goldScore - 1e-6) {
						cout << " (" << data->unChangeIter << ") ";
					}
					data->unChangeIter = 0;
				}
			}
			else {
				data->unChangeIter++;
			}

			pthread_mutex_unlock(&data->updateMutex);
		}

		//if (selfid == 0)
		//	cout << "finish sampling. iter: " << iter << endl;

		pthread_mutex_lock(&data->taskMutex[selfid]);

		data->taskDone[selfid] = true;
		pthread_cond_signal(&data->taskDoneCond[selfid]);

		pthread_mutex_unlock(&data->taskMutex[selfid]);
	}

    data->debug("exit.", selfid);

    pthread_exit(NULL);
	return NULL;
}

HillClimbingDecoder::HillClimbingDecoder(Options* options, int thread, int convergeIter) : DependencyDecoder(options), thread(thread), convergeIter(convergeIter) {
	cout << "converge iter: " << convergeIter << endl;
	earlyStopIter = 5;
    samplePos = true;
    sampleSeg = true;
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

    if (samplePos || sampleSeg)
    	initInst(this->pred, fe);

	bestScore = -DBL_MAX;
	best.copyInfoFromInst(pred);
	unChangeIter = 0;

	hitGoldSegCount = 0;
	hitGoldSegPosCount = 0;
	totRuns = 0;

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

	if (gold && unChangeIter > earlyStopIter + thread) {
		cout << " (" << unChangeIter << ") ";
		cout.flush();
	}

	best.loadInfoToInst(inst);

	//cout << "hit gold seg: " << hitGoldSegCount << ", hit gold seg and pos: " << hitGoldSegPosCount << ", total runs: " << totRuns << endl;
}

void HillClimbingDecoder::decode(DependencyInstance* inst, DependencyInstance* gold, FeatureExtractor* fe) {
	assert(fe->thread == thread);

	double goldScore = fe->getScore(gold);

	startTask(inst, NULL, fe);
	waitAndGetResult(inst);
	inst->constructConversionList();
	inst->setOptSegPosCount();
	inst->buildChild();

	cout << "aaa: score: " << bestScore << "\tgold score:" << goldScore << " " << (bestScore >= goldScore - 1e-6 ? 1 : 0) << endl;

	//FeatureVector fv;
	//fe->pipe->createFeatureVector(inst, &fv);
	//double predScore = fe->parameters->getScore(&fv);
	/*
	double predSegScore = 0.0;
	double predPosScore = 0.0;
	CacheTable* cache = fe->getCacheTable(inst);
	for (int i = 1; i < inst->numWord; ++i) {
		predSegScore += fe->getSegScore(inst, i);
		for (int j = 0; j < inst->word[i].getCurrSeg().size(); ++j) {
			HeadIndex m(i, j);
			predPosScore += fe->getPos1OScore(inst, m);
			predPosScore += fe->getPosHOScore(fe, inst, m, cache);
		}
	}*/

	FeatureVector fv2;
	fe->pipe->createFeatureVector(gold, &fv2);
	double goldScore2 = fe->parameters->getScore(&fv2);
	/*
	double goldSegScore = 0.0;
	double goldPosScore = 0.0;
	cache = fe->getCacheTable(gold);
	for (int i = 1; i < gold->numWord; ++i) {
		goldSegScore += fe->getSegScore(gold, i);
		for (int j = 0; j < gold->word[i].getCurrSeg().size(); ++j) {
			HeadIndex m(i, j);
			goldPosScore += fe->getPos1OScore(gold, m);
			goldPosScore += fe->getPosHOScore(fe, gold, m, cache);
		}
	}*/

	//cout << "bbb: score: " << predScore << "\tgold score:" << goldScore2 << endl;
	//cout << "bbb: " << predSegScore << " " << goldSegScore << " " << predPosScore << " " << goldPosScore << endl;

	if (abs(goldScore - goldScore2) > 1e-6) {
		CacheTable* cache = fe->getCacheTable(gold);
		for (int i = 1; i < gold->numWord; ++i) {
			SegInstance& segInst = gold->word[i].getCurrSeg();
			for (int j = 0; j < segInst.size(); ++j) {
				HeadIndex m(i, j);
				cout << m;
				FeatureVector segfv;
				fe->pipe->createPos1OFeatureVector(gold, m, &segfv);
				fe->pipe->createPosHOFeatureVector(gold, m, false, &segfv);
				double score = fe->parameters->getScore(&segfv);
				FeatureVector segfv2;
				fe->getPos1OFv(gold, m, &segfv2);
				fe->getPosHOFv(fe, gold, m, &segfv2, cache);
				double score2 = fe->parameters->getScore(&segfv2);
				cout << " " << score << " " << score2 << endl;

				if (abs(score - score2) > 1e-6) {
					segfv.output();
					segfv2.output();

					SegElement& ele = segInst.element[j];
					cout << ele.st << " " << ele.en << endl;
					for (unsigned int k = 0; k < gold->characterid.size(); ++k) {
						cout << gold->characterid[k] << " ";
					}
					cout << endl;
				}
			}
		}
		exit(0);
	}

    if (bestScore < goldScore - 1e-6) {
    	//pred->output();
    	//cout << "-------------------------" << endl;
    	//gold->output();
    	//cout << "--------------------------" << endl;
/*
    	while (true) {
    		string cmd;
    		cin >> cmd;
    		if (cmd == "pos") {
    			int w, s, id;
    			cin >> w >> s >> id;
        		inst->word[w].getCurrSeg().element[s].currPosCandID = id;
        		inst->setOptSegPosCount();
        		cout << "score: " << fe->getScore(inst) << endl;
    		}
    		else if (cmd == "dep"){
    			int w1, s1, w2, s2;
    			cin >> w1 >> s1 >> w2 >> s2;
    			inst->word[w1].getCurrSeg().element[s1].dep = HeadIndex(w2, s2);
        		inst->buildChild();
        		cout << "score: " << fe->getScore(inst) << endl;
    		}
    		else {
    			break;
    		}
    	}
*/
    	failTime++;
    }

    /*
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

    if (oldScore + err < newScore - 1e-6) {
    	cout << oldScore + err << " " << newScore << endl;
/*
		long code = fe->pipe->fe->genCodePF(HighOrder::SEG_PROB, 0);
		int index = fe->pipe->dataAlphabet->lookupIndex(TemplateType::THighOrder, code, false);
		cout << "seg weight: " << fe->parameters->parameters[index] << endl;
    	for (int i = 1; i < pred->numWord; ++i) {
    		cout << i << ":";
    		int old = pred->word[i].currSegCandID;
			for (unsigned int j = 0; j < pred->word[i].candSeg.size(); ++j) {
				pred->word[i].currSegCandID = j;
				cout << "_" << fe->getSegScore(pred, i);
			}
			pred->word[i].currSegCandID = old;
			cout << "\t";
    	}
    	cout << endl;
*/
    	//pred->output();
    	//gold->output();

    	cout << "use gold seg and pos" << endl;
        setGoldSegAndPos(pred, gold);

        samplePos = false;
        sampleSeg = false;

        startTask(pred, gold, fe);
        waitAndGetResult(pred);
        pred->constructConversionList();
        pred->setOptSegPosCount();
        pred->buildChild();

        oldFV.clear();
        fe->getFv(pred, &oldFV);
        oldScore = fe->parameters->getScore(&oldFV);

    	err = 0.0;
    	for (int i = 1; i < pred->numWord; ++i) {
    		err += fe->parameters->wordError(gold->word[i], pred->word[i]);
    	}

    	cout << "result: " << oldScore + err << " " << newScore << " " << unChangeIter << endl;

    	samplePos = true;
        sampleSeg = true;

        if (oldScore + err < newScore - 1e-6) {
        	failTime++;
        }
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

double HillClimbingDecoder::findOptPos(DependencyInstance* pred, DependencyInstance* gold, HeadIndex& m, FeatureExtractor* fe, CacheTable* cache) {
	//if (options->heuristicDep && m.hSeg != pred->word[m.hWord].getCurrSeg().inNode) {
	//	return false;
	//}

	SegElement & ele = pred->getElement(m);
	if (ele.candPosNum() == 1)
		return 0.0;

	double bestScore = fe->getPartialPosScore(pred, m, cache);

	if (gold) {
		// add loss
		bestScore += fe->parameters->wordError(gold->word[m.hWord], pred->word[m.hWord]);
	}
	double oldScore = bestScore;

	int bestPos = ele.currPosCandID;
	int oldPos = ele.currPosCandID;

	for (int i = 0; i < ele.candPosNum(); ++i) {
		if (i == oldPos)
			continue;

		if (ele.candProb[i] < -15.0)
			continue;

		updatePos(pred->word[m.hWord], ele, i);
		cache = fe->getCacheTable(pred);
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
	assert(bestScore - oldScore > 1e-6 || bestPos == oldPos);
	return bestScore - oldScore;
	//return (bestPos != oldPos ? 1.0 : 0.0);
}

double HillClimbingDecoder::findOptHead(DependencyInstance* pred, DependencyInstance* gold, HeadIndex& m, FeatureExtractor* fe, CacheTable* cache) {
	// return score difference

	if (options->heuristicDep && m.hSeg != pred->word[m.hWord].getCurrSeg().inNode) {
		return 0.0;
	}

	// get cache table
	//CacheTable* cache = fe->getCacheTable(pred);
	assert(!cache || cache->numSeg == pred->getNumSeg());

	// get pruned list
	vector<bool> isPruned = move(fe->isPruned(pred, m, cache));
	int segID = -1;

	SegElement& predSegEle = pred->getElement(m);
	HeadIndex oldDep = predSegEle.dep;
	HeadIndex bestDep = predSegEle.dep;
	double bestScore = fe->getPartialDepScore(pred, m, cache);
	if (gold) {
		// add loss
		//bestScore += fe->parameters->wordError(gold->word[m.hWord], pred->word[m.hWord]);
		bestScore += fe->parameters->wordDepError(gold->word[m.hWord], pred->word[m.hWord]);
	}
	double oldScore = bestScore;

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

			if (h == oldDep)
				continue;

			HeadIndex oldH = predSegEle.dep;
			predSegEle.dep = h;
			pred->updateChildList(h, oldH, m);
			double score = fe->getPartialDepScore(pred, m, cache);
			//if (!gold && pred->numWord == 12 && m.hWord == 5 && m.hSeg == 0
			//		&& (hw == 8 || hw == 10)) {
			//	cout << "score: h = " << hw << " " << score << endl;
			//}
			if (gold) {
				// add loss
				//score += fe->parameters->wordError(gold->word[m.hWord], pred->word[m.hWord]);
				score += fe->parameters->wordDepError(gold->word[m.hWord], pred->word[m.hWord]);
			}

			if (score > bestScore + 1e-6) {
				bestScore = score;
				bestDep = h;
			}
		}
	}
	assert(segID == (int)isPruned.size() - 1);

	//if (bestDep.hWord != -1) {
		HeadIndex oldH = predSegEle.dep;
		predSegEle.dep = bestDep;
		pred->updateChildList(bestDep, oldH, m);
	//}
	//else {
		// can't find one in pruning space. keep the old one
		// cout << "Warning: not in pruning space" << endl;
		//bestDep = oldDep;
		//HeadIndex oldH = predSegEle.dep;
		//predSegEle.dep = bestDep;
		//pred->updateChildList(bestDep, oldH, m);
	//}

	assert(bestScore - oldScore > 1e-6 || bestDep == oldDep);
	return bestScore - oldScore;
	//return (bestDep != oldDep ? 1.0 : 0.0);
}

double HillClimbingDecoder::findOptBigramHead(DependencyInstance* pred, DependencyInstance* gold, HeadIndex& m, HeadIndex& n, FeatureExtractor* fe, CacheTable* cache) {
	// return score difference

	if (options->heuristicDep) {
		cout << "Warning: heuristic dep mode not support" << endl;
		return 0.0;
	}

	assert(!cache || cache->numSeg == pred->getNumSeg());

	// get pruned list
	vector<bool> mPruned = move(fe->isPruned(pred, m, cache));
	vector<bool> nPruned = move(fe->isPruned(pred, n, cache));
	assert(mPruned.size() == nPruned.size());

	int segID = -1;

	SegElement& mEle = pred->getElement(m);
	SegElement& nEle = pred->getElement(n);

	HeadIndex oldDep = mEle.dep;
	HeadIndex bestDep = mEle.dep;
	double bestScore = fe->getPartialBigramDepScore(pred, m, n, cache);
	//double bestScore = fe->getScore(pred, cache);

	if (gold) {
		// add loss
		if (m.hWord != n.hWord) {
			bestScore += fe->parameters->wordDepError(gold->word[m.hWord], pred->word[m.hWord])
					 + fe->parameters->wordDepError(gold->word[n.hWord], pred->word[n.hWord]);
		}
		else {
			bestScore += fe->parameters->wordDepError(gold->word[m.hWord], pred->word[m.hWord]);
		}
	}
	double oldScore = bestScore;

	for (int hw = 0; hw < pred->numWord; ++hw) {
		SegInstance& segInst = pred->word[hw].getCurrSeg();

		for (int hs = 0; hs < segInst.size(); ++hs) {
			segID++;
			if (mPruned[segID] || nPruned[segID]) {
				continue;
			}

			if (options->heuristicDep)
				assert(hw != m.hWord && hs == segInst.outNode);
			else {
				assert(hw != m.hWord || hs != m.hSeg);
			}

			HeadIndex h(hw, hs);
			if (isAncestor(pred, m, h) || isAncestor(pred, n, h))		// loop
				continue;

			//if (options->proj && !isProj(pred, h, m))
			//	continue;

			if (h == oldDep)
				continue;

			HeadIndex oldH = mEle.dep;
			mEle.dep = h;
			pred->updateChildList(h, oldH, m);
			nEle.dep = h;
			pred->updateChildList(h, oldH, n);
			double score = fe->getPartialBigramDepScore(pred, m, n, cache);

			if (gold) {
				// add loss
				if (m.hWord != n.hWord) {
					score += fe->parameters->wordDepError(gold->word[m.hWord], pred->word[m.hWord])
							 + fe->parameters->wordDepError(gold->word[n.hWord], pred->word[n.hWord]);
				}
				else {
					score += fe->parameters->wordDepError(gold->word[m.hWord], pred->word[m.hWord]);
				}
			}

			if (score > bestScore + 1e-6) {
				bestScore = score;
				bestDep = h;
			}
		}
	}
	assert(segID == (int)mPruned.size() - 1);

	HeadIndex oldH = mEle.dep;
	mEle.dep = bestDep;
	pred->updateChildList(bestDep, oldH, m);
	nEle.dep = bestDep;
	pred->updateChildList(bestDep, oldH, n);

	assert(bestScore - oldScore > 1e-6 || bestDep == oldDep);
	return bestScore - oldScore;
	//return (bestDep != oldDep ? 1.0 : 0.0);
}

double HillClimbingDecoder::findOptSeg(DependencyInstance* pred, DependencyInstance* gold, HeadIndex& m, FeatureExtractor* fe, CacheTable* cache) {
	WordInstance& word = pred->word[m.hWord];
	if (word.candSeg.size() == 1)
		return 0.0;

	// hard to define the partial score here
	double bestScore = fe->getScore(pred, cache);

	//if (gold) {
		// add loss
	//	bestScore += fe->parameters->wordError(gold->word[m.hWord], pred->word[m.hWord]);
	//}
	double oldScore = bestScore;

	int bestSeg = word.currSegCandID;
	int oldSeg = word.currSegCandID;

	int baseOptSeg = pred->optSegCount - (oldSeg == 0 ? 1 : 0);
	int baseOptPos = word.optPosCount;
	vector<HeadIndex> oldHeadIndex(word.getCurrSeg().size());
	vector<int> oldPos(word.getCurrSeg().size());
	for (int i = 0; i < word.getCurrSeg().size(); ++i) {
		oldHeadIndex[i] = word.getCurrSeg().element[i].dep;
		oldPos[i] = word.getCurrSeg().element[i].getCurrPos();
		baseOptPos -= (word.getCurrSeg().element[i].currPosCandID == 0 ? 1 : 0);
	}

	vector<HeadIndex> relatedChildren;
	vector<int> relatedOldParent;

	for (int i = 1; i < pred->numWord; ++i) {
		if (i == m.hWord)
			continue;

		for (int j = 0; j < pred->word[j].getCurrSeg().size(); ++j) {
			if (pred->word[j].getCurrSeg().element[j].dep.hWord == m.hWord) {
				relatedChildren.push_back(HeadIndex(i, j));
				relatedOldParent.push_back(pred->word[j].getCurrSeg().element[j].dep.hSeg);
			}
		}
	}

	for (int i = 0; i < (int)word.candSeg.size(); ++i) {
		if (i == oldSeg)
			continue;

		// update segment
		updateSeg(pred, gold, m, i, oldSeg, baseOptSeg, baseOptPos, oldPos, oldHeadIndex, relatedChildren, relatedOldParent);

		cache = fe->getCacheTable(pred);
		double score = fe->getScore(pred, cache);
		//if (gold) {
			// add loss
		//	score += fe->parameters->wordError(gold->word[m.hWord], pred->word[m.hWord]);
		//}

		if (score > bestScore + 1e-6) {
			bestScore = score;
			bestSeg = i;
		}
	}

	updateSeg(pred, gold, m, bestSeg, oldSeg, baseOptSeg, baseOptPos, oldPos, oldHeadIndex, relatedChildren, relatedOldParent);

	assert(bestScore - oldScore > 1e-6 || bestSeg == oldSeg);
	return bestScore - oldScore;
}

} /* namespace segparser */
