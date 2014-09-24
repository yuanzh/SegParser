/*
 * DependencyWriter.h
 *
 *  Created on: Apr 16, 2014
 *      Author: yuanz
 */

#ifndef DEPENDENCYWRITER_H_
#define DEPENDENCYWRITER_H_

#include <fstream>
#include "../DependencyInstance.h"
#include "../Options.h"

namespace segparser {

class DependencyWriter {
public:
	DependencyWriter(Options* options);
	DependencyWriter(Options* options, string file);
	virtual ~DependencyWriter();

	void startWriting(string file);
	void close();
	void writeInstance(DependencyInstance* inst);

private:
	ofstream fout;
	Options* options;

};

} /* namespace segparser */
#endif /* DEPENDENCYWRITER_H_ */
