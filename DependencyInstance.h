/*
 * DependencyInstance.h
 *
 *  Created on: Mar 21, 2014
 *      Author: yuanz
 */

#ifndef DEPENDENCYINSTANCE_H_
#define DEPENDENCYINSTANCE_H_

#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>
#include <unordered_set>
#include "util/FeatureVector.h"

namespace segparser {

class DependencyPipe;
class Options;

using namespace std;

class HeadIndex {
public:
	int hWord;
	int hSeg;

	HeadIndex(int _hWord, int _hSeg) : hWord(_hWord), hSeg(_hSeg) { }

	HeadIndex() : hWord(-1), hSeg(0) { }

	void setIndex(int _hWord, int _hSeg) {
		hWord = _hWord; hSeg = _hSeg;
	}

	friend bool operator < (HeadIndex &id1, HeadIndex &id2) {
		return id1.hWord < id2.hWord || (id1.hWord == id2.hWord && id1.hSeg < id2.hSeg);
	}

	friend bool operator != (HeadIndex &id1, HeadIndex &id2) {
		return id1.hWord != id2.hWord || id1.hSeg != id2.hSeg;
	}

	friend bool operator == (HeadIndex &id1, HeadIndex &id2) {
		return id1.hWord == id2.hWord && id1.hSeg == id2.hSeg;
	}

	friend ostream& operator << (ostream& os, const HeadIndex& id) {
		os << id.hWord << "/" << id.hSeg;
		return os;
	}
};

class SegElement {
public:
	string form;
	int formid;
	string lemma;
	int lemmaid;

	HeadIndex dep;
	int labid;

	vector<HeadIndex> child;

	int currPosCandID;			// id of the pos in candidate list
	vector<string> candPos;
	vector<int> candPosid;
	vector<int> candDetPosid;
	vector<int> candSpecialPos;
	vector<double> candProb;

	// for chinese character
	int st;
	int en;

	SegElement() : form(""), formid(-1), lemma(""), lemmaid(-1), labid(-1), currPosCandID(-1), st(-1), en(-1) {}

	int candPosNum() {
		return candPos.size();
	}

	bool isOptPos() {
		return currPosCandID == 0;
	}

	int getCurrPos() {
		return candPosid[currPosCandID];
	}

	int getCurrDetPos() {
		return candDetPosid[currPosCandID];
	}

	int getCurrSpecialPos() {
		return candSpecialPos[currPosCandID];
	}

	friend ostream& operator << (ostream& os, const SegElement& ele) {
		int i = ele.currPosCandID;
		os << ele.form << "_" << ele.formid << " " << ele.dep << " " <<
				ele.candPos[i] << "_" << ele.candPosid[i] << "_" << ele.candProb[i] << endl;
		return os;
	}
};

class SegInstance {
public:
	vector<SegElement> element;
	string segStr;
	double prob;

	// morphology features
	int AlIndex;
	int morphIndex;
	vector<string> morph;		//per/gen/num
	vector<int> morphid;

	SegInstance() : segStr(""), prob(0.0), AlIndex(-1), morphIndex(-1) {}

	int size() {
		return element.size();
	}

	friend ostream& operator << (ostream& os, const SegInstance& inst) {
		os << "seg str: " << inst.segStr << "_" << inst.prob << endl;
		for (unsigned int i = 0; i < inst.element.size(); ++i)
			os << "element: " << i << " " << inst.element[i] << endl;
		return os;
	}
};

class WordInstance {
public:
	// form/pos/dep/lab are retrieved from the candidate and id
	// the following are just for temporarily record gold info
	vector<string> goldForm;
	vector<string> goldLemma;
	vector<string> goldPos;

	int goldAlIndex;
	int goldMorphIndex;
	vector<string> goldMorph;

	vector<HeadIndex> goldDep;
	vector<string> goldLab;

	string wordStr;
	int wordid;

	int currSegCandID;		// id of the seg in candidate list
	vector<SegInstance> candSeg;

	int optPosCount;		// number of segs with optimal pos

	vector< vector<int> > inMap;		// [a->b id][size of b], for each element of b, need a map to decide the head and POS
	vector< vector<int> > outMap;		// [a->b id][size of a], for each child of a, need a map to decide its new parent

	WordInstance() {
		wordStr = "";
		wordid = -1;
		goldAlIndex = -1;
		goldMorphIndex = -1;
	}

	SegInstance& getCurrSeg() {
		return candSeg[currSegCandID];
	}

	bool isOptSeg() {
		return currSegCandID == 0;
	}

	void setOptPosCount() {
		optPosCount = 0;
		SegInstance& segInst = getCurrSeg();
		for (int j = 0; j < segInst.size(); ++j)
			optPosCount += (segInst.element[j].currPosCandID == 0);
	}
};

class DependencyInstance {
public:
	DependencyInstance();
	virtual ~DependencyInstance();

	vector<WordInstance> word;
	int numWord;			// number of words

	vector<int> characterid;

	int optSegCount;		// number of words with optimal seg

	FeatureVector fv;		// feature vector of the current tree

	// word index and seg index conversion
	void constructConversionList();
	void setOptSegPosCount();
	int getNumSeg();
	int wordToSeg(HeadIndex& id);
	int wordToSeg(int hid, int segid);
	HeadIndex segToWord(int id);

	void setInstIds(DependencyPipe* pipe, Options* options);
	string normalize(string s);

	int segDist(HeadIndex& head, HeadIndex& mod);
	SegElement& getElement(int hw, int hs);
	SegElement& getElement(HeadIndex id);

	void buildChild();
	void updateChildList(HeadIndex& newH, HeadIndex& oldH, HeadIndex& arg);

	void output();
private:
	vector<int> numSeg;		// total number of segs before this word, appending the total number in the end
							// size = number of words
	vector<int> seg2Word;	// word index for the segment, size = number of segs

	bool isPunc(string& w);
	bool isCoord(int lang, string& w);

	int computeOverlap(SegElement& e1, SegElement& e2);
	vector<int> buildInMap(WordInstance& w, int a, int b);
	vector<int> buildOutMap(WordInstance& w, int a, int b);
};

typedef boost::shared_ptr<DependencyInstance> inst_ptr;

class VariableInfo {
public:
	vector<int> segID;
	vector<int> posID;
	vector<HeadIndex> dep;

	VariableInfo() {

	}

	VariableInfo(DependencyInstance* inst) {
		copyInfoFromInst(inst);
	}

	void copyInfoFromInst(DependencyInstance* inst) {
		if ((int)segID.size() != inst->numWord) {
			segID.resize(inst->numWord);
		}

		int numSeg = inst->getNumSeg();
		if ((int)posID.size() != numSeg) {
			posID.resize(numSeg);
			dep.resize(numSeg);
		}

		int p = 0;
		for (int i = 0; i < inst->numWord; ++i) {
			segID[i] = inst->word[i].currSegCandID;
			SegInstance& segInst = inst->word[i].getCurrSeg();

			for (int j = 0; j < segInst.size(); ++j) {
				posID[p] = segInst.element[j].currPosCandID;
				dep[p] = segInst.element[j].dep;
				p++;
			}
		}
		assert(p == inst->getNumSeg());
	}

	void loadInfoToInst(DependencyInstance* inst) {
		assert((int)segID.size() == inst->numWord);

		int p = 0;
		for (int i = 0; i < inst->numWord; ++i) {
			inst->word[i].currSegCandID = segID[i];
			SegInstance& segInst = inst->word[i].getCurrSeg();

			for (int j = 0; j < segInst.size(); ++j) {
				segInst.element[j].currPosCandID = posID[p];
				segInst.element[j].dep = dep[p];
				p++;
			}
		}

		assert(p == (int)posID.size());
	}

	bool isChanged(DependencyInstance* inst) {
		assert((int)segID.size() == inst->numWord);

		int p = 0;
		for (int i = 0; i < inst->numWord; ++i) {
			if (inst->word[i].currSegCandID != segID[i])
				return true;
			SegInstance& segInst = inst->word[i].getCurrSeg();

			for (int j = 0; j < segInst.size(); ++j) {
				if (segInst.element[j].currPosCandID != posID[p])
					return true;
				if (segInst.element[j].dep != dep[p])
					return true;
				p++;
			}
		}

		assert(p == (int)posID.size());
		return false;
	}
};

} /* namespace segparser */

#include "DependencyPipe.h"

#endif /* DEPENDENCYINSTANCE_H_ */
