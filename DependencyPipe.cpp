/*
 * DependencyPipe.cpp
 *
 *  Created on: Apr 4, 2014
 *      Author: yuanz
 */

#include "DependencyPipe.h"
#include "util/Constant.h"
#include <assert.h>
#include "util/StringUtils.h"
#include "io/DependencyReader.h"
#include "util/Random.h"

namespace segparser {

DependencyPipe::DependencyPipe(Options* options)
	: options(options) {
	dataAlphabet = new FeatureAlphabet(300000);
	typeAlphabet = new Alphabet(100);
	posAlphabet = new Alphabet(100);
	lexAlphabet = new Alphabet(30000);

	fe = new FeatureEncoder();
}

DependencyPipe::~DependencyPipe() {
	delete dataAlphabet;
	delete typeAlphabet;
	delete posAlphabet;
	delete lexAlphabet;

	delete fe;
}

void DependencyPipe::loadCoarseMap(string& file) {
	int p = file.find("/data/");
	string path = file.substr(0, file.find("/", p + 1));

	string mapFile = path + string("/lab/") + PossibleLang::langString[options->lang] + string(".uni.map");
	ifstream fin(mapFile.c_str());
	string str;
	while (!fin.eof()) {
		getline(fin, str);
		if (str.empty())
			continue;
		vector<string> data;
		StringSplit(str, "\t", &data);
		coarseMap[data[0]] = data[1];
	}
	fin.close();
}

void DependencyPipe::setAndCheckOffset() {
	// set
	fe->largeOff = fe->getBits(lexAlphabet->size());
	fe->midOff = fe->getBits(posAlphabet->size());
	fe->midOff = max(fe->getBits(typeAlphabet->size()), fe->midOff);
	fe->tempOff = fe->getBits(Arc::COUNT);
	fe->tempOff = max(fe->getBits(SecondOrder::COUNT), fe->tempOff);
	fe->tempOff = max(fe->getBits(ThirdOrder::COUNT), fe->tempOff);
	fe->tempOff = max(fe->getBits(HighOrder::COUNT), fe->tempOff);

	cout << "large offset: " << fe->largeOff << endl;
	cout << "mid offset: " << fe->midOff << endl;
	cout << "temp offset: " << fe->tempOff << endl;

	// check
	if (fe->midOff * 6 + fe->tempOff + fe->flagOff > 64) {
		ThrowException("size too large 1");
	}
	else if (fe->largeOff * 2 + fe->midOff * 2 + fe->tempOff + fe->flagOff > 64) {
		ThrowException("size too large 2");
	}
	else if (fe->largeOff * 3 + fe->tempOff > 64) {
		ThrowException("size too large 4");
	}
	else if (fe->getBits(MAX_CHILD_NUM) > fe->flagOff
			|| fe->getBits(MAX_SPAN_LENGTH) > fe->flagOff
			|| fe->getBits(MAX_LEN_DIFF) > fe->flagOff) {
		ThrowException("size too large 5");
	}
}

void DependencyPipe::closeAlphabets() {
	dataAlphabet->stopGrowth();
	typeAlphabet->stopGrowth();
	posAlphabet->stopGrowth();
	lexAlphabet->stopGrowth();

	cout << "arc alphabet: " << dataAlphabet->arcMap.size() << endl;
	cout << "second order alphabet: " << dataAlphabet->secondOrderMap.size() << endl;
	cout << "third order alphabet: " << dataAlphabet->thirdOrderMap.size() << endl;
	cout << "high order alphabet: " << dataAlphabet->highOrderMap.size() << endl;
}

void DependencyPipe::buildSuffixList() {
	suffixList.insert("h");
	suffixList.insert("hA");
	suffixList.insert("k");
	suffixList.insert("y");
	suffixList.insert("hmA");
	suffixList.insert("kmA");
	suffixList.insert("nA");
	suffixList.insert("km");
	suffixList.insert("hm");
	suffixList.insert("hn");
	suffixList.insert("kn");
	suffixList.insert("A");
	suffixList.insert("An");
	suffixList.insert("yn");
	suffixList.insert("wn");
	suffixList.insert("wA");
	suffixList.insert("At");
	suffixList.insert("t");
	suffixList.insert("n");
	suffixList.insert("p");
	suffixList.insert("ny");
}

void DependencyPipe::buildDictionary(string& goldfile) {
	cout << "Creating Dictionary ... ";
	cout.flush();

	// fill alphabet with special value
	posAlphabet->lookupIndex("#START#");
	posAlphabet->lookupIndex("#MID#");
	posAlphabet->lookupIndex("#END#");

	lexAlphabet->lookupIndex("#START#");
	lexAlphabet->lookupIndex("#MID#");
	lexAlphabet->lookupIndex("#END#");
	lexAlphabet->lookupIndex("\"");
	lexAlphabet->lookupIndex("(");
	lexAlphabet->lookupIndex(")");

	typeAlphabet->lookupIndex("<no-type>");

	buildSuffixList();

	DependencyReader reader(options, goldfile);
	if (!options->jointSegPos)
		reader.hasCandidate = false;
	inst_ptr gold = reader.nextInstance();

	int cnt = 0;

	while (gold) {
		if ((cnt + 1) % 1000 == 0) {
			cout << (cnt + 1) << "  ";
			cout.flush();
		}

		gold->setInstIds(this, options);
		gold = reader.nextInstance();
		cnt++;
		if (cnt >= options->trainSentences)
			break;
	}
	reader.close();

	cout << "Done." << endl;

	setAndCheckOffset();
}

void DependencyPipe::buildDictionaryWithOOV(string& goldfile) {
	cout << "Creating Dictionary with OOV ... ";
	cout.flush();

	// fill alphabet with special value
	posAlphabet->lookupIndex("#START#");
	posAlphabet->lookupIndex("#MID#");
	posAlphabet->lookupIndex("#END#");

	typeAlphabet->lookupIndex("<no-type>");

	buildSuffixList();

	DependencyReader reader(options, goldfile);
	if (!options->jointSegPos)
		reader.hasCandidate = false;
	inst_ptr gold = reader.nextInstance();

	int cnt = 0;
	unordered_map<string, int> wordCount;

	while (gold) {
		if ((cnt + 1) % 1000 == 0) {
			cout << (cnt + 1) << "  ";
			cout.flush();
		}

		for (int i = 0; i < gold->numWord; ++i) {
			WordInstance& word = gold->word[i];
			for (unsigned int j = 0; j < word.goldForm.size(); ++j) {
				wordCount[gold->normalize(word.goldForm[j])]++;
			}

			SegInstance& currSeg = word.getCurrSeg();
			// label id
			for (int j = 0; j < currSeg.size(); ++j)
				typeAlphabet->lookupIndex(word.goldLab[j]);

			for (unsigned int j = 0; j < word.candSeg.size(); ++j) {
				SegInstance& seg = word.candSeg[j];
				for (unsigned int k = 0; k < seg.morph.size(); ++k) {
					posAlphabet->lookupIndex(seg.morph[k]);
				}

				for (int k = 0; k < seg.size(); ++k) {
					SegElement& ele = seg.element[k];
					// pos
					for (int l = 0; l < ele.candPosNum(); ++l) {
						posAlphabet->lookupIndex(ele.candPos[l]);
					}
				}
			}
		}

		gold = reader.nextInstance();
		cnt++;
		if (cnt >= options->trainSentences)
			break;
	}
	reader.close();

	assert(lexAlphabet->size() == 1);

	lexAlphabet->lookupIndex("#START#");
	lexAlphabet->lookupIndex("#MID#");
	lexAlphabet->lookupIndex("#END#");
	lexAlphabet->lookupIndex("\"");
	lexAlphabet->lookupIndex("(");
	lexAlphabet->lookupIndex(")");

	for (unordered_map<string, int>::iterator iter = wordCount.begin(); iter != wordCount.end(); ++iter) {
		if (iter->second >= 2) {
			lexAlphabet->lookupIndex(iter->first);
		}
	}

	cout << "Done." << endl;

	setAndCheckOffset();

	lexAlphabet->stopGrowth();
	typeAlphabet->stopGrowth();
	posAlphabet->stopGrowth();
}

void DependencyPipe::createAlphabet(string& goldfile) {

	buildDictionary(goldfile);
	//buildDictionaryWithOOV(goldfile);

	cout << "Creating Alphabet ... ";
	cout.flush();

	DependencyReader reader(options, goldfile);
	if (!options->jointSegPos)
		reader.hasCandidate = false;
	inst_ptr gold = reader.nextInstance();

	int cnt = 0;

	while (gold) {
		if ((cnt + 1) % 1000 == 0) {
			cout << (cnt + 1) << "  ";
			cout.flush();
		}

		gold->setInstIds(this, options);
		createFeatureVector(gold.get(), &(gold->fv));

		//gold->fv.output();
		//gold->output();

		gold = reader.nextInstance();

		cnt++;
		if (cnt >= options->trainSentences)
			break;
	}

	if (options->useHO) {
		long code = fe->genCodePF(HighOrder::NP, 0);
		dataAlphabet->lookupIndex(TemplateType::THighOrder, code, true);
		code = fe->genCodePF(HighOrder::NP, 1);
		dataAlphabet->lookupIndex(TemplateType::THighOrder, code, true);

		//for (int i = 0; i < posAlphabet->size() + 1; ++i) {
		//	for (int j = 0; j < posAlphabet->size() + 1; ++j) {
		//		code = fe->genCodePPF(HighOrder::NP_HC_MC, i, j);
		//		dataAlphabet->lookupIndex(TemplateType::THighOrder, code, true);
		//	}
		//}
	}

	cout << "Done." << endl;

	closeAlphabets();

	reader.close();
}

vector<inst_ptr> DependencyPipe::createInstances(string goldFile) {

	createAlphabet(goldFile);

	cout << "Num Features: " << dataAlphabet->size() - 1 << endl;
    cout << "Num Edge Labels: " << typeAlphabet->size() - 1 << endl;

	vector<inst_ptr> trainData;

	cout << "Creating Instances: ... ";
	cout.flush();

	DependencyReader reader(options, goldFile);
	if (!options->jointSegPos)
		reader.hasCandidate = false;

	inst_ptr gold = reader.nextInstance();

	int num1 = 0;
	while (gold) {
		if ((num1 + 1) % 1000 == 0) {
			cout << (num1 + 1) << "  ";
			cout.flush();
		}

		if (gold->getNumSeg() - 1 > options->maxLength) {
			cout << "too long: " << gold->getNumSeg() - 1<< endl;
		}
		else {
			gold->setInstIds(this, options);
			createFeatureVector(gold.get(), &(gold->fv));
			trainData.push_back(gold);
			num1++;
			if (num1 >= options->trainSentences)
				break;
		}

		gold = reader.nextInstance();
	}

	cout << "Done." << endl;

	reader.close();

	vector<inst_ptr> ret;
	ret.resize(trainData.size());
	vector<bool> used;
	used.resize(trainData.size());
	Random r(0);
	int id = 0;
	for (unsigned int i = 0; i < trainData.size(); ++i) {
		id = (id + r.nextInt(trainData.size())) % trainData.size();
		while (used[id]) {
			id = (id + 1) % trainData.size();
		}
		ret[i] = trainData[id];
		used[id] = true;
	}

	//return trainData;
	return ret;
}

int DependencyPipe::findRightNearestChildID(vector<HeadIndex>& child, HeadIndex id) {
	unsigned int ret = 0;
	for (; ret < child.size(); ++ret)
		if (id < child[ret]) {
			break;
		}
	return ret;
}

HeadIndex DependencyPipe::findRightNearestChild(vector<HeadIndex>& child, HeadIndex id) {
	for (unsigned int ret = 0; ret < child.size(); ++ret)
		if (id < child[ret]) {
			return child[ret];
		}
	return HeadIndex(-1, 0);
}

HeadIndex DependencyPipe::findLeftNearestChild(vector<HeadIndex>& child, HeadIndex id) {
	for (int ret = child.size() - 1; ret >= 0; --ret)
		if (child[ret] < id) {
			return child[ret];
		}
	return HeadIndex(-1, 0);
}

void DependencyPipe::createFeatureVector(DependencyInstance* inst, FeatureVector* fv) {

	// first order
	for (int mw = 0; mw < inst->numWord; ++mw) {
		SegInstance& segInst = inst->word[mw].getCurrSeg();

		for (int ms = 0; ms < segInst.size(); ++ms) {
			HeadIndex headIndex = segInst.element[ms].dep;		// copy index
			HeadIndex modIndex = HeadIndex(mw, ms);

			if (mw > 0 && headIndex.hWord >= 0) {
				createArcFeatureVector(inst, headIndex, modIndex, fv);
				if (options->useSP) {
					createPos1OFeatureVector(inst, modIndex, fv);
					createPosHOFeatureVector(inst, modIndex, false, fv);
				}
			}

			vector<HeadIndex>& child = segInst.element[ms].child;
			int aid = findRightNearestChildID(child, modIndex);

			// right children
			HeadIndex prev = modIndex;
			for (unsigned int j = aid; j < child.size(); ++j) {
				HeadIndex curr = child[j];
				if (options->useCS) {
					createTripsFeatureVector(inst, modIndex, prev, curr, fv);
					createSibsFeatureVector(inst, prev, curr, prev == modIndex, fv);
				}

				prev = curr;
			}

			// left children
			prev = modIndex;
			for (int j = aid - 1; j >= 0; --j) {
				HeadIndex curr = child[j];
				if (options->useCS) {
					createTripsFeatureVector(inst, modIndex, prev, curr, fv);
					createSibsFeatureVector(inst, prev, curr, prev == modIndex, fv);
				}

				prev = curr;
			}

			if (mw > 0 && headIndex.hWord >= 0) {
				HeadIndex gpIndex = inst->getElement(headIndex).dep;
				if (options->useGP && gpIndex.hWord >= 0) {
					createGPCFeatureVector(inst, gpIndex, headIndex, modIndex, fv);
				}
			}
		}

		if (options->useSP && mw > 0) {
			createSegFeatureVector(inst, mw, fv);
		}
	}

	if (options->useHO) {
		createHighOrderFeatureVector(inst, fv);
	}
}

int DependencyPipe::getBinnedDistance(int x) {
	int flag = 0;
	if (x <= 0) {
		x = -x;
		flag = 0x8;
	}
	if (x > 10)          // x > 10
		flag |= 0x7;
	else if (x > 5)		 // x = 6 .. 10
		flag |= 0x6;
	else
		flag |= x;   	 // x = 1 .. 5
	if (flag == 0) {
		ThrowException("bad distance");
	}
	return flag;
}

void DependencyPipe::createArcFeatureVector(DependencyInstance* inst,
		HeadIndex& headIndex, HeadIndex& modIndex, FeatureVector* fv) {

	uint64_t distFlag = getBinnedDistance(inst->segDist(headIndex, modIndex)) << fe->tempOff;

	SegElement& headEle = inst->getElement(headIndex.hWord, headIndex.hSeg);
	SegElement& modEle = inst->getElement(modIndex.hWord, modIndex.hSeg);

	int headSegIndex = inst->wordToSeg(headIndex);
	int modSegIndex = inst->wordToSeg(modIndex);
	int len = inst->getNumSeg();

	// use head->mod, rather than small->large
	int LP = headEle.getCurrPos();
	int RP = modEle.getCurrPos();

	int pLP = headSegIndex > 0 ? inst->getElement(inst->segToWord(headSegIndex - 1)).getCurrPos() : ConstPosLex::START;
	pLP = headSegIndex == modSegIndex + 1 ? ConstPosLex::MID : pLP;

	int nLP = headSegIndex < len - 1 ? inst->getElement(inst->segToWord(headSegIndex + 1)).getCurrPos() : ConstPosLex::END;
	nLP = headSegIndex == modSegIndex - 1 ? ConstPosLex::MID : nLP;

	int pRP = modSegIndex > 0 ? inst->getElement(inst->segToWord(modSegIndex - 1)).getCurrPos() : ConstPosLex::START;
	pRP = modSegIndex == headSegIndex + 1 ? ConstPosLex::MID : pRP;

	int nRP = modSegIndex < len - 1 ? inst->getElement(inst->segToWord(modSegIndex + 1)).getCurrPos() : ConstPosLex::END;
	nRP = modSegIndex == headSegIndex - 1 ? ConstPosLex::MID : nRP;

	long code = 0;

	int small = headSegIndex < modSegIndex ? headSegIndex : modSegIndex;
	int large = headSegIndex > modSegIndex ? headSegIndex : modSegIndex;

	// feature posR posMid posL
	for (int i = small + 1; i < large; ++i) {
		int MP = inst->getElement(inst->segToWord(i)).getCurrPos();
		code = fe->genCodePPPF(Arc::LP_MP_RP, LP, MP, RP);
		addCode(TemplateType::TArc, code, fv);
		addCode(TemplateType::TArc, code | distFlag, fv);
	}

	// feature posL-1 posL posR posR+1
	code = fe->genCodePPPPF(Arc::pLP_LP_RP_nRP, pLP, LP, RP, nRP);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodePPPF(Arc::LP_RP_nRP, LP, RP, nRP);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodePPPF(Arc::pLP_RP_nRP, pLP, RP, nRP);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodePPPF(Arc::pLP_LP_nRP, pLP, LP, nRP);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodePPPF(Arc::pLP_LP_RP, pLP, LP, RP);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	// feature posL posL+1 posR-1 posR
	code = fe->genCodePPPPF(Arc::LP_nLP_pRP_RP, LP, nLP, pRP, RP);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodePPPF(Arc::nLP_pRP_RP, nLP, pRP, RP);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodePPPF(Arc::LP_pRP_RP, LP, pRP, RP);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodePPPF(Arc::LP_nLP_RP, LP, nLP, RP);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodePPPF(Arc::LP_nLP_pRP, LP, nLP, pRP);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	// feature posL-1 posL posR-1 posR
	// feature posL posL+1 posR posR+1
	code = fe->genCodePPPPF(Arc::pLP_LP_pRP_RP, pLP, LP, pRP, RP);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodePPPPF(Arc::LP_nLP_RP_nRP, LP, nLP, RP, nRP);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	// arc feature
	int HP = headEle.getCurrPos();
	int HW = headEle.formid;
	int MP = modEle.getCurrPos();
	int MW = modEle.formid;

	code = fe->genCodePF(Arc::HP, HP);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodeWF(Arc::HW, HW);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodePPF(Arc::HP_MP, HP, MP);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodePWF(Arc::HP_MW, HP, MW);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodePWF(Arc::HW_MP, MP, HW);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodeWWF(Arc::HW_MW, HW, MW);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodePWF(Arc::HW_HP, HP, HW);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodePPWF(Arc::HP_MP_MW, HP, MP, MW);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodePPWF(Arc::HP_HW_MP, HP, MP, HW);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodePWWF(Arc::HW_MP_MW, MP, HW, MW);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodePWWF(Arc::HP_HW_MW, HP, HW, MW);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodePPWWF(Arc::HP_HW_MP_MW, HP, MP, HW, MW);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	if (options->lang == PossibleLang::Chinese) {
		/*
		int HL = inst->characterid[headEle.st];
		int ML = inst->characterid[modEle.st];

		code = fe->genCodeWF(Arc::HL, HL);
		addCode(TemplateType::TArc, code, fv);

		code = fe->genCodePWF(Arc::HP_ML, HP, ML);
		addCode(TemplateType::TArc, code, fv);

		code = fe->genCodePWF(Arc::HL_MP, MP, HL);
		addCode(TemplateType::TArc, code, fv);

		code = fe->genCodeWWF(Arc::HL_ML, HL, ML);
		addCode(TemplateType::TArc, code, fv);

		code = fe->genCodePWF(Arc::HP_HL, HP, HL);
		addCode(TemplateType::TArc, code, fv);

		code = fe->genCodePPWF(Arc::HP_MP_ML, HP, MP, ML);
		addCode(TemplateType::TArc, code, fv);

		code = fe->genCodePPWF(Arc::HP_HL_MP, HP, MP, HL);
		addCode(TemplateType::TArc, code, fv);

		code = fe->genCodePWWF(Arc::HL_MP_ML, MP, HL, ML);	// fix a HP-MP bug here
		addCode(TemplateType::TArc, code, fv);

		code = fe->genCodePWWF(Arc::HP_HL_ML, HP, HL, ML);
		addCode(TemplateType::TArc, code, fv);

		code = fe->genCodePPWWF(Arc::HP_HL_MP_ML, HP, MP, HL, ML);
		addCode(TemplateType::TArc, code, fv);
		 */

		if (headEle.en > headEle.st && headEle.st >= 0) {
			int HL = inst->characterid[headEle.en - 1];
			int ML = inst->characterid[modEle.en - 1];

			code = fe->genCodeWF(Arc::HL, HL);
			addCode(TemplateType::TArc, code, fv);

			code = fe->genCodePWF(Arc::HP_ML, HP, ML);
			addCode(TemplateType::TArc, code, fv);

			code = fe->genCodePWF(Arc::HL_MP, MP, HL);
			addCode(TemplateType::TArc, code, fv);

			//code = fe->genCodeWWF(Arc::HL_ML, HL, ML);
			//addCode(TemplateType::TArc, code, fv);

			//code = fe->genCodePWF(Arc::HP_HL, HP, HL);
			//addCode(TemplateType::TArc, code, fv);

			code = fe->genCodePPWF(Arc::HP_MP_ML, HP, MP, ML);
			addCode(TemplateType::TArc, code, fv);

			code = fe->genCodePPWF(Arc::HP_HL_MP, HP, MP, HL);
			addCode(TemplateType::TArc, code, fv);

			code = fe->genCodePWWF(Arc::HL_MP_ML, MP, HL, ML);	// fix a HP-MP bug here
			addCode(TemplateType::TArc, code, fv);

			code = fe->genCodePWWF(Arc::HP_HL_ML, HP, HL, ML);
			addCode(TemplateType::TArc, code, fv);

			//code = fe->genCodePPWWF(Arc::HP_HL_MP_ML, HP, MP, HL, ML);
			//addCode(TemplateType::TArc, code, fv);
		}
	}
	else if (options->lang == PossibleLang::Arabic || options->lang == PossibleLang::SPMRL) {
		int HL = headEle.lemmaid;
		int ML = modEle.lemmaid;

		code = fe->genCodeWF(Arc::HL, HL);
		addCode(TemplateType::TArc, code, fv);
		addCode(TemplateType::TArc, code | distFlag, fv);

		code = fe->genCodePWF(Arc::HP_ML, HP, ML);
		addCode(TemplateType::TArc, code, fv);
		addCode(TemplateType::TArc, code | distFlag, fv);

		code = fe->genCodePWF(Arc::HL_MP, MP, HL);
		addCode(TemplateType::TArc, code, fv);
		addCode(TemplateType::TArc, code | distFlag, fv);

		code = fe->genCodeWWF(Arc::HL_ML, HL, ML);
		addCode(TemplateType::TArc, code, fv);
		addCode(TemplateType::TArc, code | distFlag, fv);

		code = fe->genCodePWF(Arc::HP_HL, HP, HL);
		addCode(TemplateType::TArc, code, fv);
		addCode(TemplateType::TArc, code | distFlag, fv);

		code = fe->genCodePPWF(Arc::HP_MP_ML, HP, MP, ML);
		addCode(TemplateType::TArc, code, fv);
		addCode(TemplateType::TArc, code | distFlag, fv);

		code = fe->genCodePPWF(Arc::HP_HL_MP, HP, MP, HL);
		addCode(TemplateType::TArc, code, fv);
		addCode(TemplateType::TArc, code | distFlag, fv);

		code = fe->genCodePWWF(Arc::HL_MP_ML, MP, HL, ML);	// fix a HP-MP bug here
		addCode(TemplateType::TArc, code, fv);
		addCode(TemplateType::TArc, code | distFlag, fv);

		code = fe->genCodePWWF(Arc::HP_HL_ML, HP, HL, ML);
		addCode(TemplateType::TArc, code, fv);
		addCode(TemplateType::TArc, code | distFlag, fv);

		code = fe->genCodePPWWF(Arc::HP_HL_MP_ML, HP, MP, HL, ML);
		addCode(TemplateType::TArc, code, fv);
		addCode(TemplateType::TArc, code | distFlag, fv);

		if (options->lang == PossibleLang::SPMRL) {
			// morphology
			SegInstance& headSegInst = inst->word[headIndex.hWord].getCurrSeg();
			SegInstance& modSegInst = inst->word[modIndex.hWord].getCurrSeg();
			if (headSegInst.morphIndex == headIndex.hSeg
					&& modSegInst.morphIndex == modIndex.hSeg) {
				assert(headSegInst.morphid.size() > 0 && modSegInst.morphid.size() > 0);

				for (unsigned int fh = 0; fh < headSegInst.morphid.size(); ++fh) {
					for (unsigned int fc = 0; fc < modSegInst.morphid.size(); ++fc) {
						int IDH = fh;
						int IDM = fc;
						int HV = headSegInst.morphid[fh];
						int MV = modSegInst.morphid[fc];

						code = fe->genCodeIIVF(Arc::FF_IDH_IDM_HV, IDH, IDM, HV);
						addCode(TemplateType::TArc, code, fv);
						addCode(TemplateType::TArc, code | distFlag, fv);

						code = fe->genCodeIIVF(Arc::FF_IDH_IDM_MV, IDH, IDM, MV);
						addCode(TemplateType::TArc, code, fv);
						addCode(TemplateType::TArc, code | distFlag, fv);

						code = fe->genCodeIIVPF(Arc::FF_IDH_IDM_HP_MV, IDH, IDM, MV, HP);
						addCode(TemplateType::TArc, code, fv);
						addCode(TemplateType::TArc, code | distFlag, fv);

						code = fe->genCodeIIVPF(Arc::FF_IDH_IDM_HV_MP, IDH, IDM, HV, MP);
						addCode(TemplateType::TArc, code, fv);
						addCode(TemplateType::TArc, code | distFlag, fv);

						code = fe->genCodeIIVVF(Arc::FF_IDH_IDM_HV_MV, IDH, IDM, HV, MV);
						addCode(TemplateType::TArc, code, fv);
						addCode(TemplateType::TArc, code | distFlag, fv);

						code = fe->genCodeIIVPF(Arc::FF_IDH_IDM_HV_HP, IDH, IDM, HV, HP);
						addCode(TemplateType::TArc, code, fv);
						addCode(TemplateType::TArc, code | distFlag, fv);

						code = fe->genCodeIIVPF(Arc::FF_IDH_IDM_MV_MP, IDH, IDM, MV, MP);
						addCode(TemplateType::TArc, code, fv);
						addCode(TemplateType::TArc, code | distFlag, fv);

						code = fe->genCodeIIVPPF(Arc::FF_IDH_IDM_HP_MP_MV, IDH, IDM, MV, HP, MP);
						addCode(TemplateType::TArc, code, fv);
						addCode(TemplateType::TArc, code | distFlag, fv);

						code = fe->genCodeIIVPPF(Arc::FF_IDH_IDM_HP_HV_MP, IDH, IDM, HV, HP, MP);
						addCode(TemplateType::TArc, code, fv);
						addCode(TemplateType::TArc, code | distFlag, fv);

						code = fe->genCodeIIVVPF(Arc::FF_IDH_IDM_HV_MP_MV, IDH, IDM, HV, MV, MP);
						addCode(TemplateType::TArc, code, fv);
						addCode(TemplateType::TArc, code | distFlag, fv);

						code = fe->genCodeIIVVPF(Arc::FF_IDH_IDM_HP_HV_MV, IDH, IDM, HV, MV, HP);
						addCode(TemplateType::TArc, code, fv);
						addCode(TemplateType::TArc, code | distFlag, fv);

						code = fe->genCodeIIVVPPF(Arc::FF_IDH_IDM_HP_HV_MP_MV, IDH, IDM, HV, MV, HP, MP);
						addCode(TemplateType::TArc, code, fv);
						addCode(TemplateType::TArc, code | distFlag, fv);
					}
				}
			}

			// det pos
			int HD = headEle.getCurrDetPos();
			int MD = modEle.getCurrDetPos();

			int pHD = headSegIndex > 0 ? inst->getElement(inst->segToWord(headSegIndex - 1)).getCurrDetPos() : ConstPosLex::START;
			pHD = headSegIndex == modSegIndex + 1 ? ConstPosLex::MID : pLP;

			int nHD = headSegIndex < len - 1 ? inst->getElement(inst->segToWord(headSegIndex + 1)).getCurrDetPos() : ConstPosLex::END;
			nHD = headSegIndex == modSegIndex - 1 ? ConstPosLex::MID : nLP;

			int pMD = modSegIndex > 0 ? inst->getElement(inst->segToWord(modSegIndex - 1)).getCurrDetPos() : ConstPosLex::START;
			pMD = modSegIndex == headSegIndex + 1 ? ConstPosLex::MID : pRP;

			int nMD = modSegIndex < len - 1 ? inst->getElement(inst->segToWord(modSegIndex + 1)).getCurrDetPos() : ConstPosLex::END;
			nMD = modSegIndex == headSegIndex - 1 ? ConstPosLex::MID : nRP;

			int small = headSegIndex < modSegIndex ? headSegIndex : modSegIndex;
			int large = headSegIndex > modSegIndex ? headSegIndex : modSegIndex;
			// feature posR posMid posL
			for (int i = small + 1; i < large; ++i) {
				int BD = inst->getElement(inst->segToWord(i)).getCurrDetPos();
				code = fe->genCodePPPF(Arc::HD_BD_MD, HD, BD, MD);
				addCode(TemplateType::TArc, code, fv);
				addCode(TemplateType::TArc, code | distFlag, fv);
			}

			code = fe->genCodePPPPF(Arc::pHD_HD_MD_nMD, pHD, HD, MD, nMD);
			addCode(TemplateType::TArc, code, fv);
			addCode(TemplateType::TArc, code | distFlag, fv);

			code = fe->genCodePPPF(Arc::HD_MD_nMD, HD, MD, nMD);
			addCode(TemplateType::TArc, code, fv);
			addCode(TemplateType::TArc, code | distFlag, fv);

			code = fe->genCodePPPF(Arc::pHD_MD_nMD, pHD, MD, nMD);
			addCode(TemplateType::TArc, code, fv);
			addCode(TemplateType::TArc, code | distFlag, fv);

			code = fe->genCodePPPF(Arc::pHD_HD_nMD, pHD, HD, nMD);
			addCode(TemplateType::TArc, code, fv);
			addCode(TemplateType::TArc, code | distFlag, fv);

			code = fe->genCodePPPF(Arc::pHD_HD_MD, pHD, HD, MD);
			addCode(TemplateType::TArc, code, fv);
			addCode(TemplateType::TArc, code | distFlag, fv);

			code = fe->genCodePPPPF(Arc::HD_nHD_pMD_MD, HD, nHD, pMD, MD);
			addCode(TemplateType::TArc, code, fv);
			addCode(TemplateType::TArc, code | distFlag, fv);

			code = fe->genCodePPPF(Arc::nHD_pMD_MD, nHD, pMD, MD);
			addCode(TemplateType::TArc, code, fv);
			addCode(TemplateType::TArc, code | distFlag, fv);

			code = fe->genCodePPPF(Arc::HD_pMD_MD, HD, pMD, MD);
			addCode(TemplateType::TArc, code, fv);
			addCode(TemplateType::TArc, code | distFlag, fv);

			code = fe->genCodePPPF(Arc::HD_nHD_MD, HD, nHD, MD);
			addCode(TemplateType::TArc, code, fv);
			addCode(TemplateType::TArc, code | distFlag, fv);

			code = fe->genCodePPPF(Arc::HD_nHD_pMD, HD, nHD, pMD);
			addCode(TemplateType::TArc, code, fv);
			addCode(TemplateType::TArc, code | distFlag, fv);

			code = fe->genCodePPPPF(Arc::pHD_HD_pMD_MD, pHD, HD, pMD, MD);
			addCode(TemplateType::TArc, code, fv);
			addCode(TemplateType::TArc, code | distFlag, fv);

			code = fe->genCodePPPPF(Arc::HD_nHD_MD_nMD, HD, nHD, MD, nMD);
			addCode(TemplateType::TArc, code, fv);
			addCode(TemplateType::TArc, code | distFlag, fv);

			code = fe->genCodePF(Arc::HD, HD);
			addCode(TemplateType::TArc, code, fv);
			addCode(TemplateType::TArc, code | distFlag, fv);

			code = fe->genCodePPF(Arc::HD_MD, HD, MD);
			addCode(TemplateType::TArc, code, fv);
			addCode(TemplateType::TArc, code | distFlag, fv);

		}

	}
/*
	// contextual
	int pHW = headSegIndex > 0 ? inst->getElement(inst->segToWord(headSegIndex - 1)).formid: ConstPosLex::START;
	pHW = headSegIndex == modSegIndex + 1 ? ConstPosLex::MID : pLP;

	int nHW = headSegIndex < len - 1 ? inst->getElement(inst->segToWord(headSegIndex + 1)).formid : ConstPosLex::END;
	nHW = headSegIndex == modSegIndex - 1 ? ConstPosLex::MID : nLP;

	int pMW = modSegIndex > 0 ? inst->getElement(inst->segToWord(modSegIndex - 1)).formid : ConstPosLex::START;
	pMW = modSegIndex == headSegIndex + 1 ? ConstPosLex::MID : pRP;

	int nMW = modSegIndex < len - 1 ? inst->getElement(inst->segToWord(modSegIndex + 1)).formid : ConstPosLex::END;
	nMW = modSegIndex == headSegIndex - 1 ? ConstPosLex::MID : nRP;

	code = fe->genCodeWF(Arc::pHW, pHW);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodeWF(Arc::nHW, nHW);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodeWF(Arc::pMW, pMW);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodeWF(Arc::MW, MW);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodeWF(Arc::nMW, nMW);
	addCode(TemplateType::TArc, code | distFlag, fv);
*/
	int flagVerb = 0x1;
	int flagCoord = 0x2;
	int flagPunc = 0x3;
	int verbNum = 0;
	int coordNum = 0;
	int puncNum = 0;
	for (int i = small + 1; i < large; ++i) {
		int specialPos = inst->getElement(inst->segToWord(i)).getCurrSpecialPos();
		if (SpecialPos::V == specialPos)
			verbNum++;
		else if (SpecialPos::C == specialPos)
			coordNum++;
		else if (SpecialPos::PNX == specialPos)
			puncNum++;
	}
	flagVerb = (flagVerb << 4) | getBinnedDistance(verbNum);
	flagCoord = (flagCoord << 4) | getBinnedDistance(coordNum);
	flagPunc = (flagPunc << 4) | getBinnedDistance(puncNum);

	code = fe->genCodePPPF(Arc::HP_MP_FLAG, HP, MP, flagVerb);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodePPPF(Arc::HP_MP_FLAG, HP, MP, flagCoord);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodePPPF(Arc::HP_MP_FLAG, HP, MP, flagPunc);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodePWWF(Arc::HW_MW_FLAG, flagVerb, HW, MW);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodePWWF(Arc::HW_MW_FLAG, flagCoord, HW, MW);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

	code = fe->genCodePWWF(Arc::HW_MW_FLAG, flagPunc, HW, MW);
	addCode(TemplateType::TArc, code, fv);
	addCode(TemplateType::TArc, code | distFlag, fv);

}

void DependencyPipe::createTripsFeatureVector(DependencyInstance* inst,
		HeadIndex& par, HeadIndex& ch1, HeadIndex& ch2, FeatureVector* fv) {

	// ch1 is always the closes to par
	int dirFlag = (((par < ch2 ? 0 : 1) << 1) | 1) << fe->tempOff;

	SegElement& parEle = inst->getElement(par);
	SegElement& ch1Ele = inst->getElement(ch1);
	SegElement& ch2Ele = inst->getElement(ch2);

	int HC = parEle.getCurrPos();
	int SC = ch1 == par ? ConstPosLex::START : ch1Ele.getCurrPos();
	int MC = ch2Ele.getCurrPos();

	long code = 0;

	code = fe->genCodePPPF(SecondOrder::HC_SC_MC, HC, SC, MC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	int HL = parEle.lemmaid;
	int SL = ch1 == par ? ConstPosLex::START : ch1Ele.lemmaid;
	int ML = ch2Ele.lemmaid;

	int len = inst->getNumSeg();
	int parIdx = inst->wordToSeg(par);
	int ch1Idx = inst->wordToSeg(ch1);
	int ch2Idx = inst->wordToSeg(ch2);

	int pHC = parIdx > 0 ? inst->getElement(inst->segToWord(parIdx - 1)).getCurrPos() : ConstPosLex::START;
	int nHC = parIdx < len - 1 ? inst->getElement(inst->segToWord(parIdx + 1)).getCurrPos() : ConstPosLex::END;
	int pSC = ch1Idx > 0 ? inst->getElement(inst->segToWord(ch1Idx - 1)).getCurrPos() : ConstPosLex::START;
	int nSC = ch1Idx < len - 1 ? inst->getElement(inst->segToWord(ch1Idx + 1)).getCurrPos() : ConstPosLex::END;
	int pMC = ch2Idx > 0 ? inst->getElement(inst->segToWord(ch2Idx - 1)).getCurrPos() : ConstPosLex::START;
	int nMC = ch2Idx < len - 1 ? inst->getElement(inst->segToWord(ch2Idx + 1)).getCurrPos() : ConstPosLex::END;

	// CCC
	code = fe->genCodePPPPF(SecondOrder::pHC_HC_SC_MC, pHC, HC, SC, MC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPF(SecondOrder::HC_nHC_SC_MC, HC, nHC, SC, MC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPF(SecondOrder::HC_pSC_SC_MC, HC, pSC, SC, MC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPF(SecondOrder::HC_SC_nSC_MC, HC, SC, nSC, MC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPF(SecondOrder::HC_SC_pMC_MC, HC, SC, pMC, MC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPF(SecondOrder::HC_SC_MC_nMC, HC, SC, MC, nMC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	// LCC
	code = fe->genCodePPPWF(SecondOrder::pHC_HL_SC_MC, pHC, SC, MC, HL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::HL_nHC_SC_MC, nHC, SC, MC, HL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::HL_pSC_SC_MC, pSC, SC, MC, HL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::HL_SC_nSC_MC, SC, nSC, MC, HL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::HL_SC_pMC_MC, SC, pMC, MC, HL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::HL_SC_MC_nMC, SC, MC, nMC, HL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	// CLC
	code = fe->genCodePPPWF(SecondOrder::pHC_HC_SL_MC, pHC, HC, MC, SL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::HC_nHC_SL_MC, HC, nHC, MC, SL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::HC_pSC_SL_MC, HC, pSC, MC, SL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::HC_SL_nSC_MC, HC, nSC, MC, SL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::HC_SL_pMC_MC, HC, pMC, MC, SL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::HC_SL_MC_nMC, HC, MC, nMC, SL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	// CCL
	code = fe->genCodePPPWF(SecondOrder::pHC_HC_SC_ML, pHC, HC, SC, ML);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::HC_nHC_SC_ML, HC, nHC, SC, ML);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::HC_pSC_SC_ML, HC, pSC, SC, ML);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::HC_SC_nSC_ML, HC, SC, nSC, ML);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::HC_SC_pMC_ML, HC, SC, pMC, ML);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::HC_SC_ML_nMC, HC, SC, nMC, ML);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	// large
	/*
	code = fe->genCodePPPPPF(SecondOrder::HC_MC_SC_pHC_pMC, HC, MC, SC, pHC, pMC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPPF(SecondOrder::HC_MC_SC_pHC_pSC, HC, MC, SC, pHC, pSC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPPF(SecondOrder::HC_MC_SC_pMC_pSC, HC, MC, SC, pMC, pSC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPPF(SecondOrder::HC_MC_SC_nHC_nMC, HC, MC, SC, nHC, nMC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPPF(SecondOrder::HC_MC_SC_nHC_nSC, HC, MC, SC, nHC, nSC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPPF(SecondOrder::HC_MC_SC_nMC_nSC, HC, MC, SC, nMC, nSC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPPF(SecondOrder::HC_MC_SC_pHC_nMC, HC, MC, SC, pHC, nMC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPPF(SecondOrder::HC_MC_SC_pHC_nSC, HC, MC, SC, pHC, nSC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPPF(SecondOrder::HC_MC_SC_pMC_nSC, HC, MC, SC, pMC, nSC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPPF(SecondOrder::HC_MC_SC_nHC_pMC, HC, MC, SC, nHC, pMC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPPF(SecondOrder::HC_MC_SC_nHC_pSC, HC, MC, SC, nHC, pSC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPPF(SecondOrder::HC_MC_SC_nMC_pSC, HC, MC, SC, nMC, pSC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);
	*/
}

void DependencyPipe::createSibsFeatureVector(DependencyInstance* inst,
		HeadIndex& ch1, HeadIndex& ch2, bool isST, FeatureVector* fv) {
	// ch1 is always the closes to par

	SegElement& ch1Ele = inst->getElement(ch1);
	SegElement& ch2Ele = inst->getElement(ch2);

	int SC = isST ? ConstPosLex::START : ch1Ele.getCurrPos();
	int MC = ch2Ele.getCurrPos();
	int SP = SC;
	int MP = MC;
	int SL = isST ? ConstPosLex::START : ch1Ele.lemmaid;
	int ML = ch2Ele.lemmaid;
	int SW = isST ? ConstPosLex::START : ch1Ele.formid;
	int MW = ch2Ele.formid;

	int flag = getBinnedDistance(inst->segDist(ch1, ch2)) << fe->tempOff;

	long code = 0;

	code = fe->genCodePPF(SecondOrder::SP_MP, SP, MP);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | flag, fv);

	code = fe->genCodeWWF(SecondOrder::SW_MW, SW, MW);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | flag, fv);

	code = fe->genCodePWF(SecondOrder::SW_MP, MP, SW);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | flag, fv);

	code = fe->genCodePWF(SecondOrder::SP_MW, SP, MW);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | flag, fv);

	code = fe->genCodePPF(SecondOrder::SC_MC, SC, MC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | flag, fv);

	code = fe->genCodeWWF(SecondOrder::SL_ML, SL, ML);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | flag, fv);

	code = fe->genCodePWF(SecondOrder::SL_MC, MC, SL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | flag, fv);

	code = fe->genCodePWF(SecondOrder::SC_ML, SC, ML);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | flag, fv);

}

void DependencyPipe::createGPCFeatureVector(DependencyInstance* inst,
		HeadIndex& gp, HeadIndex& par, HeadIndex& c, FeatureVector* fv) {

	int flag = (((((gp < par ? 0 : 1) << 1) | (par < c ? 0 : 1)) << 1) | 1) << fe->tempOff;

	SegElement& gpEle = inst->getElement(gp);
	SegElement& parEle = inst->getElement(par);
	SegElement& cEle = inst->getElement(c);

	int GC = gpEle.getCurrPos();
	int HC = parEle.getCurrPos();
	int MC = cEle.getCurrPos();
	int GL = gpEle.lemmaid;
	int HL = parEle.lemmaid;
	int ML = cEle.lemmaid;

	int len = inst->getNumSeg();
	int gpIdx = inst->wordToSeg(gp);
	int parIdx = inst->wordToSeg(par);
	int cIdx = inst->wordToSeg(c);

	int pGC = gpIdx > 0 ? inst->getElement(inst->segToWord(gpIdx - 1)).getCurrPos() : ConstPosLex::START;
	int nGC = gpIdx < len - 1 ? inst->getElement(inst->segToWord(gpIdx + 1)).getCurrPos() : ConstPosLex::END;
	int pHC = parIdx > 0 ? inst->getElement(inst->segToWord(parIdx - 1)).getCurrPos() : ConstPosLex::START;
	int nHC = parIdx < len - 1 ? inst->getElement(inst->segToWord(parIdx + 1)).getCurrPos() : ConstPosLex::END;
	int pMC = cIdx > 0 ? inst->getElement(inst->segToWord(cIdx - 1)).getCurrPos() : ConstPosLex::START;
	int nMC = cIdx < len - 1 ? inst->getElement(inst->segToWord(cIdx + 1)).getCurrPos() : ConstPosLex::END;

	long code = 0;

	code = fe->genCodePPPF(SecondOrder::GC_HC_MC, GC, HC, MC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | flag, fv);

	code = fe->genCodePPWF(SecondOrder::GL_HC_MC, HC, MC, GL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | flag, fv);

	code = fe->genCodePPWF(SecondOrder::GC_HL_MC, GC, MC, HL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | flag, fv);

	code = fe->genCodePPWF(SecondOrder::GC_HC_ML, GC, HC, ML);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | flag, fv);

	int dirFlag = flag;

	// CCC
	code = fe->genCodePPPPF(SecondOrder::pGC_GC_HC_MC, pGC, GC, HC, MC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPF(SecondOrder::GC_nGC_HC_MC, GC, nGC, HC, MC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPF(SecondOrder::GC_pHC_HC_MC, GC, pHC, HC, MC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPF(SecondOrder::GC_HC_nHC_MC, GC, HC, nHC, MC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPF(SecondOrder::GC_HC_pMC_MC, GC, HC, pMC, MC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPF(SecondOrder::GC_HC_MC_nMC, GC, HC, MC, nMC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	// LCC
	code = fe->genCodePPPWF(SecondOrder::pGC_GL_HC_MC, pGC, HC, MC, GL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::GL_nGC_HC_MC, nGC, HC, MC, GL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::GL_pHC_HC_MC, pHC, HC, MC, GL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::GL_HC_nHC_MC, HC, nHC, MC, GL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::GL_HC_pMC_MC, HC, pMC, MC, GL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::GL_HC_MC_nMC, HC, MC, nMC, GL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	// CLC
	code = fe->genCodePPPWF(SecondOrder::pGC_GC_HL_MC, pGC, GC, MC, HL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::GC_nGC_HL_MC, GC, nGC, MC, HL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::GC_pHC_HL_MC, GC, pHC, MC, HL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::GC_HL_nHC_MC, GC, nHC, MC, HL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::GC_HL_pMC_MC, GC, pMC, MC, HL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::GC_HL_MC_nMC, GC, MC, nMC, HL);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	// CCL
	code = fe->genCodePPPWF(SecondOrder::pGC_GC_HC_ML, pGC, GC, HC, ML);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::GC_nGC_HC_ML, GC, nGC, HC, ML);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::GC_pHC_HC_ML, GC, pHC, HC, ML);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::GC_HC_nHC_ML, GC, HC, nHC, ML);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::GC_HC_pMC_ML, GC, HC, pMC, ML);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPWF(SecondOrder::GC_HC_ML_nMC, GC, HC, nMC, ML);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	// large
	/*
	code = fe->genCodePPPPPF(SecondOrder::GC_HC_MC_pGC_pHC, GC, HC, MC, pGC, pHC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPPF(SecondOrder::GC_HC_MC_pGC_pMC, GC, HC, MC , pGC, pMC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPPF(SecondOrder::GC_HC_MC_pHC_pMC, GC, HC, MC, pHC, pMC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPPF(SecondOrder::GC_HC_MC_nGC_nHC, GC, HC, MC, nGC, nHC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPPF(SecondOrder::GC_HC_MC_nGC_nMC, GC, HC, MC, nGC, nMC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPPF(SecondOrder::GC_HC_MC_nHC_nMC, GC, HC, MC, nHC, nMC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPPF(SecondOrder::GC_HC_MC_pGC_nHC, GC, HC, MC, pGC, nHC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPPF(SecondOrder::GC_HC_MC_pGC_nMC, GC, HC, MC, pGC, nMC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPPF(SecondOrder::GC_HC_MC_pHC_nMC, GC, HC, MC, pHC, nMC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPPF(SecondOrder::GC_HC_MC_nGC_pHC, GC, HC, MC, nGC, pHC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPPF(SecondOrder::GC_HC_MC_nGC_pMC, GC, HC, MC, nGC, pMC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);

	code = fe->genCodePPPPPF(SecondOrder::GC_HC_MC_nHC_pMC, GC, HC, MC, nHC, pMC);
	addCode(TemplateType::TSecondOrder, code, fv);
	addCode(TemplateType::TSecondOrder, code | dirFlag, fv);
	*/

	code = fe->genCodePWWF(ThirdOrder::GL_HL_MC, MC, GL, HL);
	addCode(TemplateType::TThirdOrder, code, fv);
	addCode(TemplateType::TThirdOrder, code | dirFlag, fv);

	code = fe->genCodePWWF(ThirdOrder::GL_HC_ML, HC, GL, ML);
	addCode(TemplateType::TThirdOrder, code, fv);
	addCode(TemplateType::TThirdOrder, code | dirFlag, fv);

	code = fe->genCodePWWF(ThirdOrder::GC_HL_ML, GC, HL, ML);
	addCode(TemplateType::TThirdOrder, code, fv);
	addCode(TemplateType::TThirdOrder, code | dirFlag, fv);

	code = fe->genCodeWWW(ThirdOrder::GL_HL_ML, GL, HL, ML);
	addCode(TemplateType::TThirdOrder, code, fv);

	code = fe->genCodePPF(ThirdOrder::GC_HC, GC, HC);
	addCode(TemplateType::TThirdOrder, code, fv);
	addCode(TemplateType::TThirdOrder, code | dirFlag, fv);

	code = fe->genCodePWF(ThirdOrder::GL_HC, HC, GL);
	addCode(TemplateType::TThirdOrder, code, fv);
	addCode(TemplateType::TThirdOrder, code | dirFlag, fv);

	code = fe->genCodePWF(ThirdOrder::GC_HL, GC, HL);
	addCode(TemplateType::TThirdOrder, code, fv);
	addCode(TemplateType::TThirdOrder, code | dirFlag, fv);

	code = fe->genCodeWWF(ThirdOrder::GL_HL, GL, HL);
	addCode(TemplateType::TThirdOrder, code, fv);
	addCode(TemplateType::TThirdOrder, code | dirFlag, fv);

	code = fe->genCodePPF(ThirdOrder::GC_MC, GC, MC);
	addCode(TemplateType::TThirdOrder, code | dirFlag, fv);

	code = fe->genCodePWF(ThirdOrder::GL_MC, MC, GL);
	addCode(TemplateType::TThirdOrder, code | dirFlag, fv);

	code = fe->genCodePWF(ThirdOrder::GC_ML, GC, ML);
	addCode(TemplateType::TThirdOrder, code | dirFlag, fv);

	code = fe->genCodeWWF(ThirdOrder::GL_ML, GL, ML);
	addCode(TemplateType::TThirdOrder, code | dirFlag, fv);

	code = fe->genCodePPF(ThirdOrder::HC_MC, HC, MC);
	addCode(TemplateType::TThirdOrder, code | dirFlag, fv);

	code = fe->genCodePWF(ThirdOrder::HL_MC, MC, HL);
	addCode(TemplateType::TThirdOrder, code | dirFlag, fv);

	code = fe->genCodePWF(ThirdOrder::HC_ML, HC, ML);
	addCode(TemplateType::TThirdOrder, code | dirFlag, fv);

	code = fe->genCodeWWF(ThirdOrder::HL_ML, HL, ML);
	addCode(TemplateType::TThirdOrder, code | dirFlag, fv);

}

void DependencyPipe::createGPSibFeatureVector(DependencyInstance* inst,
		SegElement* gp, SegElement* par, SegElement* ch1, SegElement* ch2, FeatureVector* fv) {

	int GC = gp->getCurrPos();
	int HC = par->getCurrPos();
	int SC = ch1 == par ? ConstPosLex::START : ch1->getCurrPos();
	int MC = ch2->getCurrPos();
	int GL = gp->formid;
	int HL = par->formid;
	int SL = ch1 == par ? ConstPosLex::START : ch1->formid;
	int ML = ch2->formid;

	long code = 0;

	code = fe->genCodePPPPF(ThirdOrder::GC_HC_MC_SC, GC, HC, SC, MC);
	addCode(TemplateType::TThirdOrder, code, fv);

	code = fe->genCodePPPWF(ThirdOrder::GC_HL_MC_SC, GC, SC, MC, HL);
	addCode(TemplateType::TThirdOrder, code, fv);

	code = fe->genCodePPPWF(ThirdOrder::GC_HC_ML_SC, GC, SC, HC, ML);
	addCode(TemplateType::TThirdOrder, code, fv);

	code = fe->genCodePPPWF(ThirdOrder::GL_HC_MC_SC, HC, SC, MC, GL);
	addCode(TemplateType::TThirdOrder, code, fv);

	code = fe->genCodePPPWF(ThirdOrder::GC_HC_MC_SL, GC, HC, MC, SL);
	addCode(TemplateType::TThirdOrder, code, fv);
}

void DependencyPipe::createTriSibFeatureVector(DependencyInstance* inst,
		SegElement* par, SegElement* ch1, SegElement* ch2, SegElement* ch3, FeatureVector* fv) {

	int HC = par->getCurrPos();
	int SC = ch1 == par ? ConstPosLex::START : ch1->getCurrPos();
	int MC = ch2->getCurrPos();
	int CC = ch3->getCurrPos();
	int HL = par->formid;
	int SL = ch1 == par ? ConstPosLex::START : ch1->formid;
	int ML = ch2->formid;
	int CL = ch3->formid;

	long code = 0;

	code = fe->genCodePPPPF(ThirdOrder::HC_MC_CC_SC, HC, MC, CC, SC);
	addCode(TemplateType::TThirdOrder, code, fv);

	code = fe->genCodePPPWF(ThirdOrder::HL_MC_CC_SC, MC, CC, SC, HL);
	addCode(TemplateType::TThirdOrder, code, fv);

	code = fe->genCodePPPWF(ThirdOrder::HC_ML_CC_SC, HC, CC, SC, ML);
	addCode(TemplateType::TThirdOrder, code, fv);

	code = fe->genCodePPPWF(ThirdOrder::HC_MC_CL_SC, MC, HC, SC, CL);
	addCode(TemplateType::TThirdOrder, code, fv);

	code = fe->genCodePPPWF(ThirdOrder::HC_MC_CC_SL, MC, CC, HC, SL);
	addCode(TemplateType::TThirdOrder, code, fv);
}

void DependencyPipe::createPos1OFeatureVector(DependencyInstance* inst, HeadIndex& m, FeatureVector* fv) {
	//int len = inst->getNumSeg();
	//int idx = inst->wordToSeg(m);
	SegElement& ele = inst->getElement(m);
	int P = ele.getCurrPos();

	int L = ele.formid;

	long code = 0;

	code = fe->genCodePWF(HighOrder::L_P, P, L);
	addCode(TemplateType::THighOrder, code, fv);

	code = fe->genCodePF(HighOrder::POS_PROB, 1);
	addCode(TemplateType::THighOrder, code, ele.candProb[ele.currPosCandID], fv);

	code = fe->genCodePF(HighOrder::P_POS_PROB, P);
	addCode(TemplateType::THighOrder, code, ele.candProb[ele.currPosCandID], fv);

	code = fe->genCodeWF(HighOrder::W_POS_PROB, L);
	addCode(TemplateType::THighOrder, code, ele.candProb[ele.currPosCandID], fv);

	if (options->lang == PossibleLang::Chinese) {

		if (ele.st >= 0 && ele.en > ele.st) {
			code = fe->genCodePPWF(HighOrder::P_PRE, P, 1, inst->characterid[ele.st]);
			addCode(TemplateType::THighOrder, code, fv);

			if (ele.st + 1 < ele.en) {
				code = fe->genCodePPWWF(HighOrder::P_PRE, P, 2, inst->characterid[ele.st], inst->characterid[ele.st + 1]);
				addCode(TemplateType::THighOrder, code, fv);
			}

			code = fe->genCodePPWF(HighOrder::P_SUF, P, 1, inst->characterid[ele.en - 1]);
			addCode(TemplateType::THighOrder, code, fv);

			if (ele.en - 2 >= ele.st) {
				code = fe->genCodePPWWF(HighOrder::P_SUF, P, 2, inst->characterid[ele.en - 2], inst->characterid[ele.en - 1]);
				addCode(TemplateType::THighOrder, code, fv);
			}
		}

		code = fe->genCodePPF(HighOrder::P_LENGTH, P, min(5, ele.en - ele.st));
		addCode(TemplateType::THighOrder, code, fv);

		//for (int i = ele.st + 1; i < ele.en; ++i) {
			//code = fe->genCodePWWF(HighOrder::P_MID_C_pC, P, inst->characterid[i - 1], inst->characterid[i]);
			//addCode(TemplateType::THighOrder, code, fv);

			//code = fe->genCodePWWF(HighOrder::P_C_C0, P, inst->characterid[ele.st], inst->characterid[i]);
			//addCode(TemplateType::THighOrder, code, fv);
		//}

		for (int i = ele.st; i < ele.en; ++i) {
			code = fe->genCodePWF(HighOrder::P_C0, P, inst->characterid[i]);
			addCode(TemplateType::THighOrder, code, fv);
		}
	}

}

void DependencyPipe::createPosHOFeatureVector(DependencyInstance* inst, HeadIndex& m, bool unigram, FeatureVector* fv) {
	int len = inst->getNumSeg();
	int idx = inst->wordToSeg(m);
	SegElement& ele = inst->getElement(m);

	int P = ele.getCurrPos();

	int ppL = idx > 1 ? inst->getElement(inst->segToWord(idx - 2)).formid : ConstPosLex::START;
	int pL = idx > 0 ? inst->getElement(inst->segToWord(idx - 1)).formid : ConstPosLex::START;
	int L = ele.formid;
	int nL = idx < len - 1 ? inst->getElement(inst->segToWord(idx + 1)).formid : ConstPosLex::END;
	int nnL = idx < len - 2 ? inst->getElement(inst->segToWord(idx + 2)).formid : ConstPosLex::END;

	long code = 0;

	code = fe->genCodePWF(HighOrder::ppL_P, P, ppL);
	addCode(TemplateType::THighOrder, code, fv);

	code = fe->genCodePWF(HighOrder::pL_P, P, pL);
	addCode(TemplateType::THighOrder, code, fv);

	code = fe->genCodePWF(HighOrder::P_nL, P, nL);
	addCode(TemplateType::THighOrder, code, fv);

	code = fe->genCodePWF(HighOrder::P_nnL, P, nnL);
	addCode(TemplateType::THighOrder, code, fv);
	if (options->lang == PossibleLang::Chinese) {

		code = fe->genCodePWWF(HighOrder::pL_P_L, P, pL, L);
		addCode(TemplateType::THighOrder, code, fv);

		code = fe->genCodePWWF(HighOrder::P_L_nL, P, L, nL);
		addCode(TemplateType::THighOrder, code, fv);

		//code = fe->genCodePWWF(HighOrder::pL_P_nL, P, pL, nL);
		//addCode(TemplateType::THighOrder, code, fv);

		if (ele.st >= 0) {
			int pC = ele.st > 0 ? inst->characterid[ele.st - 1] : ConstPosLex::START;
			code = fe->genCodePWWF(HighOrder::P_START_C_pC, P, inst->characterid[ele.st], pC);
			addCode(TemplateType::THighOrder, code, fv);
		}
	}

	if (!unigram) {

		int ppP = idx > 1 ? inst->getElement(inst->segToWord(idx - 2)).getCurrPos() : ConstPosLex::START;
		int pP = idx > 0 ? inst->getElement(inst->segToWord(idx - 1)).getCurrPos() : ConstPosLex::START;
		int nP = idx < len - 1 ? inst->getElement(inst->segToWord(idx + 1)).getCurrPos() : ConstPosLex::END;
		int nnP = idx < len - 2 ? inst->getElement(inst->segToWord(idx + 2)).getCurrPos() : ConstPosLex::END;

		if (options->lang == PossibleLang::Chinese && ele.st >= 0) {
			int pC = ele.st > 0 ? inst->characterid[ele.st - 1] : ConstPosLex::START;
			code = fe->genCodePPWWF(HighOrder::pP_P_pC_C, pP, P, inst->characterid[ele.st], pC);
			addCode(TemplateType::THighOrder, code, fv);
		}

		code = fe->genCodePPF(HighOrder::ppP_P, ppP, P);
		addCode(TemplateType::THighOrder, code, fv);

		code = fe->genCodePPF(HighOrder::pP_P, pP, P);
		addCode(TemplateType::THighOrder, code, fv);

		code = fe->genCodePPF(HighOrder::P_nP, P, nP);
		addCode(TemplateType::THighOrder, code, fv);

		code = fe->genCodePPF(HighOrder::P_nnP, P, nnP);
		addCode(TemplateType::THighOrder, code, fv);

		code = fe->genCodePPPF(HighOrder::ppP_pP_P, ppP, pP, P);
		addCode(TemplateType::THighOrder, code, fv);

		code = fe->genCodePPPF(HighOrder::pP_P_nP, pP, P, nP);
		addCode(TemplateType::THighOrder, code, fv);

		code = fe->genCodePPPF(HighOrder::P_nP_nnP, P, nP, nnP);
		addCode(TemplateType::THighOrder, code, fv);

		code = fe->genCodePPPF(HighOrder::ppP_P_nP, ppP, P, nP);
		addCode(TemplateType::THighOrder, code, fv);

		code = fe->genCodePPPF(HighOrder::pP_P_nnP, pP, P, nnP);
		addCode(TemplateType::THighOrder, code, fv);

		code = fe->genCodePPPF(HighOrder::ppP_P_nnP, ppP, P, nnP);
		addCode(TemplateType::THighOrder, code, fv);

		code = fe->genCodePPPPF(HighOrder::ppP_pP_P_nP, ppP, pP, P, nP);
		addCode(TemplateType::THighOrder, code, fv);

		code = fe->genCodePPPPF(HighOrder::ppP_pP_P_nnP, ppP, pP, P, nnP);
		addCode(TemplateType::THighOrder, code, fv);

		code = fe->genCodePPPPF(HighOrder::pP_P_nP_nnP, pP, P, nP, nnP);
		addCode(TemplateType::THighOrder, code, fv);

		code = fe->genCodePPPPF(HighOrder::ppP_P_nP_nnP, ppP, P, nP, nnP);
		addCode(TemplateType::THighOrder, code, fv);

		code = fe->genCodePPPPPF(HighOrder::ppP_pP_P_nP_nnP, ppP, pP, P, nP, nnP);
		addCode(TemplateType::THighOrder, code, fv);

		code = fe->genCodePPWF(HighOrder::pP_L_P, pP, P, L);
		addCode(TemplateType::THighOrder, code, fv);

		code = fe->genCodePPWF(HighOrder::L_P_nP, P, nP, L);
		addCode(TemplateType::THighOrder, code, fv);

		code = fe->genCodePPPWF(HighOrder::pP_L_P_nP, pP, P, nP, L);
		addCode(TemplateType::THighOrder, code, fv);

		code = fe->genCodePPPWF(HighOrder::ppP_pP_L_P, ppP, pP, P, L);
		addCode(TemplateType::THighOrder, code, fv);

		code = fe->genCodePPPWF(HighOrder::L_P_nP_nnP, P, nP, nnP, L);
		addCode(TemplateType::THighOrder, code, fv);
	}
}

void DependencyPipe::createSegFeatureVector(DependencyInstance* inst, int wordid, FeatureVector* fv) {

	long code = 0;
	SegInstance& segInst = inst->word[wordid].getCurrSeg();

	code = fe->genCodePF(HighOrder::SEG_PROB, 0);
	addCode(TemplateType::THighOrder, code, segInst.prob, fv);

	//code = fe->genCodeWF(HighOrder::W_SEG_PROB, inst->word[wordid].wordid);
	//addCode(TemplateType::THighOrder, code, segInst.prob, fv);

	if (wordid > 0) {
		for (int i = 0; i < segInst.size(); ++i) {
			code = fe->genCodeWF(HighOrder::SEG_W, segInst.element[i].lemmaid);
			addCode(TemplateType::THighOrder, code, fv);
		}
		//for (int i = 0; i < segInst.size() - 1; ++i) {
		//	code = fe->genCodeWWF(HighOrder::SEG_P2, inst->characterid[segInst.element[i].en - 1], inst->characterid[segInst.element[i + 1].st]);
		//	addCode(TemplateType::THighOrder, code, fv);
		//}
	}


/*
	if (options->lang == PossibleLang::Chinese && wordid > 0) {
		for (int i = 0; i < segInst.size(); ++i) {
			SegElement& ele = segInst.element[i];
			for (int j = ele.st; j < ele.en; ++j) {
				int label = -1;
				if (ele.en == ele.st + 1)
					label = 0;		// S
				else if (j == ele.st)
					label = 1;		// B
				else if (j == ele.en - 1)
					label = 2;		// E
				else
					label = 3;		// M

				int U = inst->characterid[j];
				int P1 = j > 0 ? inst->characterid[j - 1] : ConstPosLex::START;
				int P2 = j > 1 ? inst->characterid[j - 2] : ConstPosLex::START;
				int P3 = j > 2 ? inst->characterid[j - 3] : ConstPosLex::START;
				int N1 = j < (int)inst->characterid.size() - 1 ? inst->characterid[j + 1] : ConstPosLex::END;
				int N2 = j < (int)inst->characterid.size() - 2 ? inst->characterid[j + 2] : ConstPosLex::END;

				code = fe->genCodePWF(HighOrder::SEG_P2, label, P2);
				addCode(TemplateType::THighOrder, code, fv);

				code = fe->genCodePWF(HighOrder::SEG_P1, label, P1);
				addCode(TemplateType::THighOrder, code, fv);

				code = fe->genCodePWF(HighOrder::SEG_U, label, U);
				addCode(TemplateType::THighOrder, code, fv);

				code = fe->genCodePWF(HighOrder::SEG_N1, label, N1);
				addCode(TemplateType::THighOrder, code, fv);

				code = fe->genCodePWF(HighOrder::SEG_N2, label, N2);
				addCode(TemplateType::THighOrder, code, fv);

				code = fe->genCodePWWF(HighOrder::SEG_P2_P1, label, P2, P1);
				addCode(TemplateType::THighOrder, code, fv);

				code = fe->genCodePWWF(HighOrder::SEG_P1_U, label, P1, U);
				addCode(TemplateType::THighOrder, code, fv);

				code = fe->genCodePWWF(HighOrder::SEG_U_N1, label, U, N1);
				addCode(TemplateType::THighOrder, code, fv);

				code = fe->genCodePWWF(HighOrder::SEG_N1_N2, label, N1, N2);
				addCode(TemplateType::THighOrder, code, fv);

				code = fe->genCodePPF(HighOrder::SEG_IP2P1, label, P2 == P1 ? 1 : 0);
				addCode(TemplateType::THighOrder, code, fv);

				code = fe->genCodePPF(HighOrder::SEG_IP1U, label, P1 == U ? 1 : 0);
				addCode(TemplateType::THighOrder, code, fv);

				code = fe->genCodePPF(HighOrder::SEG_IUN1, label, U == N1 ? 1 : 0);
				addCode(TemplateType::THighOrder, code, fv);

				code = fe->genCodePPF(HighOrder::SEG_IN1N2, label, N1 == P2 ? 1 : 0);
				addCode(TemplateType::THighOrder, code, fv);

				code = fe->genCodePPF(HighOrder::SEG_IP3P1, label, P3 == P1 ? 1 : 0);
				addCode(TemplateType::THighOrder, code, fv);

				code = fe->genCodePPF(HighOrder::SEG_IP2U, label, P2 == U ? 1 : 0);
				addCode(TemplateType::THighOrder, code, fv);

				code = fe->genCodePPF(HighOrder::SEG_IP1N1, label, P1 == N1 ? 1 : 0);
				addCode(TemplateType::THighOrder, code, fv);

				code = fe->genCodePPF(HighOrder::SEG_IUN2, label, U == N2 ? 1 : 0);
				addCode(TemplateType::THighOrder, code, fv);

			}
		}
	}
*/
}

vector<HeadIndex> DependencyPipe::findConjArg(DependencyInstance* s, HeadIndex& arg) {
	// 0: head; 1:left; 2:right
	HeadIndex head(-1, 0);
	HeadIndex left(-1, 0);
	HeadIndex right(-1, 0);
	SegElement& ele = s->getElement(arg);

	if (options->lang == PossibleLang::Chinese) {
		// head left arg right
		//   0   4    4    1
		right = ele.dep;
		if (arg < right) {
			head = s->getElement(right).dep;
			left = findLeftNearestChild(s->getElement(right).child, arg);
		}
		else {
			right.setIndex(-1, 0);
		}
	} else {
		ThrowException("undefined cc type");
	}

	vector<HeadIndex> ret;
	if (head.hWord != -1 && left.hWord != -1 && right.hWord != -1) {
		ret.resize(3);
		ret[0] = head;
		ret[1] = left;
		ret[2] = right;
	}
	return ret;
}

void DependencyPipe::createHighOrderFeatureVector(DependencyInstance* inst, FeatureVector* fv) {

	for (int i = 0; i < inst->numWord; ++i) {
		SegInstance& segInst = inst->word[i].getCurrSeg();
		for (int j = 0; j < segInst.size(); ++j) {
			SegElement& ele = segInst.element[j];
			SegElement* headEle = ele.dep.hWord >= 0 ? &inst->getElement(ele.dep) : NULL;
			HeadIndex m(i, j);

/*
			int m1 = inst->wordToSeg(m);
			int h1 = inst->wordToSeg(ele.dep);

			int small = m1 < h1 ? m1 : h1;
			int large = m1 < h1 ? h1 : m1;
			bool isProj = true;
			for (int m2 = small + 1; m2 < large; ++m2) {
				int h2 = inst->wordToSeg(inst->getElement(inst->segToWord(m2)).dep);
				if (h2 < small || h2 > large) {
					isProj = false;
					break;
				}
			}

			if (!isProj) {
				int MC = ele.getCurrPos();
				int HC = inst->getElement(ele.dep).getCurrPos();
				//long code = fe->genCodePF(HighOrder::NP, 1);
				//addCode(TemplateType::THighOrder, code, fv);

				//code = fe->genCodePF(HighOrder::NP_MC, MC);
				//addCode(TemplateType::THighOrder, code, fv);

				//code = fe->genCodePF(HighOrder::NP_HC, HC);
				//addCode(TemplateType::THighOrder, code, fv);

				long code = fe->genCodePPF(HighOrder::NP_HC_MC, HC, MC);
				addCode(TemplateType::THighOrder, code, fv);
			}
*/

			if (ele.getCurrSpecialPos() == SpecialPos::C) {
				// POS tag of CC
				HeadIndex m(i, j);
				vector<HeadIndex> arg = findConjArg(inst, m);
				if (arg.size() == 3) {
					int HP = inst->getElement(arg[0]).getCurrPos();
					int CP = ele.getCurrPos();
					int LP = inst->getElement(arg[1]).getCurrPos();
					int RP = inst->getElement(arg[2]).getCurrPos();
					long code = fe->genCodePPPF(HighOrder::CC_CP_LP_RP, CP, LP, RP);
					addCode(TemplateType::THighOrder, code, fv);

					code = fe->genCodePPPF(HighOrder::CC_CP_HC_AC, CP, HP, LP);
					addCode(TemplateType::THighOrder, code, fv);

					code = fe->genCodePPPF(HighOrder::CC_CP_HC_AC, CP, HP, RP);
					addCode(TemplateType::THighOrder, code, fv);
				}
			}
			else if (ele.getCurrSpecialPos() == SpecialPos::P) {
				int numChild = 0;
				for (int i = ele.child.size() - 1; i >= 0; --i) {
					SegElement& c = inst->getElement(ele.child[i]);
					if (c.getCurrSpecialPos() != SpecialPos::PNX) {
						int HC = headEle->getCurrPos();
						int MC = c.getCurrPos();
						long code = fe->genCodePPF(HighOrder::PP_HC_MC, HC, MC);
						addCode(TemplateType::THighOrder, code, fv);
						numChild++;
						//break;
					}
				}
				long code = fe->genCodePF(HighOrder::PP_HC_ML, numChild == 1 ? 1 : 0);
				addCode(TemplateType::THighOrder, code, fv);
			}

			// POS & child num
			long code = fe->genCodePPF(HighOrder::CN_HP_NUM, ele.getCurrPos(), min(5, (int)ele.child.size()));
			addCode(TemplateType::THighOrder, code, fv);

			// GPSib
			vector<HeadIndex>& child = ele.child;
			int aid = findRightNearestChildID(child, m);

			HeadIndex prev = m;
			SegElement* prevEle = &inst->getElement(prev);
			SegElement* nextEle = NULL;
			for (unsigned int j = aid; j < child.size(); ++j) {
				HeadIndex curr = child[j];
				SegElement* currEle = nextEle ? nextEle : &inst->getElement(curr);
				if (ele.dep.hWord >= 0) {
					createGPSibFeatureVector(inst, headEle, &ele, prevEle, currEle, fv);
				}

				if (j < child.size() - 1) {
					HeadIndex next = child[j + 1];
					nextEle = &inst->getElement(next);
					createTriSibFeatureVector(inst, &ele, prevEle, currEle, nextEle, fv);
				}

				prev = curr;
				prevEle = currEle;
			}

			// left children
			prev = m;
			prevEle = &inst->getElement(prev);
			nextEle = NULL;
			for (int j = aid - 1; j >= 0; --j) {
				HeadIndex curr = child[j];
				SegElement* currEle = nextEle ? nextEle : &inst->getElement(curr);
				if (ele.dep.hWord >= 0) {
					createGPSibFeatureVector(inst, headEle, &ele, prevEle, currEle, fv);
				}

				if (j > 0) {
					HeadIndex next = child[j - 1];
					nextEle = &inst->getElement(next);
					createTriSibFeatureVector(inst, &ele, prevEle, currEle, nextEle, fv);
				}

				prev = curr;
				prevEle = currEle;
			}
		}
	}
}

void DependencyPipe::createPartialHighOrderFeatureVector(DependencyInstance* inst, HeadIndex& x, bool bigram, FeatureVector* fv) {
	int xid = inst->wordToSeg(x);
	int yid = xid + (bigram ? 1 : 0);
	int hid = 0;
	for (int i = 0; i < inst->numWord; ++i) {
		SegInstance& segInst = inst->word[i].getCurrSeg();
		for (int j = 0; j < segInst.size(); ++j) {
			SegElement& ele = segInst.element[j];
			SegElement* headEle = ele.dep.hWord >= 0 ? &inst->getElement(ele.dep) : NULL;
			HeadIndex m(i, j);
/*
			// non-proj
			int m1 = inst->wordToSeg(m);
			int h1 = inst->wordToSeg(ele.dep);

			int small = m1 < h1 ? m1 : h1;
			int large = m1 < h1 ? h1 : m1;
			bool isProj = true;
			for (int m2 = small + 1; m2 < large; ++m2) {
				int h2 = inst->wordToSeg(inst->getElement(inst->segToWord(m2)).dep);
				if (h2 < small || h2 > large) {
					isProj = false;
					break;
				}
			}

			if (!isProj) {
				int MC = ele.getCurrPos();
				int HC = inst->getElement(ele.dep).getCurrPos();
				//long code = fe->genCodePF(HighOrder::NP, 1);
				//addCode(TemplateType::THighOrder, code, fv);

				//code = fe->genCodePF(HighOrder::NP_MC, MC);
				//addCode(TemplateType::THighOrder, code, fv);

				//code = fe->genCodePF(HighOrder::NP_HC, HC);
				//addCode(TemplateType::THighOrder, code, fv);

				long code = fe->genCodePPF(HighOrder::NP_HC_MC, HC, MC);
				addCode(TemplateType::THighOrder, code, fv);
			}
*/

			if (ele.getCurrSpecialPos() == SpecialPos::C) {
				// POS tag of CC
				HeadIndex m(i, j);
				vector<HeadIndex> arg = findConjArg(inst, m);
				if (arg.size() == 3) {
					int HP = inst->getElement(arg[0]).getCurrPos();
					int CP = ele.getCurrPos();
					int LP = inst->getElement(arg[1]).getCurrPos();
					int RP = inst->getElement(arg[2]).getCurrPos();
					long code = fe->genCodePPPF(HighOrder::CC_CP_LP_RP, CP, LP, RP);
					addCode(TemplateType::THighOrder, code, fv);

					code = fe->genCodePPPF(HighOrder::CC_CP_HC_AC, CP, HP, LP);
					addCode(TemplateType::THighOrder, code, fv);

					code = fe->genCodePPPF(HighOrder::CC_CP_HC_AC, CP, HP, RP);
					addCode(TemplateType::THighOrder, code, fv);
				}
			}
			else if (ele.getCurrSpecialPos() == SpecialPos::P) {
				int numChild = 0;
				for (int i = ele.child.size() - 1; i >= 0; --i) {
					SegElement& c = inst->getElement(ele.child[i]);
					if (c.getCurrSpecialPos() != SpecialPos::PNX) {
						int HC = headEle->getCurrPos();
						int MC = c.getCurrPos();
						long code = fe->genCodePPF(HighOrder::PP_HC_MC, HC, MC);
						addCode(TemplateType::THighOrder, code, fv);
						numChild++;
						//break;
					}
				}
				long code = fe->genCodePF(HighOrder::PP_HC_ML, numChild == 1 ? 1 : 0);
				addCode(TemplateType::THighOrder, code, fv);
			}

			// POS & child num
			long code = fe->genCodePPF(HighOrder::CN_HP_NUM, ele.getCurrPos(), min(5, (int)ele.child.size()));
			addCode(TemplateType::THighOrder, code, fv);

			// GPSib
			vector<HeadIndex>& child = ele.child;
			int aid = findRightNearestChildID(child, m);

			HeadIndex prev = m;
			SegElement* prevEle = &inst->getElement(prev);
			int previd = inst->wordToSeg(prev);
			SegElement* nextEle = NULL;
			int nextid = -1;
			for (unsigned int j = aid; j < child.size(); ++j) {
				HeadIndex curr = child[j];
				SegElement* currEle = nextEle ? nextEle : &inst->getElement(curr);
				int currid = nextEle ? nextid : inst->wordToSeg(curr);
				if (ele.dep.hWord >= 0) {
					if ((yid >= previd && xid <= currid)
							|| yid == hid || xid == hid) {
						createGPSibFeatureVector(inst, headEle, &ele, prevEle, currEle, fv);
					}
				}

				if (j < child.size() - 1) {
					HeadIndex next = child[j + 1];
					nextEle = &inst->getElement(next);
					nextid = inst->wordToSeg(next);
					if (yid >= previd && xid <= nextid) {
						createTriSibFeatureVector(inst, &ele, prevEle, currEle, nextEle, fv);
					}
				}

				prev = curr;
				prevEle = currEle;
				previd = currid;
			}

			// left children
			prev = m;
			prevEle = &inst->getElement(prev);
			previd = inst->wordToSeg(prev);
			nextEle = NULL;
			nextid = -1;
			for (int j = aid - 1; j >= 0; --j) {
				HeadIndex curr = child[j];
				SegElement* currEle = nextEle ? nextEle : &inst->getElement(curr);
				int currid = nextEle ? nextid : inst->wordToSeg(curr);
				if (ele.dep.hWord >= 0) {
					if ((yid >= currid && xid <= previd)
							|| yid == hid || xid == hid) {
						createGPSibFeatureVector(inst, headEle, &ele, prevEle, currEle, fv);
					}
				}

				if (j > 0) {
					HeadIndex next = child[j - 1];
					nextEle = &inst->getElement(next);
					nextid = inst->wordToSeg(next);
					if (yid >= nextid && xid <= previd) {
						createTriSibFeatureVector(inst, &ele, prevEle, currEle, nextEle, fv);
					}
				}

				prev = curr;
				prevEle = currEle;
				previd = currid;
			}

			hid++;
		}
	}
}

void DependencyPipe::createPartialPosHighOrderFeatureVector(DependencyInstance* inst, HeadIndex& x, FeatureVector* fv) {
	int hid = 0;
	for (int i = 0; i < inst->numWord; ++i) {
		SegInstance& segInst = inst->word[i].getCurrSeg();
		for (int j = 0; j < segInst.size(); ++j) {
			SegElement& ele = segInst.element[j];
			SegElement* headEle = ele.dep.hWord >= 0 ? &inst->getElement(ele.dep) : NULL;
			HeadIndex m(i, j);
/*
			// non-proj
			int m1 = inst->wordToSeg(m);
			int h1 = inst->wordToSeg(ele.dep);

			int small = m1 < h1 ? m1 : h1;
			int large = m1 < h1 ? h1 : m1;
			bool isProj = true;
			for (int m2 = small + 1; m2 < large; ++m2) {
				int h2 = inst->wordToSeg(inst->getElement(inst->segToWord(m2)).dep);
				if (h2 < small || h2 > large) {
					isProj = false;
					break;
				}
			}

			if (!isProj) {
				int MC = ele.getCurrPos();
				int HC = inst->getElement(ele.dep).getCurrPos();
				//long code = fe->genCodePF(HighOrder::NP, 1);
				//addCode(TemplateType::THighOrder, code, fv);

				//code = fe->genCodePF(HighOrder::NP_MC, MC);
				//addCode(TemplateType::THighOrder, code, fv);

				//code = fe->genCodePF(HighOrder::NP_HC, HC);
				//addCode(TemplateType::THighOrder, code, fv);

				long code = fe->genCodePPF(HighOrder::NP_HC_MC, HC, MC);
				addCode(TemplateType::THighOrder, code, fv);
			}
*/

			if (ele.getCurrSpecialPos() == SpecialPos::C) {
				// POS tag of CC
				HeadIndex m(i, j);
				vector<HeadIndex> arg = findConjArg(inst, m);
				if (arg.size() == 3) {
					if (x == arg[0] || x == arg[1] || x == arg[2] || x == m) {
						int HP = inst->getElement(arg[0]).getCurrPos();
						int CP = ele.getCurrPos();
						int LP = inst->getElement(arg[1]).getCurrPos();
						int RP = inst->getElement(arg[2]).getCurrPos();
						long code = fe->genCodePPPF(HighOrder::CC_CP_LP_RP, CP, LP, RP);
						addCode(TemplateType::THighOrder, code, fv);

						code = fe->genCodePPPF(HighOrder::CC_CP_HC_AC, CP, HP, LP);
						addCode(TemplateType::THighOrder, code, fv);

						code = fe->genCodePPPF(HighOrder::CC_CP_HC_AC, CP, HP, RP);
						addCode(TemplateType::THighOrder, code, fv);
					}
				}
			}
			else if (ele.getCurrSpecialPos() == SpecialPos::P) {
				int numChild = 0;
				for (int i = ele.child.size() - 1; i >= 0; --i) {
					SegElement& c = inst->getElement(ele.child[i]);
					if (c.getCurrSpecialPos() != SpecialPos::PNX) {
						int HC = headEle->getCurrPos();
						int MC = c.getCurrPos();
						long code = fe->genCodePPF(HighOrder::PP_HC_MC, HC, MC);
						addCode(TemplateType::THighOrder, code, fv);
						numChild++;
						//break;
					}
				}
				long code = fe->genCodePF(HighOrder::PP_HC_ML, numChild == 1 ? 1 : 0);
				addCode(TemplateType::THighOrder, code, fv);
			}

			// POS & child num
			if (x == m) {
				long code = fe->genCodePPF(HighOrder::CN_HP_NUM, ele.getCurrPos(), min(5, (int)ele.child.size()));
				addCode(TemplateType::THighOrder, code, fv);
			}

			// GPSib
			vector<HeadIndex>& child = ele.child;
			int aid = findRightNearestChildID(child, m);

			HeadIndex prev = m;
			SegElement* prevEle = &inst->getElement(prev);
			SegElement* nextEle = NULL;
			for (unsigned int j = aid; j < child.size(); ++j) {
				HeadIndex curr = child[j];
				SegElement* currEle = nextEle ? nextEle : &inst->getElement(curr);
				if (ele.dep.hWord >= 0) {
					if (x == m || x == ele.dep || x == prev || x == curr) {
						createGPSibFeatureVector(inst, headEle, &ele, prevEle, currEle, fv);
					}
				}

				if (j < child.size() - 1) {
					HeadIndex next = child[j + 1];
					nextEle = &inst->getElement(next);
					if (x == m || x == next || x == prev || x == curr) {
						createTriSibFeatureVector(inst, &ele, prevEle, currEle, nextEle, fv);
					}
				}

				prev = curr;
				prevEle = currEle;
			}

			// left children
			prev = m;
			prevEle = &inst->getElement(prev);
			nextEle = NULL;
			for (int j = aid - 1; j >= 0; --j) {
				HeadIndex curr = child[j];
				SegElement* currEle = nextEle ? nextEle : &inst->getElement(curr);
				if (ele.dep.hWord >= 0) {
					if (x == m || x == ele.dep || x == prev || x == curr) {
						createGPSibFeatureVector(inst, headEle, &ele, prevEle, currEle, fv);
					}
				}

				if (j > 0) {
					HeadIndex next = child[j - 1];
					nextEle = &inst->getElement(next);
					if (x == m || x == next || x == prev || x == curr) {
						createTriSibFeatureVector(inst, &ele, prevEle, currEle, nextEle, fv);
					}
				}

				prev = curr;
				prevEle = currEle;
			}

			hid++;
		}
	}
}

void DependencyPipe::addCode(int type, long code, double val, FeatureVector* fv) {
	int feat = dataAlphabet->lookupIndex(type, code, true);
	if (feat > 0)
		fv->add(feat, val);
}

void DependencyPipe::addCode(int type, long code, FeatureVector* fv) {
	int feat = dataAlphabet->lookupIndex(type, code, true);
	if (feat > 0)
		fv->addBinary(feat);
}

} /* namespace segparser */
