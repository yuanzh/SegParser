/*
 * DevelopmentThread.cpp
 *
 *  Created on: Apr 16, 2014
 *      Author: yuanz
 */

#include "DevelopmentThread.h"
#include <boost/shared_ptr.hpp>
#include "../io/DependencyWriter.h"
#include "../io/DependencyReader.h"
#include "../util/Timer.h"
#include "../util/Constant.h"

namespace segparser {

void* work(void* threadid);
void* outputThreadFunc(void* instance);
void* decodeThreadFunc(void* instance);

DevelopmentThread::DevelopmentThread() {
	isDevTesting = false;
}

DevelopmentThread::~DevelopmentThread() {
}

void* outputThreadFunc(void* instance) {
	DevelopmentThread* inst = (DevelopmentThread*)instance;

	DependencyWriter writer(inst->options, inst->devoutfile);

	bool allFinish = false;
	int badcnt = 0;
	while (!allFinish || !inst->id2Pred.empty()) {
		bool find = true;

		pthread_mutex_lock( &inst->finishMutex );

		if (inst->finishThreadNum >= inst->decodeThreadNum) {
			allFinish = true;
		}

		if (inst->id2Pred.find(inst->currFinishID) != inst->id2Pred.end()) {
			inst_ptr tmp_ptr = inst->id2Pred[inst->currFinishID];

			writer.writeInstance(tmp_ptr.get());
			inst->id2Pred.erase(inst->currFinishID);
			inst->currFinishID++;
		}
		else {
			find = false;
		}

		pthread_mutex_unlock( &inst->finishMutex );

		if (allFinish && !inst->id2Pred.empty() && !find) {
			cout << "output thread bug" << endl;
			badcnt++;
			//System.exit(0);
		}
		else {
			badcnt = 0;
		}

		if (!find) {
			usleep(100 * 1000);
		}
		if (badcnt > 1000) {
			cout << "too many bugs. break" << endl;
			break;
		}
	}

	pthread_exit(NULL);
	return NULL;
}

void* decodeThreadFunc(void* instance) {
	DevelopmentThread* inst = (DevelopmentThread*)instance;

	DependencyReader& reader = inst->reader;

	Parameters* params = inst->sp->devParams;		// params for development
	DependencyDecoder* decoder = DependencyDecoder::createDependencyDecoder(inst->options, inst->options->testingMode, inst->options->devThread, false);

	decoder->initialize();
	decoder->failTime = 0;

	while(true) {
		inst_ptr pred;
		boost::shared_ptr<FeatureExtractor> fe;
		int currProcessID = 0;

		pthread_mutex_lock(&inst->processMutex);

		currProcessID = inst->currProcessID;
		inst->currProcessID++;
		pred = reader.nextInstance();

		DependencyInstance gold;
		assert(&gold != pred.get());

		if (pred) {
			pred->setInstIds(inst->sp->pipe, inst->options);
			gold = *(pred.get());
			decoder->removeGoldInfo(pred.get());		// make sure gold information is wiped at the beginning;
			fe = boost::shared_ptr<FeatureExtractor>(new FeatureExtractor(pred.get(), inst->sp, params, inst->options->devThread));
			//decoder->initInst(pred.get(), fe.get());	// remove gold info and init trees
		}

		pthread_mutex_unlock(&inst->processMutex);

		if (inst->currProcessID > inst->options->testSentences || !pred) {
			pthread_mutex_lock( &inst->finishMutex );
			inst->finishThreadNum++;
			pthread_mutex_unlock( &inst->finishMutex );

			break;
		}

		if (inst->verbal) {
			cout << currProcessID << " ";
			cout.flush();
		}

		//cout << "decode " << currProcessID << endl;

		//if (currProcessID < 17) {
		//	cout << "skip" << endl;
		//}
		//else {
			decoder->decode(pred.get(), &gold, fe.get());
		//}

		pthread_mutex_lock( &inst->finishMutex );

		inst->id2Pred[currProcessID] = pred;

		//cout << "evaluate " << currProcessID << endl;

		inst->evaluate(pred.get(), &gold);

		pthread_mutex_unlock( &inst->finishMutex );

		cout << "finish decode " << currProcessID << endl;
	}

	cout << "Fail time: " << decoder->failTime << endl;

	decoder->shutdown();
	delete decoder;

	cout << "decoder shutdown" << endl;

	pthread_exit(NULL);
	return NULL;
}

void* work(void* instance) {
	DevelopmentThread* inst = (DevelopmentThread*)instance;

	Timer timer;

	if (inst->verbal) {
		cout << "Processing Dev Sentence: ";
		cout.flush();
	}

	inst->reader.startReading(inst->options, inst->devfile);

	inst->currProcessID = 0;
	inst->currFinishID = 0;
	inst->processMutex = PTHREAD_MUTEX_INITIALIZER;
	inst->finishMutex = PTHREAD_MUTEX_INITIALIZER;

	//inst->stats.resize(8);
	inst->wordNum = 0;
	inst->corrWordSegNum = 0;
	inst->goldSegNum = 0;
	inst->predSegNum = 0;
	inst->corrSegNum = 0;
	inst->corrPosNum = 0;
	inst->goldDepNum = 0;
	inst->predDepNum = 0;
	inst->corrDepNum = 0;

	// build output thread
	inst->finishThreadNum = 0;
	int rc = pthread_create(&inst->outputThread, NULL, outputThreadFunc, (void*)inst);
	if (rc){
		ThrowException("Error:unable to create output thread: " + to_string(rc));
	}

	if (inst->options->testingMode == DecodingMode::HillClimb) {
		inst->decodeThreadNum = 1;
	}
	else {
		inst->decodeThreadNum = inst->options->devThread;
	}

	inst->decodeThread.resize(inst->decodeThreadNum);
	for (int i = 0; i < inst->decodeThreadNum; ++i) {
		int rc = pthread_create(&inst->decodeThread[i], NULL, decodeThreadFunc, (void*)inst);
	    if (rc){
	       ThrowException("Error:unable to create decode thread: " + to_string(rc));
	    }
	}

	for (int i = 0; i < inst->decodeThreadNum; ++i) {
		pthread_join(inst->decodeThread[i], NULL);
		cout << "decode thread " << i << " finish" << endl;
	}
	pthread_join(inst->outputThread, NULL);
	cout << "output thread finish" << endl;

	cout << endl;
	cout << "-------------------------------------" << endl;
	cout << endl;
	cout << "Seg Acc: " << inst->corrWordSegNum / inst->wordNum << endl;
	double segpre = inst->corrSegNum / inst->predSegNum;
	double segrec = inst->corrSegNum / inst->goldSegNum;
	cout << "Seg pre/rec/f1: " << segpre << " " << segrec << " " << (2 * segpre * segrec) / (segpre + segrec) << " " << inst->predSegNum << " " << inst->goldSegNum << endl;
	double pospre = inst->corrPosNum / inst->predSegNum;
	double posrec = inst->corrPosNum / inst->goldSegNum;
	cout << "Pos pre/rec/f1: " << pospre << " " << posrec << " " << (2 * pospre * posrec) / (pospre + posrec) << " " << inst->predSegNum << " " << inst->goldSegNum << endl;
	double deppre = inst->corrDepNum / inst->predDepNum;
	double deprec = inst->corrDepNum / inst->goldDepNum;
	cout << "Dep pre/rec/f1: " << deppre << " " << deprec << " " << (2 * deppre * deprec) / (deppre + deprec) << " " << inst->predDepNum << " " << inst->goldDepNum << endl;
	cout << endl;

	if (inst->options->useTedEval) {
		cout << "Running TedEval" << endl;
		double tedEval = inst->computeTedEval();
		cout << "TedEval: " << tedEval << endl;
	}
	cout << "-------------------------------------" << endl;
	cout << endl;


	double diffms = timer.stop();

	if (inst->verbal) {
	    cout << "Testing took: " << int(diffms) << " ms"<< endl;
	}

	inst->reader.close();

	inst->isDevTesting = false;

	pthread_exit(NULL);

	return NULL;
}

void DevelopmentThread::start(string devfile, string devoutfile, SegParser* sp, bool verbal) {

	this->devfile = devfile;
	this->devoutfile = devoutfile;
	this->sp = sp;
	this->options = sp->options;
	this->verbal = verbal;
	isDevTesting = true;

    int rc = pthread_create(&workThread, NULL, work, (void*)this);
    if (rc){
       ThrowException("Error:unable to start development thread: " + to_string(rc));
    }
}

int DevelopmentThread::numSegWithoutPunc(DependencyInstance* inst) {
	int num = 0;
	for (int i = 1; i < inst->numWord; ++i) {
		SegInstance& segInst = inst->word[i].getCurrSeg();
		for (int j = 0; j < segInst.size(); ++j) {
			SegElement& ele = segInst.element[j];
			if (!isPunc(ele.candPos[ele.currPosCandID])) {
				num++;
			}
		}
	}
	return num;
}

bool DevelopmentThread::isPunc(string& pos) {
	return pos == "PU";
}

void DevelopmentThread::evaluate(DependencyInstance* inst, DependencyInstance* gold) {
	vector<int> pred2gold(inst->getNumSeg(), -1);	// -1 means removed
	vector<int> gold2pred(gold->getNumSeg(), -1);	// -1 means added

	vector<int> predPos(inst->getNumSeg());
	vector<int> goldPos(gold->getNumSeg());
	vector<string> predPosStr(inst->getNumSeg());
	vector<string> goldPosStr(gold->getNumSeg());

	vector<int> predDep(inst->getNumSeg());
	vector<int> goldDep(gold->getNumSeg());

	wordNum += inst->numWord - 1;		// # word
	goldSegNum += gold->getNumSeg() - 1;	// # gold seg
	predSegNum += inst->getNumSeg() - 1;	// # pred seg
	if (options->evalPunc) {
		goldDepNum += gold->getNumSeg() - 1;
		predDepNum += inst->getNumSeg() - 1;
	}
	else {
		goldDepNum += numSegWithoutPunc(gold);
		predDepNum += numSegWithoutPunc(inst);
	}

	int tmpcnt = 0;
	for (int i = 0; i < inst->numWord; ++i) {
		SegInstance& segInst = inst->word[i].getCurrSeg();
		for (int j = 0; j < segInst.size(); ++j) {
			assert(tmpcnt == inst->wordToSeg(i, j));
			tmpcnt++;
		}
	}

	//for (int i = 1; i < inst->numWord; ++i) {
	//	SegInstance& segInst = inst->word[i].getCurrSeg();
	//	for (int j = 0; j < segInst.size(); ++j) {
	//		assert(segInst.element[j].candProb[segInst.element[j].currPosCandID] >= 1e-6);
	//	}
	//}

	int predLen = 0;
	for (int i = 0; i < inst->numWord; ++i) {
		SegInstance& predSegInst = inst->word[i].getCurrSeg();
		for (int j = 0; j < predSegInst.size(); ++j) {
			predLen += predSegInst.element[j].form.size();
		}
	}
	assert(inst->word[0].getCurrSeg().size() == 1);
	int goldLen = 0;
	for (int i = 0; i < gold->numWord; ++i) {
		goldLen += gold->word[i].wordStr.size();
	}
	assert(gold->word[0].getCurrSeg().size() == 1);
	assert(predLen == goldLen);

	unordered_map<int, int> predID;
	int predIdx = 0;
	int currChar = 0;
	for (int i = 0; i < inst->numWord; ++i) {
		if (i > 0 && inst->word[i].currSegCandID == gold->word[i].currSegCandID)
			corrWordSegNum += 1.0;		// accuracy

		// add pred info
		SegInstance& predSegInst = inst->word[i].getCurrSeg();
		for (int j = 0; j < predSegInst.size(); ++j) {
			int endChar = currChar + predSegInst.element[j].form.size();
			assert(endChar <= predLen);
			assert(predIdx < inst->getNumSeg());
			predPos[predIdx] = predSegInst.element[j].getCurrPos();
			predPosStr[predIdx] = predSegInst.element[j].candPos[predSegInst.element[j].currPosCandID];
			predDep[predIdx] = inst->wordToSeg(predSegInst.element[j].dep);
			int id = currChar * predLen + endChar;
			predID[id] = predIdx;
			assert(predIdx == inst->wordToSeg(i, j));
			predIdx++;
			currChar = endChar;
		}
	}
	assert(currChar == predLen);

	int goldIdx = 0;
	currChar = 0;
	for (int i = 0; i < gold->numWord; ++i) {
		// process gold info
		SegInstance& goldSegInst = gold->word[i].getCurrSeg();
		for (int j = 0; j < goldSegInst.size(); ++j) {
			int endChar = currChar + goldSegInst.element[j].form.size();
			assert(endChar <= goldLen);
			goldPos[goldIdx] = goldSegInst.element[j].getCurrPos();
			goldPosStr[goldIdx] = goldSegInst.element[j].candPos[goldSegInst.element[j].currPosCandID];
			goldDep[goldIdx] = gold->wordToSeg(goldSegInst.element[j].dep);
			int id = currChar * goldLen + endChar;
			if (predID.find(id) != predID.end()) {
				// find match
				int predIdx = predID[id];
				assert(gold2pred[goldIdx] == -1);
				assert(pred2gold[predIdx] == -1);		// one-one mapping
				gold2pred[goldIdx] = predIdx;
				pred2gold[predIdx] = goldIdx;
			}
			assert(goldIdx == gold->wordToSeg(i, j));
			goldIdx++;
			currChar = endChar;
		}
	}
	assert(currChar == goldLen);
	assert(predIdx == inst->getNumSeg());
	assert(goldIdx == gold->getNumSeg());

	// evaluate
	for (unsigned int i = 1; i < pred2gold.size(); ++i) {
		if (pred2gold[i] != -1) {
			corrSegNum += 1.0;			// # correct seg
			int gi = pred2gold[i];
			if (predPos[i] == goldPos[gi]) {
				assert(predPosStr[i] == goldPosStr[gi]);
				corrPosNum += 1.0;		// # correct pos
			}

			int head = predDep[i];
			int ghead = goldDep[gi];
			assert(head >= 0 && head < (int)pred2gold.size());
			assert(ghead >= 0 && ghead < (int)gold2pred.size());

			if ((options->evalPunc || !isPunc(goldPosStr[gi])) && pred2gold[head] == ghead) {
				corrDepNum += 1.0;
				assert(gold2pred[ghead] == head);
			}
		}
	}

	//score
	/*
	FeatureVector fv;
	sp->pipe->createFeatureVector(inst, &fv);
	double predScore = sp->devParams->getScore(&fv);

	gold->fv.clear();
	sp->pipe->createFeatureVector(gold, &gold->fv);
	double goldScore = sp->devParams->getScore(&gold->fv);

	cout << "ccc: pred score: " << predScore << "; goldScore: " << goldScore << " " << (predScore >= goldScore - 1e-6 ? 1 : 0) << endl;
	*/
}

string DevelopmentThread::normalize(string form) {
	while (form.find(":") != string::npos)
		form = form.replace(form.find(":"), 1, "<COLON>");

	while (form.find("(") != string::npos)
		form = form.replace(form.find("("), 1, "<LRB>");

	while (form.find(")") != string::npos)
		form = form.replace(form.find(")"), 1, "<RRB>");

	if (form == "_") {
		form = "<UNDER>";
	}

	return form;
}

double DevelopmentThread::computeTedEval() {
	// re-format gold
	{
		string treeFileName = devfile + ".tree";
		string segFileName = devfile + ".seg";

		ofstream foutTree(treeFileName.c_str());
		ofstream foutSeg(segFileName.c_str());

		DependencyReader reader(options, devfile);
		inst_ptr inst = reader.nextInstance();

		int num = 0;
		while (inst) {
			inst->setInstIds(sp->pipe, options);
			// output tree
			for (int i = 1; i < inst->numWord; ++i) {
				SegInstance& segInst = inst->word[i].getCurrSeg();
				for (int j = 0; j < segInst.size(); ++j) {
					int idx = inst->wordToSeg(i, j);
					SegElement& ele = segInst.element[j];
					string form = normalize(ele.form);
					foutTree << idx << "\t" << form << "\t" << form << "\t";
					string pos = ele.candPos[ele.currPosCandID];
					foutTree << pos << "\t" << pos << "\t_\t";
					int headIdx = inst->wordToSeg(ele.dep);
					foutTree << headIdx << "\t---\t_\t_" << endl;
				}
			}
			foutTree << endl;

			// output seg
			for (int i = 1; i < inst->numWord; ++i) {
				SegInstance& segInst = inst->word[i].getCurrSeg();
				string form = normalize(inst->word[i].wordStr);
				foutSeg << form << "\t";
				for (int j = 0; j < segInst.size(); ++j) {
					if (j > 0) foutSeg << ":";
					SegElement& ele = segInst.element[j];
					form = normalize(ele.form);
					foutSeg << form;
				}
				foutSeg << endl;
			}
			foutSeg << endl;

			num++;
			if (num >= options->testSentences)
				break;

			inst = reader.nextInstance();
		}

		reader.close();
		foutTree.close();
		foutSeg.close();
	}

	// re-format output
	{
		string treeFileName = devoutfile + ".tree";
		string segFileName = devoutfile + ".seg";

		ofstream foutTree(treeFileName.c_str());
		ofstream foutSeg(segFileName.c_str());

		DependencyReader reader(options, devoutfile);
		reader.hasCandidate = false;
		inst_ptr inst = reader.nextInstance();

		while (inst) {
			inst->setInstIds(sp->pipe, options);
			// output tree
			for (int i = 1; i < inst->numWord; ++i) {
				SegInstance& segInst = inst->word[i].getCurrSeg();
				for (int j = 0; j < segInst.size(); ++j) {
					int idx = inst->wordToSeg(i, j);
					SegElement& ele = segInst.element[j];
					string form = normalize(ele.form);
					foutTree << idx << "\t" << form << "\t" << form << "\t";
					string pos = ele.candPos[ele.currPosCandID];
					foutTree << pos << "\t" << pos << "\t_\t";
					int headIdx = inst->wordToSeg(ele.dep);
					foutTree << headIdx << "\t---\t_\t_" << endl;
				}
			}
			foutTree << endl;

			// output seg
			for (int i = 1; i < inst->numWord; ++i) {
				SegInstance& segInst = inst->word[i].getCurrSeg();
				string form = normalize(inst->word[i].wordStr);
				foutSeg << form << "\t";
				for (int j = 0; j < segInst.size(); ++j) {
					if (j > 0) foutSeg << ":";
					SegElement& ele = segInst.element[j];
					form = normalize(ele.form);
					foutSeg << form;
				}
				foutSeg << endl;
			}
			foutSeg << endl;

			inst = reader.nextInstance();
		}

		reader.close();
		foutTree.close();
		foutSeg.close();
	}

	// run command
	{
		string cmd = "sh ../../../tedeval/TedWrappers_20131015/tedeval_seg.sh";
		cmd += " -s " + devoutfile + ".tree" + " -P " + devoutfile + ".seg";
		cmd += " -g " + devfile + ".tree" + " -G " + devfile + ".seg";
		//cmd += " &> " + devfile + ".tedeval_log";

		cout << "cmd: " << cmd << endl;

		//int c = system(cmd.c_str());
		//assert(c == 0);

	    FILE* in = popen(cmd.c_str(), "r");
	    ofstream log(devfile + ".tedeval_log");
	    assert(in != 0);
	    char buff[512];
	    while(fgets(buff, sizeof(buff), in) != NULL){
	        log << buff;
	    }
	    pclose(in);
	    log.close();
	}

	string resultFileName = devoutfile + ".tree.4tedeval.simple_tedeval.res-unlabeled.ted";

	ifstream res(resultFileName.c_str());
	string str;
	getline(res, str);
	while (!res.eof() && str.find("AVG:") == string::npos)
		getline(res, str);
	assert(str.find("AVG:") != string::npos);
	vector<string> data;
	StringSplit(str, "\t", &data);
	double ret = atof(data[2].c_str());
	res.close();

	return ret;
}

} /* namespace segparser */
