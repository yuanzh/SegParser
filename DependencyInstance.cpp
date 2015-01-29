/*
 * DependencyInstance.cpp
 *
 *  Created on: Mar 21, 2014
 *      Author: yuanz
 */

#include "DependencyInstance.h"
#include "util/Constant.h"
#include <boost/regex.hpp>
#include "util/StringUtils.h"
#include <float.h>
#include "util/Logarithm.h"

namespace segparser {

DependencyInstance::DependencyInstance() {
	numWord = -1;
}

DependencyInstance::~DependencyInstance() {
}

int DependencyInstance::getNumSeg() {
	return seg2Word.size();
}

void DependencyInstance::constructConversionList() {
	int size = 0;
	for (int i = 0; i < numWord; ++i)
		size += word[i].getCurrSeg().size();

	if ((int)numSeg.size() != numWord + 1)
		numSeg.resize(numWord + 1);
	if (getNumSeg() != size)
		seg2Word.resize(size);

	size = 0;
	for (int i = 0; i < numWord; ++i) {
		SegInstance& segInst = word[i].getCurrSeg();
		numSeg[i] = size;
		for (int j = 0; j < segInst.size(); ++j)
			seg2Word[size + j] = i;
		size += segInst.size();
	}
	numSeg[numWord] = size;
}

void DependencyInstance::setOptSegPosCount() {
	optSegCount = 0;
	for (int i = 0; i < numWord; ++i) {
		optSegCount += (word[i].currSegCandID == 0);
		word[i].setOptPosCount();
	}
}

int DependencyInstance::wordToSeg(HeadIndex& id) {
	if (id.hWord == -1)
		return -1;
	else {
		return numSeg[id.hWord] + id.hSeg;
	}
}

int DependencyInstance::wordToSeg(int hid, int segid) {
	if (hid == -1)
		return -1;
	else
		return numSeg[hid] + segid;
}

HeadIndex DependencyInstance::segToWord(int id) {
	if (id == -1)
		return HeadIndex(-1, 0);
	else {
		int word = seg2Word[id];
		return HeadIndex(word, id - numSeg[word]);
	}
}

bool DependencyInstance::isPunc(string& w) {
	return boost::regex_match(w, boost::regex("[-!\"#%&'()*,./:;?@\\[\\]_{}、]+"));
}

bool DependencyInstance::isCoord(int lang, string& w) {
	switch (lang) {
		case PossibleLang::Arabic:
			if (/*w.compare("w") == 0 || */w.compare(">w") == 0/* || w.compare(">n") == 0*/)
				return true;
			else
				return false;
			break;
		case PossibleLang::SPMRL:
			if (/*w.compare("w") == 0 || */w.compare(">w") == 0/* || w.compare(">n") == 0*/)
				return true;
			else
				return false;
			break;
		case PossibleLang::Chinese:
			//if (w.compare("和") == 0|| w.compare("或") == 0)
				return true;
			//else
			//	return false;
			break;
		default:
			return true;
			break;
	}
	return true;
}

string DependencyInstance::normalize(string s) {
	if (s.compare("-LRB-") == 0)
		s = "(";
	else if (s.compare("-RRB-") == 0)
		s = ")";
	if (boost::regex_match(s, boost::regex("(-*)([0-9]+|[0-9]+\\.[0-9]+|[0-9]+[0-9,]+)")))
		s = "<num>";
	return s;
}

int DependencyInstance::computeOverlap(SegElement& e1, SegElement& e2) {
	return max(0, min(e1.en, e2.en) - max(e1.st, e2.st));
}

vector<int> DependencyInstance::buildInMap(WordInstance& w, int a, int b) {
	SegInstance& sa = w.candSeg[a];
	SegInstance& sb = w.candSeg[b];

	vector<int> map;
	map.resize(sb.size());

	int pa = 0;
	for (int pb = 0; pb < sb.size(); ++pb) {
		while (pa < sa.size() && computeOverlap(sa.element[pa], sb.element[pb]) == 0) {
			pa++;
		}
		assert(pa != sa.size());
		map[pb] = pa;
	}
	//assert(pa == (int)sa.size() - 1);

	return map;
}

vector<int> DependencyInstance::buildOutMap(WordInstance& w, int a, int b) {
	SegInstance& sa = w.candSeg[a];
	SegInstance& sb = w.candSeg[b];

	vector<int> map;
	map.resize(sa.size());

	int pb = 0;
	for (int pa = 0; pa < sa.size(); ++pa) {
		while (pb < sb.size() && computeOverlap(sa.element[pa], sb.element[pb]) == 0) {
			pb++;
		}
		assert(pb != sb.size());
		map[pa] = pb;
	}
	//assert(pb == (int)sb.size() - 1);

	return map;
}

void DependencyInstance::setInstIds(DependencyPipe* pipe, Options* options) {
	Alphabet* typeAlphabet = pipe->typeAlphabet;
	Alphabet* posAlphabet = pipe->posAlphabet;
	Alphabet* lexAlphabet = pipe->lexAlphabet;
	unordered_set<string>& suffixList = pipe->suffixList;
	unordered_map<string, string>& coarseMap = pipe->coarseMap;

	if (options->lang == PossibleLang::Chinese) {
		int len = 0;
		for (int i = 1; i < numWord; ++i) {
			for (unsigned int j = 0; j < word[i].goldForm.size(); ++j) {
				len += ChineseStringLength(word[i].goldForm[j]);
			}
		}
		characterid.resize(len);
		len = 0;
		for (int i = 1; i < numWord; ++i) {
			for (unsigned int j = 0; j < word[i].goldForm.size(); ++j) {
				int length = ChineseStringLength(word[i].goldForm[j]);
				for (int k = 0; k < length; ++k) {
					characterid[len] = lexAlphabet->lookupIndex(GetChineseChar(word[i].goldForm[j], k));
					len++;
				}
			}
		}
		assert(len == (int)characterid.size());

		len = 0;
		for (int i = 1; i < numWord; ++i) {
			WordInstance& currWord = word[i];
			int off = 0;
			for (unsigned int j = 0; j < currWord.candSeg.size(); ++j) {
				SegInstance& seg = currWord.candSeg[j];
				off = 0;
				for (int k = 0; k < seg.size(); ++k) {
					SegElement& ele = seg.element[k];
					ele.st = len + off;
					ele.en = ele.st + ChineseStringLength(ele.form);
					off += ChineseStringLength(ele.form);

					for (int l = ele.st; l < ele.en; ++l) {
						assert(lexAlphabet->lookupIndex(GetChineseChar(ele.form, l - ele.st)) == characterid[l]);
					}
				}
			}
			len += off;

			// build in/out map
			/*
			if (currWord.candSeg.size() > 1) {
				int size = currWord.candSeg.size();
				currWord.inMap.resize(size * size);
				currWord.outMap.resize(size * size);
				for (int a = 0; a < size; ++a) {
					for (int b = 0; b < size; ++b) {
						currWord.inMap[a * size + b] = buildInMap(currWord, a, b);
						currWord.outMap[a * size + b] = buildOutMap(currWord, a, b);
					}
				}
			}
			*/
		}
		assert(len == (int)characterid.size());
	}

	for (int i = 0; i < numWord; ++i) {
		WordInstance& currWord = word[i];
		currWord.wordid = lexAlphabet->lookupIndex(normalize(currWord.wordStr));

		SegInstance& currSeg = currWord.getCurrSeg();
		assert(currSeg.size() == (int)currWord.goldDep.size()
				&& currSeg.size() == (int)currWord.goldLab.size());

		// label id
		for (int j = 0; j < currSeg.size(); ++j)
			currSeg.element[j].labid = typeAlphabet->lookupIndex(currWord.goldLab[j]);

		// lex
		for (unsigned int j = 0; j < currWord.candSeg.size(); ++j) {
			SegInstance& seg = currWord.candSeg[j];
			for (unsigned int k = 0; k < seg.morph.size(); ++k) {
				seg.morphid.push_back(posAlphabet->lookupIndex(seg.morph[k]));
			}

			for (int k = 0; k < seg.size(); ++k) {
				SegElement& ele = seg.element[k];
				ele.formid = lexAlphabet->lookupIndex(normalize(ele.form));
				ele.lemmaid = lexAlphabet->lookupIndex(normalize(ele.lemma));
				ele.labid = ConstLab::NOTYPE;

				// pos
				for (int l = 0; l < ele.candPosNum(); ++l) {
					ele.candPosid[l] = posAlphabet->lookupIndex(ele.candPos[l]);
					if (k == seg.AlIndex) {
						ele.candDetPosid[l] = posAlphabet->lookupIndex("dt+" + ele.candPos[l]);
					}
					else {
						ele.candDetPosid[l] = ele.candPosid[l];
					}

					// special pos
					if (coarseMap.find(ele.candPos[l]) != coarseMap.end()) {
						string cpos = coarseMap[ele.candPos[l]];
						if (cpos.compare("CONJ") == 0 && isCoord(options->lang, ele.form))
							ele.candSpecialPos[l] = SpecialPos::C;
						else if (cpos.compare("ADP") == 0)
							ele.candSpecialPos[l] = SpecialPos::P;
						else if (isPunc(ele.form) || (options->lang == PossibleLang::Chinese && ele.candPos[l] == "PU"))
							ele.candSpecialPos[l] = SpecialPos::PNX;
						else if (cpos.compare("VERB") == 0)
							ele.candSpecialPos[l] = SpecialPos::V;
						else
							ele.candSpecialPos[l] = SpecialPos::OTHER;
					}
					else {
						ele.candSpecialPos[l] = SpecialPos::OTHER;
					}
				}
			}

			// heuristic rule to decide in/out node, first non-Al node is in node, last non-suffix node is out node, dep is right branching
			if (options->heuristicDep) {
				seg.AlNode = -1;
				for (int k = 0; k < seg.size(); ++k)
					if (seg.element[k].form == "Al") {
						assert(seg.AlNode == -1);
						seg.AlNode = k;
					}
				if (seg.element[0].form == "Al") {
					if (seg.size() == 1)
						seg.inNode = 0;
					else
						seg.inNode = 1;
				} else {
					seg.inNode = 0;
				}
				seg.outNode = seg.size() - 1;
				for (; seg.outNode >= 0; seg.outNode--) {
					if (suffixList.find(seg.element[seg.outNode].form) == suffixList.end())
						break;
				}
				if (seg.outNode < 0)
					seg.outNode = 0;
			}
			else {
				seg.inNode = -1;
				seg.outNode = -1;
				seg.AlNode = -1;
			}
		}
	}
}

int DependencyInstance::segDist(HeadIndex& head, HeadIndex& mod) {
	HeadIndex small = head < mod ? head : mod;
	HeadIndex large = head < mod ? mod : head;

	int smallIndex = wordToSeg(small);
	int largeIndex = wordToSeg(large);

	return largeIndex - smallIndex;

	//int hid = wordToSeg(head);
	//int mid = wordToSeg(mod);
	//return hid - mid;
}

SegElement& DependencyInstance::getElement(int hw, int hs) {
	return word[hw].getCurrSeg().element[hs];
}

SegElement& DependencyInstance::getElement(HeadIndex id) {
	return word[id.hWord].getCurrSeg().element[id.hSeg];
}

void DependencyInstance::buildChild() {
	vector<int> count(getNumSeg(), 0);
	for (int i = 1; i < numWord; ++i) {
		SegInstance& segInst = word[i].getCurrSeg();
		for (int j = 0; j < segInst.size(); ++j) {
			int hSegid = wordToSeg(segInst.element[j].dep);
			count[hSegid]++;
		}
	}

	for (int i = 0; i < numWord; ++i) {
		SegInstance& segInst = word[i].getCurrSeg();
		for (int j = 0; j < segInst.size(); ++j) {
			int hSegid = wordToSeg(i, j);
			segInst.element[j].child.resize(count[hSegid]);
			count[hSegid] = 0;
		}
	}

	for (int i = 1; i < numWord; ++i) {
		SegInstance& segInst = word[i].getCurrSeg();
		for (int j = 0; j < segInst.size(); ++j) {
			HeadIndex& head = segInst.element[j].dep;
			int hSegid = wordToSeg(head);
			getElement(head).child[count[hSegid]].setIndex(i, j);
			count[hSegid]++;
		}
	}
}

void DependencyInstance::updateChildList(HeadIndex& newH, HeadIndex& oldH, HeadIndex& arg) {
	SegElement& argEle = word[arg.hWord].getCurrSeg().element[arg.hSeg];

	assert(newH == argEle.dep);

	// remove arg in oldH
	vector<HeadIndex>& oldChildList = word[oldH.hWord].getCurrSeg().element[oldH.hSeg].child;
	vector<HeadIndex> list(oldChildList.size() - 1);
	int p = 0;
	for (unsigned int i = 0; i < oldChildList.size(); ++i) {
		if (oldChildList[i] == arg)
			continue;
		list[p] = oldChildList[i];
		p++;
	}
	oldChildList = list;
	// add arg in newH
	vector<HeadIndex>& newChildList = word[newH.hWord].getCurrSeg().element[newH.hSeg].child;
	list.resize(newChildList.size() + 1);
	p = 0;
	bool add = false;
	for (unsigned int i = 0; i < newChildList.size(); ++i) {
		if (!add && arg < newChildList[i]) {
			list[p] = arg;
			add = true;
			p++;
		}
		list[p] = newChildList[i];
		p++;
	}
	if (!add) {
		list[p] = arg;
	}
	newChildList = list;
}

void DependencyInstance::output() {
	//for (unsigned int i = 0; i < characterid.size(); ++i) {
	//	cout << characterid[i] << " ";
	//}
	//cout << endl;
	cout << numWord << endl;
	for (int i = 1; i < numWord; ++i) {
		SegInstance& segInst = word[i].getCurrSeg();
		cout << segInst.AlIndex << " " << segInst.morphIndex << " morph: ";
		cout.flush();
		for (unsigned int j = 0; j < segInst.morph.size(); ++j) {
			cout << segInst.morph[j] << "_" << segInst.morphid[j] << "/";
		}
		vector<double> probList(word[i].candSeg.size());
		for (unsigned int j = 0; j < word[i].candSeg.size(); ++j)
			probList[j] = word[i].candSeg[j].prob;

		double sumScore = -DBL_MAX;
		for (unsigned int j = 0; j < probList.size(); ++j) {
			sumScore = logsumexp(sumScore, probList[j]);
		}

		for (unsigned int j = 0; j < probList.size(); ++j) {
			probList[j] = exp(probList[j] - sumScore);
		}
		for (unsigned int j = 0; j < word[i].candSeg.size(); ++j)
			cout << "\t" << probList[j] << "_" << word[i].candSeg[j].prob;
		cout << endl;
		for (int j = 0; j < segInst.size(); ++j) {
			SegElement& ele = segInst.element[j];
			cout << HeadIndex(i, j) << "\t" << ele.form << "_" << ele.formid;
			cout << "\t" << ele.lemma << "_" << ele.lemmaid;
			cout << "\t" << ele.candPos[ele.currPosCandID] << "_" << ele.getCurrPos();
			cout << "\t" << ele.dep << "\t" << word[i].currSegCandID << "\t" << ele.currPosCandID << "\t" << ele.st << "_" << ele.en << endl;
		}
	}
	cout << endl;
	int x;
	cin >> x;
}

} /* namespace segparser */
