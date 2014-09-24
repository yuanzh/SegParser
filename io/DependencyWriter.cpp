/*
 * DependencyWriter.cpp
 *
 *  Created on: Apr 16, 2014
 *      Author: yuanz
 */

#include "DependencyWriter.h"

namespace segparser {

DependencyWriter::DependencyWriter(Options* options) : options(options) {
}

DependencyWriter::DependencyWriter(Options* options, string file) : options(options) {
	startWriting(file);
}

DependencyWriter::~DependencyWriter() {
}

void DependencyWriter::startWriting(string file) {
	fout.open(file.c_str());
}

void DependencyWriter::close() {
	if (fout.is_open())
		fout.close();
}

void DependencyWriter::writeInstance(DependencyInstance* inst) {
	for (int i = 1; i < inst->numWord; ++i) {
		WordInstance& word = inst->word[i];
		SegInstance& segInst = word.getCurrSeg();

		for (int j = 0; j < segInst.size(); ++j) {
			fout << i << "/" << j << "\t" << segInst.element[j].form << "\t" << segInst.element[j].form << "\t";
			string pos = segInst.element[j].candPos[segInst.element[j].currPosCandID];
			fout << pos << "\t" << pos << "\t_\t";
			fout << segInst.element[j].dep << "\t" << word.currSegCandID << "\t" << segInst.element[j].currPosCandID << "\t_\n";
		}
	}
	fout << endl;
	fout.flush();
}

} /* namespace segparser */
