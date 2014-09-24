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
		//begin = clock();
		gettimeofday(&begin, NULL);
	}

	double stop() {
	    //clock_t end = clock();
		timeval end;
		gettimeofday(&end, NULL);
	    //double diffticks = end - begin;
	    //double diffms = (diffticks*1000) / CLOCKS_PER_SEC;
		double diffms = (((end.tv_sec - begin.tv_sec) * 1000000) + (end.tv_usec - begin.tv_usec))/1000;
	    return diffms;
	}
private:
	timeval begin;
};


#endif /* TIMER_H_ */
