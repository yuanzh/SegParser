/*
 * DependencyReader.cpp
 *
 *  Created on: Mar 27, 2014
 *      Author: yuanz
 */

#include "DependencyReader.h"
#include "../util/StringUtils.h"
#include <assert.h>
#include <utility>
#include <boost/regex.hpp>

namespace segparser {

DependencyReader::DependencyReader(Options* options, string file) : options(options) {
	hasCandidate = true;
	isTrain = false;
	startReading(file);
}

DependencyReader::DependencyReader() {
	hasCandidate = true;
	isTrain = false;
}

DependencyReader::~DependencyReader() {
}

void DependencyReader::startReading(Options* options, string file) {
	this->options = options;
	fin.open(file.c_str());
}

void DependencyReader::startReading(string file) {
	fin.open(file.c_str());
}

void DependencyReader::close() {
	if (fin.is_open())
		fin.close();
}

HeadIndex DependencyReader::parseHeadIndex(string str) {
	unsigned int pos = str.find("/");
	assert(pos != string::npos);
	int word = atoi(str.substr(0, pos).c_str());
	int seg = atoi(str.substr(pos + 1).c_str());
	return HeadIndex(word, seg);
}

void DependencyReader::addGoldSegElement(WordInstance* word, string form, string lemma, string pos,
		string morphStr, int segid, int hwordid, int hsegid, string lab) {
	word->goldForm.push_back(form);
	word->goldLemma.push_back(lemma);
	word->goldPos.push_back(pos);
	word->goldDep.push_back(HeadIndex(hwordid, hsegid));
	word->goldLab.push_back(lab);
	word->goldMorphIndex = -1;
	word->goldAlIndex = -1;

	if (morphStr != "_") {
		vector<string> data;
		StringSplit(morphStr, "|", &data);

		if (word->goldAlIndex == -1 && data[0][data[0].length() - 1] == 'y') {
			word->goldAlIndex = segid;
		}

		if (word->goldMorphIndex == -1) {
			assert(data.size() == 4);
			bool hasMorphInfo = false;
			for (int i = 1; i < 4; ++i) {
				string val = data[i].substr(data[i].find_last_of("=") + 1, string::npos);
				if (val != "na" && val != "NA")
					hasMorphInfo = true;
			}
			if (hasMorphInfo) {
				word->goldMorph.clear();
				for (int i = 1; i < 4; ++i) {
					string val = data[i].substr(data[i].find_last_of("=") + 1, string::npos);
					word->goldMorph.push_back(val);
				}
				word->goldMorphIndex = segid;
			}
		}
	}
}

void DependencyReader::normalizeProb(WordInstance* word) {
	// normalize seg/pos candidate probability

	double sumSegProb = 0.0;
	for (unsigned int i = 0; i < word->candSeg.size(); ++i) {
		for (unsigned int j = 0; j < word->candSeg[i].element.size(); ++j) {
			SegElement& ele = word->candSeg[i].element[j];
			double sumPosProb = 0.0;
			for (unsigned int k = 0; k < ele.candPos.size(); ++k) {
				sumPosProb += ele.candProb[k];

				//if (k > 0) {
				//	assert(ele.candProb[k - 1] >= ele.candProb[k]);
				//}
			}
			assert(sumPosProb > 0.0);
			for (unsigned int k = 0; k < ele.candPos.size(); ++k) {
				ele.candProb[k] /= sumPosProb;
				if (ele.candProb[k] > 1e-6)
					ele.candProb[k] = log(ele.candProb[k]);
				else
					ele.candProb[k] = -1000000;
			}
		}
		sumSegProb += word->candSeg[i].prob;

		//if (i > 0) {
		//	assert(word->candSeg[i - 1].prob >= word->candSeg[i].prob);
		//}
	}

	assert(sumSegProb > 0.0);
	for (unsigned int i = 0; i < word->candSeg.size(); ++i) {
		word->candSeg[i].prob /= sumSegProb;
		if (word->candSeg[i].prob > 1e-6)
			word->candSeg[i].prob = log(word->candSeg[i].prob);
		else
			word->candSeg[i].prob = -1000000;
	}
}

void DependencyReader::addGoldSegToCand(WordInstance* word) {
	// add the gold seg in to seg candidate if not exist (with prob 0)
	double prob = hasCandidate ? (isTrain ? 0.3 : 0.0) : 1.0;

	string goldSegStr = word->goldForm[0];
	for (unsigned int i = 1; i < word->goldForm.size(); ++i)
		goldSegStr += "+" + word->goldForm[i];

	unsigned int goldSegID = 0;
	for (; goldSegID < word->candSeg.size(); ++goldSegID) {
		if (goldSegStr.compare(word->candSeg[goldSegID].segStr) == 0) {
			break;
		}
	}

	if (goldSegID == word->candSeg.size()) {
		// new cand
		//cout << "gold seg not exist" << endl;

		SegInstance segInst;
		segInst.prob = prob;
		segInst.segStr = goldSegStr;
		segInst.morph = word->goldMorph;
		segInst.morphIndex = word->goldMorphIndex;
		segInst.AlIndex = word->goldAlIndex;

		segInst.element.resize(word->goldForm.size());
		for (unsigned int i = 0; i < word->goldForm.size(); ++i) {
			SegElement& curr = segInst.element[i];
			curr.currPosCandID = 0;
			curr.form = word->goldForm[i];
			curr.lemma = word->goldLemma[i];
			curr.candPos.resize(1);
			curr.candPosid.resize(1);
			curr.candDetPosid.resize(1);
			curr.candProb.resize(1);
			curr.candSpecialPos.resize(1);

			curr.candPos[0] = word->goldPos[i];
			curr.candProb[0] = 1.0;
		}

		word->currSegCandID = goldSegID;
		word->candSeg.push_back(segInst);
	}
	else {
		// old cand, check pos
		SegInstance& segInst = word->candSeg[goldSegID];
		word->currSegCandID = goldSegID;

		assert(word->goldForm.size() == segInst.element.size());
		for (unsigned int i = 0; i < word->goldForm.size(); ++i) {
			assert(word->goldForm[i].compare(segInst.element[i].form) == 0);
			string goldPos = word->goldPos[i];
			unsigned int goldPosID = 0;
			SegElement& ele = segInst.element[i];
			for (; goldPosID < ele.candPos.size(); ++goldPosID) {
				if (goldPos.compare(ele.candPos[goldPosID]) == 0) {
					break;
				}
			}

			if (goldPosID == ele.candPos.size()) {
				// new pos
				//cout << "gold pos not exist" << endl;
				ele.candPos.resize(goldPosID + 1);
				ele.candPosid.resize(goldPosID + 1);
				ele.candDetPosid.resize(goldPosID + 1);
				ele.candProb.resize(goldPosID + 1);
				ele.candSpecialPos.resize(goldPosID + 1);

				ele.candPos[goldPosID] = goldPos;
				ele.candProb[goldPosID] = prob;

				ele.currPosCandID = goldPosID;
			}
			else {
				// old pos
				ele.currPosCandID = goldPosID;
			}
		}
	}

	// add dep, lab will be set in segInstID
	SegInstance& segInst = word->getCurrSeg();
	for (int i = 0; i < segInst.size(); ++i) {
		segInst.element[i].dep = word->goldDep[i];
	}
}

void DependencyReader::addSegCand(WordInstance* word, string str) {
	vector<string> dataList;
	StringSplit(str, "||", &dataList);
	assert(dataList.size() == 5);
	SegInstance segInst;

	double segProb = atof(dataList[4].c_str());
	segInst.prob = segProb;

	int AlIndex = atoi(dataList[1].c_str());
	segInst.AlIndex = AlIndex;

	int morphIndex = atoi(dataList[2].c_str());
	segInst.morphIndex = morphIndex;

	StringSplit(dataList[3], "/", &segInst.morph);

	bool hasMorphValue = false;
	for (unsigned int i = 0; i < segInst.morph.size(); ++i) {
		if (segInst.morph[i] != "na" && segInst.morph[i] != "NA") {
			hasMorphValue = true;
			break;
		}
	}
	if (!hasMorphValue) {
		segInst.morphIndex = -1;
		segInst.morph.clear();
	}

	string segStr = dataList[0];
	vector<string> segList;
	StringSplit(segStr, "&&", &segList);

	segInst.element.resize(segList.size());
	for (unsigned int i = 0; i < segList.size(); ++i) {
		SegElement& curr = segInst.element[i];
		vector<string> posList;
		StringSplit(segList[i], "@#", &posList);
		curr.form = posList[0];		// normalize is done when set inst ids
		curr.lemma = posList[1];
		curr.candPos.resize(posList.size() - 2);
		curr.candPosid.resize(posList.size() - 2);
		curr.candDetPosid.resize(posList.size() - 2);
		curr.candProb.resize(posList.size() - 2);
		curr.candSpecialPos.resize(posList.size() - 2);

		for (unsigned int j = 2; j < posList.size(); ++j) {
			int pos = posList[j].find_last_of("_");
			curr.candPos[j - 2] = posList[j].substr(0, pos);
			curr.candProb[j - 2] = atof(posList[j].substr(pos + 1).c_str());
		}

		if (i > 0)
			segInst.segStr += "+";
		segInst.segStr += curr.form;
	}

	//string wordStr = "";
	//for (int i = 0; i < segInst.size(); ++i) {
	//	wordStr += segInst.element[i].form;
	//}
	//assert(wordStr == word->wordStr);

	word->candSeg.push_back(move(segInst));
}

void DependencyReader::concatSegStr(WordInstance* word) {
	word->wordStr = "";
	for (unsigned int i = 0; i < word->goldForm.size(); ++i) {
		word->wordStr += word->goldForm[i];
	}
}

inst_ptr DependencyReader::nextInstance() {

	if (fin.eof()) {
		return inst_ptr((DependencyInstance*)NULL);
	}

	string str;
	getline(fin, str);
	if (str.empty()) {
		return inst_ptr((DependencyInstance*)NULL);
	}

	inst_ptr s(new DependencyInstance());
	vector<string> data;
	while (!str.empty()) {
		data.push_back(str);
		getline(fin, str);
	}

	// get sentence length and seg counts
	// word id starts from 1, seg starts from 0
	int len = 0;
	for (unsigned int i = 0; i < data.size(); ++i) {
		int pos = data[i].find("\t");
		HeadIndex hi = parseHeadIndex(data[i].substr(0, pos));
		len = hi.hWord + 1;	// include root
	}

	s->numWord = len;
	s->word.resize(len);

	// add root information
	addGoldSegElement(&s->word[0], "<root>", "<root>", "<root-POS>", "_", 0, -1, 0, "<no-type>");
	concatSegStr(&s->word[0]);
	addSegCand(&s->word[0], "<root>@#<root>@#<root-POS>_1.0||-1||0||_||1.0");

	// process each line
	for (unsigned int i = 0; i < data.size(); ++i) {
		vector<string> line;
		StringSplit(data[i], "\t", &line);

		HeadIndex id = parseHeadIndex(line[0]);
		string word = line[1];
		string lemma = line[2];
		string pos = line[3];
		string morphStr = line[5];
		HeadIndex head = parseHeadIndex(line[6]);
		string lab = options->labeled ? line[7] : "<no-type>";

		addGoldSegElement(&s->word[id.hWord], word, lemma, pos, morphStr, id.hSeg, head.hWord, head.hSeg, lab);
		assert((unsigned int)id.hSeg + 1 == s->word[id.hWord].goldForm.size());
	}

	// complete information
	for (int i = 1; i < len; ++i) {
		concatSegStr(&s->word[i]);
	}

	// process segmentation candidate
	if (hasCandidate) {
		for (int i = 1; i < len; ++i) {
			getline(fin, str);
			vector<string> segCand;
			StringSplit(str, "\t", &segCand);
			if (s->word[i].wordStr != segCand[0]) {
				cout << str << endl;
				cout << s->word[i].wordStr << " " << segCand[0] << endl;
			}
			assert(s->word[i].wordStr.compare(segCand[0]) == 0);
			for (unsigned int j = 1; j < segCand.size(); ++j) {
				addSegCand(&s->word[i], segCand[j]);
			}
		}
		getline(fin, str);
		assert(str.empty());
	}

	// complete information
	for (int i = 0; i < len; ++i) {
		addGoldSegToCand(&s->word[i]);
		normalizeProb(&s->word[i]);
	}

	s->constructConversionList();
   	s->setOptSegPosCount();
  	s->buildChild();

   	return s;
}

} /* namespace segparser */
