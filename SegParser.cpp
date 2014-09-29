/*
 * SegParser.cpp
 *
 *  Created on: Mar 19, 2014
 *      Author: yuanz
 */

#include "SegParser.h"
#include <pthread.h>
#include "util/Random.h"
#include <float.h>
#include "util/Timer.h"
#include <fstream>
#include "util/SerializationUtils.h"
#include <set>

namespace segparser {

void* testFunc(void* instance) {
	cout << "lalala" << endl;

	Random r(0);
	SegParser* sp = (SegParser*)instance;

	for (int i = 0; i < 300000; ++i) {
		//sp->a = a;
		//assert(sp->a->a == 2);

		int x = r.nextInt(100);
		int y = r.nextInt(100);
		int id = x * 100 + y;

		//int score = -1;
		test_ptr ptr = atomic_load(&(*sp->ptr)[id]);

		if (!ptr) {
			//ptr = test_ptr();
			//TestClass* y = new TestClass();
			//assert(y->a == 2);
			//pthread_mutex_lock(&sp->mutex);
			//(*sp->ptr)[id].reset(y);
			ptr = test_ptr(new TestClass());
			atomic_store(&(*sp->ptr)[id], ptr);
			//assert(((TestClass*)(*sp->ptr)[id].get())->a == 2);
			//pthread_mutex_unlock(&sp->mutex);
		}

		ptr = atomic_load(&(*sp->ptr)[id]);
		TestClass* tmp2 = ptr.get();

		if (tmp2->a != 2) {
			cout << tmp2->a << endl;
		}
		//assert(2 == tmp2->a);

	}

	pthread_exit(NULL);
	return NULL;
}

double getValue() {
	cout << "klalal" << endl;
	return 1.0;
}

void test1() {
	vector<test_ptr> ptr;
	//multi_array<TestClass*, 2> ptr;
	//multi_array<int, 2> score;
/*
	ptr.resize(10000, test_ptr((TestClass*)NULL));
	//score.resize(extents[100][100]);

	pthread_t thread[10];
	SegParser* sp = new SegParser();
	sp->ptr = &ptr;
	//sp->score = &score;

	for (int i = 0; i < 10; ++i) {
		int rc = pthread_create(&thread[i], NULL, testFunc, (void*)sp);
		if (rc) {
			cout << i << " " << rc << " " << "Create train thread failed." << endl;
			exit(0);
		}
	}

	for (int i = 0; i < 10; ++i) {
		// wait thread
		pthread_join(thread[i], NULL);
	}
	*/

	/*
	test_ptr p1 = test_ptr(new TestClass());
	test_ptr p2 = p1;
	p1 = test_ptr(new TestClass());
	cout << p1.use_count() << " " << p2.use_count() << endl;

	TestClass* a = new TestClass();
	TestClass b;
	a->a = 3;
	b = *a;
	a->a = 2;
	*a = b;
	cout << a->a << endl;

	bool flag = a->a != 3;
	double value = flag ? 0.0 : getValue();
	cout << value << endl;
	*/

	wstring str;
	wifstream wfin("test.txt");
	cout << wfin.good() << endl;
	getline(wfin, str);
	wcout << str << endl;
	cout << str.length() << endl;
}

vector<double> evaluate(string act_file, string pred_file, Options* options) {
	// TODO: evaluation

	vector<double> ret(2);

	ret[0] = 0.0;
	ret[1] = 0.0;

	return ret;

}

SegParser::SegParser(DependencyPipe* pipe, Options* options)
	: pipe(pipe), options(options) {
	// Set up arrays
	parameters = new Parameters(pipe->dataAlphabet->size(), options);
	devParams = new Parameters(pipe->dataAlphabet->size(), options);
	pruner = NULL;
	if (options->train) {
		decoder = DependencyDecoder::createDependencyDecoder(options, options->learningMode, options->trainThread, true);
		decoder->initialize();
	}
	dt = new DevelopmentThread();
	FeatureVector::initVec(pipe->dataAlphabet->size());

	bestLabAcc = -DBL_MAX;
	bestUlabAcc = -DBL_MAX;
}

void SegParser::closeDecoder() {
	if (decoder)
		decoder->shutdown();
}

SegParser::~SegParser() {
	delete parameters;
	delete devParams;
	delete decoder;
	delete dt;

	delete pruner;
}

void SegParser::train(vector<inst_ptr>& il) {

	cout << "About to train" << endl;
	//cout << "Num Feats: " << pipe->dataAlphabet->size() - 1 << endl;

	devTimes = 0;

	// construct pred instance list
	vector<inst_ptr> pred(il.size());
	for (unsigned int i = 0; i < il.size(); ++i) {
		pred[i] = inst_ptr(new DependencyInstance());
		*(pred[i].get()) = *(il[i].get());
	}

	for(int i = 0; i < options->numIters; ++i) {

		cout << "========================" << endl;
		cout << "Iteration: " << i << endl;
		cout << "========================" << endl;
		cout << "Processed: ";
		cout.flush();

		Timer timer;

		trainingIter(il, pred, i+1);

		double diff = timer.stop();
		cout << "Training iter took: " << diff / 1000 << " secs." << endl;

	}

	parameters->averageParams(decoder->getUpdateTimes());

	// wait until dev finish
	if (options->test) {
		if (dt->isDevTesting)
			pthread_join(dt->workThread, NULL);

		if (devTimes > 0) {
			// get last dev accuracy
			//string devfile = options->testFile;
			//string outfile = devfile + ".res";

			//evaluate(devfile, outfile, options);
		}
	}
}

void SegParser::trainingIter(vector<inst_ptr>& goldList, vector<inst_ptr>& predList, int iter) {

	int totalWordNum = 0;
	int correctSegNum = 0;
	int totalSegNum = 0;
	int correctDepNum = 0;

	decoder->failTime = 0;
	decoder->sampleTime = 0.0;
	decoder->climbTime = 0.0;

	for(unsigned int i = 0; i < goldList.size(); ++i) {
		if((i+1) % 50 == 0) {
			cout << "  " << (i+1) <<
					" (T_s=" << (int)(decoder->sampleTime / 1000) << "s, " <<
					" T_c=" << (int)(decoder->climbTime / 1000) << "s)";
			cout.flush();
		}

		inst_ptr gold = goldList[i];
		inst_ptr pred = predList[i];

		FeatureExtractor fe(pred.get(), this, parameters, options->trainThread);

		string str;

		//if (pred->numWord - 1 > options->maxLength)
		//	continue;

		//gold->fv.clear();
		//pipe->createFeatureVector(gold.get(), &gold->fv);
		assert(gold->fv.binaryIndex.size() > 0);

		decoder->train(gold.get(), pred.get(), &fe, iter);

		// train accuracy
		totalWordNum += gold->numWord - 1;
		bool allSegCorrect = true;
		for (int j = 1; j < pred->numWord; j++) {
			if (pred->word[j].currSegCandID == gold->word[j].currSegCandID) {
				correctSegNum++;
			}
			else {
				allSegCorrect = false;
			}
		}
		if (allSegCorrect) {
			for (int j = 1; j < pred->numWord; j++) {
				SegInstance& gsi = gold->word[j].getCurrSeg();
				SegInstance& psi = pred->word[j].getCurrSeg();

				totalSegNum += gsi.size();
				for (int k = 0; k < gsi.size(); k++) {
					if (gsi.element[k].dep == psi.element[k].dep) {
						correctDepNum++;
					}
				}
			}
		}

	}

	cout << endl;

	cout << "  " << goldList.size() << " instances" << endl;
	cout << "Training seg accuracy: " << (double)correctSegNum / totalWordNum << endl;
	cout << "Training dep accuracy: " << (double)correctDepNum / totalSegNum << endl;
	cout << "Fail time: " << decoder->failTime << endl;

	if (options->test)
		checkDevStatus(iter);

	//resetParams();
}

void SegParser::resetParams() {
	parameters->averageParams(decoder->getUpdateTimes());
	for (unsigned int j = 0; j < parameters->total.size(); ++j)
		parameters->total[j] = 0;
	decoder->resetUpdateTimes();
}

void SegParser::checkDevStatus(int iter) {
	if (dt->isDevTesting) {
		cout << "processing sentences: ";

		pthread_mutex_lock(&dt->finishMutex);
		cout << dt->currFinishID << " ";
		pthread_mutex_unlock(&dt->finishMutex);

		pthread_mutex_lock(&dt->processMutex);
		cout << dt->currProcessID << endl;
		pthread_mutex_unlock(&dt->processMutex);
	}
	else {
		cout << "dev and test done." << endl;

		if (devTimes > 0) {
			// get last dev accuracy
    		string devfile = options->testFile;
    		string outfile = devfile + ".res";

    	    evaluate(devfile, outfile, options);
		}

		// start new thread for dev
		string devfile = options->testFile;
		string devoutfile = devfile + ".res";

		cout << "build dev params" << endl;
		devParams->copyParams(parameters);
		devParams->averageParams(decoder->getUpdateTimes());

		if (options->useHO){
			// check non proj weight
			long code = pipe->fe->genCodePF(HighOrder::NP, 1);
			int index = pipe->dataAlphabet->lookupIndex(TemplateType::THighOrder, code, false);
			assert(index > 0);
			cout << "proj weight: " << parameters->parameters[index] << endl;
		}

		if (options->useSP) {
			long code = pipe->fe->genCodePF(HighOrder::SEG_PROB, 0);
			int index = pipe->dataAlphabet->lookupIndex(TemplateType::THighOrder, code, false);
			assert(index > 0);
			cout << "seg weight: " << parameters->parameters[index] << endl;
			cout << "dev seg weight: " << devParams->parameters[index] << endl;

			code = pipe->fe->genCodePF(HighOrder::POS_PROB, 1);
			index = pipe->dataAlphabet->lookupIndex(TemplateType::THighOrder, code, false);
			//assert(index > 0);
			cout << "pos weight: " << parameters->parameters[index] << endl;
			cout << "dev pos weight: " << devParams->parameters[index] << endl;
		}
		cout << "Saving model ... ";
	    cout.flush();
	    if (pruner) {
	    	pruner->saveModel(options->modelName + ".pruner", pruner->parameters);
	    }
	    saveModel(options->modelName, devParams);
	    saveModel(options->modelName + ".train", parameters);
	    cout << "done." << endl;

	    cout << "start new dev " << devTimes << endl;
		dt->start(devfile, devoutfile, this, false);
		devTimes++;
	}
}

///////////////////////////////////////////////////////
// Saving and loading models
///////////////////////////////////////////////////////
void SegParser::outputWeight(ofstream& fout, int type, Parameters* params) {
	unordered_map<uint64_t, int>* intmap = pipe->dataAlphabet->getMap(type);
	for (auto kv : (*intmap)) {
		uint64_t s = kv.first;
		int index = kv.second;
		if (index > 0) {
			fout << s << "\t" << parameters->parameters[index] << "\t" << parameters->total[index] << endl;
		}
	}
}

void SegParser::outputWeight(string fStr) {
	cout << "output feature weight to " << fStr << endl;
	ofstream fout(fStr.c_str());

	outputWeight(fout, TemplateType::TArc, parameters);

	fout.close();
}

void SegParser::saveModel(string file, Parameters* params) {
	FILE *fs = fopen(file.c_str(), "wb");
	params->writeParams(fs);
	pipe->dataAlphabet->writeObject(fs);
	pipe->typeAlphabet->writeObject(fs);
	pipe->posAlphabet->writeObject(fs);
	pipe->lexAlphabet->writeObject(fs);
	fclose(fs);
}

void SegParser::loadModel(string file) {
	FILE *fs = fopen(file.c_str(), "rb");
	parameters->readParams(fs);
	pipe->dataAlphabet->readObject(fs);
	pipe->typeAlphabet->readObject(fs);
	pipe->posAlphabet->readObject(fs);
	pipe->lexAlphabet->readObject(fs);
	fclose(fs);

	pipe->closeAlphabets();
	pipe->setAndCheckOffset();

	parameters->total.clear();
	parameters->total.resize(parameters->parameters.size());
	//parameters->tmpParams.clear();
	//parameters->tmpParams.resize(parameters->parameters.size());
	parameters->size = parameters->parameters.size();

	if (options->useSP) {
		long code = pipe->fe->genCodePF(HighOrder::SEG_PROB, 0);
		int index = pipe->dataAlphabet->lookupIndex(TemplateType::THighOrder, code, false);
		assert(index > 0);
		cout << "seg weight: " << parameters->parameters[index] << endl;

		code = pipe->fe->genCodePF(HighOrder::POS_PROB, 1);
		index = pipe->dataAlphabet->lookupIndex(TemplateType::THighOrder, code, false);
		//assert(index > 0);
		cout << "pos weight: " << parameters->parameters[index] << endl;
	}
/*
	set<int> usedTemp;
	long mask = 1;
	mask = (mask << pipe->fe->tempOff) - 1;
	for (unordered_map<uint64_t, int>::iterator it = pipe->dataAlphabet->highOrderMap.begin(); it != pipe->dataAlphabet->highOrderMap.end(); ++it) {
		int temp = it->first & mask;
		usedTemp.insert(temp);
	}

	for (set<int>::iterator it = usedTemp.begin(); it != usedTemp.end(); ++it) {
		cout << *it << " ";
	}
	cout << endl;
	int x;
	cin >> x;
	*/
}

void SegParser::evaluatePruning() {
	cout << "Evaluate pruning quality..." << endl;
	DependencyReader reader(options, options->testFile);
	inst_ptr gold = reader.nextInstance();

	int numSeg = 0;
	double oracle = 0.0;

	while(gold) {
		gold->setInstIds(pipe, options);
		DependencyInstance pred;
		pred = *(gold.get());

		PrunerFeatureExtractor pfe;
		pfe.init(&pred, this, 1);

		for (int i = 1; i < pred.numWord; ++i) {
			WordInstance& word = pred.word[i];
			for (int j = 0; j < word.getCurrSeg().size(); ++j) {
				numSeg++;
				vector<bool> tmpPruned;
				HeadIndex m(i, j);
				pfe.prune(&pred, m, tmpPruned);

				HeadIndex& goldDep = gold->getElement(i, j).dep;
				int goldDepIndex = gold->wordToSeg(goldDep);

				vector<bool> pruned;
				int p = 0;
				for (int hw = 0; hw < pred.numWord; ++hw) {
					SegInstance& headSeg = pred.word[hw].getCurrSeg();
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

				if (!pruned[goldDepIndex])
					oracle++;
			}
		}

		gold = reader.nextInstance();
	}

	cout << "Pruning recall: " << oracle / numSeg << endl;
}

} /* namespace segparser */

using namespace segparser;

int main(int argc, char** argv) {
	//test1();

	Options options;
	options.processArguments(argc, argv);

	Options prunerOptions = options;
	prunerOptions.setPrunerOptions();

	SegParser* pruner = NULL;
	DependencyPipe prunerPipe(&prunerOptions);

    DependencyPipe pipe(&options);

	if (options.train) {

		if (options.trainPruner) {

			cout << "Pruner flags:" << endl;
			prunerOptions.outputArg();

			prunerPipe.loadCoarseMap(prunerOptions.trainFile);

			vector<inst_ptr> trainingData = prunerPipe.createInstances(prunerOptions.trainFile);

			pruner = new SegParser(&prunerPipe, &prunerOptions);
			pruner->pruner = NULL;
/*
			pruner->loadModel(options.modelName + ".pruner");

			int numFeats = prunerPipe.dataAlphabet->size() - 1;
			int numTypes = prunerPipe.typeAlphabet->size() - 1;
			cout << "Pruner Num Feats: " << numFeats << endl;
			cout << "Pruner Num Edge Labels: " << numTypes << endl;
*/

			int numFeats = prunerPipe.dataAlphabet->size() - 1;
			int numTypes = prunerPipe.typeAlphabet->size() - 1;
			cout << "Pruner Num Feats: " << numFeats << endl;
			cout << "Pruner Num Edge Labels: " << numTypes << endl;

			pruner->train(trainingData);
			pruner->closeDecoder();

			pruner->evaluatePruning();
		}

	    cout << "Model flags:" << endl;
    	options.outputArg();

	    pipe.loadCoarseMap(options.trainFile);

	    vector<inst_ptr> trainingData = pipe.createInstances(options.trainFile);

	    //pipe.closeAlphabets();

	    SegParser sp(&pipe, &options);
	    sp.pruner = pruner;

	   // sp.loadModel(options.modelName + ".train");

	    int numFeats = pipe.dataAlphabet->size() - 1;
	    int numTypes = pipe.typeAlphabet->size() - 1;
	    cout << "Num Feats: " << numFeats << endl;
	    cout << "Num Edge Labels: " << numTypes << endl;

	    sp.train(trainingData);

	    cout << "Saving model ... ";
	    cout.flush();
	    if (sp.pruner) {
	    	sp.pruner->saveModel(options.modelName + ".pruner", sp.pruner->parameters);
	    }
	    sp.saveModel(options.modelName, sp.parameters);
	    sp.closeDecoder();
	    cout << "done." << endl;

	}

	if (options.test) {
	    DependencyPipe testPipe(&options);
	    testPipe.loadCoarseMap(options.testFile);

	    SegParser testSp(&testPipe, &options);

	    cout << "\nLoading model ... ";
	    cout.flush();
	    pruner = NULL;
		if (options.trainPruner) {

			prunerPipe.loadCoarseMap(prunerOptions.testFile);

			pruner = new SegParser(&prunerPipe, &prunerOptions);
			pruner->pruner = NULL;
			pruner->loadModel(options.modelName + ".pruner");

			int numFeats = prunerPipe.dataAlphabet->size() - 1;
			int numTypes = prunerPipe.typeAlphabet->size() - 1;
			cout << "Pruner Num Feats: " << numFeats << endl;
			cout << "Pruner Num Edge Labels: " << numTypes << endl;
		}
		testSp.pruner = pruner;
	    testSp.loadModel(options.modelName);
	    cout << "done." << endl;

	    int numFeats = testPipe.dataAlphabet->size() - 1;
	    int numTypes = testPipe.typeAlphabet->size() - 1;
	    cout << "Num Feats: " << numFeats << endl;
	    cout << "Num Edge Labels: " << numTypes << endl;

	    //pipe.closeAlphabets();

	    // run multi-thread to test
		string devfile = options.testFile;
		string devoutfile = devfile + ".res";
		cout << "build dev params" << endl;
		testSp.devParams->copyParams(testSp.parameters);
	    testSp.dt->start(devfile, devoutfile, &testSp, true);

	    // wait until all finishes
	    pthread_join(testSp.dt->workThread, NULL);
	    testSp.closeDecoder();
	}

    //if(options.eval) {
	 //   cout << "\nEVALUATION PERFORMANCE:" << endl;
	  //  evaluate(options.testFile, options.outFile, &options);
	//}

	return 0;
}


