/*
 * Timer.h
 *
 *  Created on: Jan 21, 2014
 *      Author: yuanz
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <sys/time.h>
#include <iostream>

//#define CLOCKS_PER_SEC  1000000l

class Timer {
public:

	Timer() {
		gettimeofday(&begin, NULL);
	}

	double stop() {
		timeval end;
		gettimeofday(&end, NULL);
		double diffms = (((end.tv_sec - begin.tv_sec) * 1000000) + (end.tv_usec - begin.tv_usec))/1000;
	    return diffms;
	}
private:
	timeval begin;
};


#endif /* TIMER_H_ */
