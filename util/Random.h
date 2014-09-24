/*
 * Random.h
 *
 *  Created on: Jan 21, 2014
 *      Author: yuanz
 */

#ifndef RANDOM_H_
#define RANDOM_H_

#include <time.h>
#include <random>

class Random {
public:
	Random() {
		eng.seed(time(NULL));
	}

	Random(int seed) {
		eng.seed(seed);
	}

	void setSeed(int seed) {
		eng.seed(seed);
	}

	int nextInt(int n) {
		std::uniform_int_distribution<int> dist(0, n - 1);
		return dist(eng);
	}

	double nextDouble() {
		std::uniform_real_distribution<double> dist(0, 1);
		return dist(eng);
	}

private:
	std::default_random_engine eng;
};

#endif /* RANDOM_H_ */
