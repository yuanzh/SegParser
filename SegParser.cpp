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

SegParser::SegParser(DependencyPipe* pipe, Options* options)
	: pipe(pipe), options(options), devTimes(0) {
	// Set up arrays
	parameters = new Parameters(pipe->dataAlphabet->size(), options);
	devParams = new Parameters(pipe->dataAlphabet->size(), options);
	pruner = NULL;
	if (options->train) {
		decoder = DependencyDecoder::createDependencyDecoder(options, options->learningMode, options->trainThread, true);
		decoder->initialize();
	}
	else {
		decoder = NULL;
	}
	dt = new DevelopmentThread();
	FeatureVector::initVec(pipe->dataAlphabet->size());
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
	}

	if (options->saveBestModel) {
		cout << "Best model performance: " << options->bestScore << endl;
	}
}

void SegParser::trainingIter(vector<inst_ptr>& goldList, vector<inst_ptr>& predList, int iter) {

	Timer timer;

	for(unsigned int i = 0; i < goldList.size(); ++i) {
		if((i+1) % 100 == 0) {
			cout << "  " << (i+1);
			double diff = timer.stop();
			cout << " (time=" << (int)(diff / 1000) << "s)";
			cout.flush();
		}

		inst_ptr gold = goldList[i];
		inst_ptr pred = predList[i];

		FeatureExtractor fe(pred.get(), this, parameters, options->trainThread);

		string str;

		assert(gold->fv.binaryIndex.size() > 0);

		decoder->train(gold.get(), pred.get(), &fe, iter);

		if (options->useSP) {
			uint64_t code = pipe->fe->genCodePF(HighOrder::SEG_PROB, 0);
			int index = pipe->dataAlphabet->lookupIndex(TemplateType::THighOrder, code, false);
			if (index > 0 && parameters->parameters[index] < 0.0) {
				parameters->parameters[index] = 0.0;
			}
		}

	}

	cout << endl;

	cout << "  " << goldList.size() << " instances" << endl;

	if (options->test)
		checkDevStatus(iter);
}

void SegParser::checkDevStatus(int iter) {
	if (dt->isDevTesting) {
		cout << "processing sentences: ";

		pthread_mutex_lock(&dt->finishMutex);
		cout << dt->currFinishID << " to ";
		pthread_mutex_unlock(&dt->finishMutex);

		pthread_mutex_lock(&dt->processMutex);
		cout << dt->currProcessID << endl;
		pthread_mutex_unlock(&dt->processMutex);

		cout << "Wait for testing to finish." << endl;
		pthread_join(dt->workThread, NULL);
	}

	// start new thread for dev
	string devfile = options->testFile;
	string devoutfile = options->outFile;

	cout << "build dev params" << endl;
	devParams->copyParams(parameters);
	devParams->averageParams(decoder->getUpdateTimes());

	cout << "start new dev " << devTimes << endl;
	dt->start(devfile, devoutfile, this, false);
	devTimes++;
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
	parameters->size = parameters->parameters.size();
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

	    int numFeats = pipe.dataAlphabet->size() - 1;
	    int numTypes = pipe.typeAlphabet->size() - 1;
	    cout << "Num Feats: " << numFeats << endl;
	    cout << "Num Edge Labels: " << numTypes << endl;

	    sp.train(trainingData);
	    sp.closeDecoder();
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
		string devoutfile = options.outFile;
		cout << "build dev params" << endl;
		testSp.devParams->copyParams(testSp.parameters);
	    testSp.dt->start(devfile, devoutfile, &testSp, true);

	    // wait until all finishes
	    pthread_join(testSp.dt->workThread, NULL);
	    testSp.closeDecoder();
	}

	return 0;
}


