/*
 * Logarithm.cpp
 *
 *  Created on: Apr 15, 2014
 *      Author: yuanz
 */


#include "Logarithm.h"
#include <math.h>

double logsumexp(double num1, double num2) {
	double max_exp = 0.0;
	double sum = 0.0;
	if (num2 > num1) {
		max_exp = num2;
		sum = 1.0 + exp(num1 - max_exp);
	}
	else {
		max_exp = num1;
		sum = 1.0 + exp(num2 - max_exp);
	}
	double ret = log(sum) + max_exp;
	return ret;
}



