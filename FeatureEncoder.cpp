/*
 * FeatureEncoder.cpp
 *
 *  Created on: Mar 29, 2014
 *      Author: yuanz
 */

#include "FeatureEncoder.h"
#include <iostream>
#include <stdlib.h>
#include <iomanip>
#include <bitset>

namespace segparser {

FeatureEncoder::FeatureEncoder() {
	largeOff = 17;
	midOff = 9;
	flagOff = 4;
	tempOff = 7;
}

FeatureEncoder::~FeatureEncoder() {
}

/*********************************
 * code generator
 * generally flag will be added lately, because code without flag is also needed
 */

int FeatureEncoder::getBits(uint64_t x) {
	uint64_t y = 1;
    int i = 0;
    while (y < x) {
            y = y << 1;
            ++i;
    }
    return i;
}

uint64_t FeatureEncoder::genCodePF(uint64_t temp, uint64_t p1) {
//	uint64_t ret = p1;
//	ret = ret << flagOff;
//	ret = ret << tempOff;
//	ret = ret | temp;
//	return ret;
	return ((p1 << flagOff) << tempOff) | temp;
}

uint64_t FeatureEncoder::genCodePPF(uint64_t temp, uint64_t p1, uint64_t p2) {
//	uint64_t ret = p1;
//	ret = ret << midOff;
//	ret = ret | p2;
//	ret = ret << flagOff;
//	ret = ret << tempOff;
//	ret = ret | temp;
//	return ret;
	return ((((p1 << midOff) | p2) << flagOff) << tempOff) | temp;
}

uint64_t FeatureEncoder::genCodePPPF(uint64_t temp, uint64_t p1, uint64_t p2, uint64_t p3) {
//	uint64_t ret = p1;
//	ret = ret << midOff;
//	ret = ret | p2;
//	ret = ret << midOff;
//	ret = ret | p3;
//	ret = ret << flagOff;
//	ret = ret << tempOff;
//	ret = ret | temp;
//	return ret;
	return ((((((p1 << midOff) | p2) << midOff) | p3) << flagOff) << tempOff) | temp;
}

uint64_t FeatureEncoder::genCodePPPPF(uint64_t temp, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4) {
//	uint64_t ret = p1;
//	ret = ret << midOff;
//	ret = ret | p2;
//	ret = ret << midOff;
//	ret = ret | p3;
//	ret = ret << midOff;
//	ret = ret | p4;
//	ret = ret << flagOff;
//	ret = ret << tempOff;
//	ret = ret | temp;
//	return ret;
	return ((((((((p1 << midOff) | p2) << midOff) | p3) << midOff) | p4) << flagOff) << tempOff) | temp;
}

uint64_t FeatureEncoder::genCodePPPPPF(uint64_t temp, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5) {
//	uint64_t ret = p1;
//	ret = ret << midOff;
//	ret = ret | p2;
//	ret = ret << midOff;
//	ret = ret | p3;
//	ret = ret << midOff;
//	ret = ret | p4;
//	ret = ret << midOff;
//	ret = ret | p5;
//	ret = ret << flagOff;
//	ret = ret << tempOff;
//	ret = ret | temp;
//	return ret;
	return ((((((((((p1 << midOff) | p2) << midOff) | p3) << midOff) | p4) << midOff) | p5) << flagOff) << tempOff) | temp;
}

uint64_t FeatureEncoder::genCodeWF(uint64_t temp, uint64_t w1) {
//	uint64_t ret = w1;
//	ret = ret << flagOff;
//	ret = ret << tempOff;
//	ret = ret | temp;
//	return ret;
	return ((w1 << flagOff) << tempOff) | temp;
}

uint64_t FeatureEncoder::genCodePWF(uint64_t temp, uint64_t p1, uint64_t w1) {
//	uint64_t ret = w1;
//	ret = ret << midOff;
//	ret = ret | p1;
//	ret = ret << flagOff;
//	ret = ret << tempOff;
//	ret = ret | temp;
//	return ret;
	return ((((w1 << midOff) | p1) << flagOff) << tempOff) | temp;
}

uint64_t FeatureEncoder::genCodeWWF(uint64_t temp, uint64_t w1, uint64_t w2) {
//	uint64_t ret = w1;
//	ret = ret << largeOff;
//	ret = ret | w2;
//	ret = ret << flagOff;
//	ret = ret << tempOff;
//	ret = ret | temp;
//	return ret;
	return ((((w1 << largeOff) | w2) << flagOff) << tempOff) | temp;
}

uint64_t FeatureEncoder::genCodeWWW(uint64_t temp, uint64_t w1, uint64_t w2, uint64_t w3) {
//	uint64_t ret = w1;
//	ret = ret << largeOff;
//	ret = ret | w2;
//	ret = ret << largeOff;
//	ret = ret | w3;
//	ret = ret << tempOff;
//	ret = ret | temp;
//	return ret;
	return (((((w1 << largeOff) | w2) << largeOff) | w3) << tempOff) | temp;
}

uint64_t FeatureEncoder::genCodePPWF(uint64_t temp, uint64_t p1, uint64_t p2, uint64_t w1) {
//	uint64_t ret = w1;
//	ret = ret << midOff;
//	ret = ret | p1;
//	ret = ret << midOff;
//	ret = ret | p2;
//	ret = ret << flagOff;
//	ret = ret << tempOff;
//	ret = ret | temp;
//	return ret;
	return ((((((w1 << midOff) | p1) << midOff) | p2) << flagOff) << tempOff) | temp;
}

uint64_t FeatureEncoder::genCodePPPWF(uint64_t temp, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t w1) {
//	uint64_t ret = w1;
//	ret = ret << midOff;
//	ret = ret | p1;
//	ret = ret << midOff;
//	ret = ret | p2;
//	ret = ret << midOff;
//	ret = ret | p3;
//	ret = ret << flagOff;
//	ret = ret << tempOff;
//	ret = ret | temp;
//	return ret;
	return ((((((((w1 << midOff) | p1) << midOff) | p2) << midOff) | p3) << flagOff) << tempOff) | temp;
}

uint64_t FeatureEncoder::genCodePWWF(uint64_t temp, uint64_t p1, uint64_t w1, uint64_t w2) {
//	uint64_t ret = w1;
//	ret = ret << largeOff;
//	ret = ret | w2;
//	ret = ret << midOff;
//	ret = ret | p1;
//	ret = ret << flagOff;
//	ret = ret << tempOff;
//	ret = ret | temp;
//	return ret;
	return ((((((w1 << largeOff) | w2) << midOff) | p1) << flagOff) << tempOff) | temp;
}

uint64_t FeatureEncoder::genCodePPWWF(uint64_t temp, uint64_t p1, uint64_t p2, uint64_t w1, uint64_t w2) {
//	uint64_t ret = w1;
//	ret = ret << largeOff;
//	ret = ret | w2;
//	ret = ret << midOff;
//	ret = ret | p1;
//	ret = ret << midOff;
//	ret = ret | p2;
//	ret = ret << flagOff;
//	ret = ret << tempOff;
//	ret = ret | temp;
//	return ret;
	return ((((((((w1 << largeOff) | w2) << midOff) | p1) << midOff) | p2) << flagOff) << tempOff) | temp;
}

uint64_t FeatureEncoder::genCodeIIVF(uint64_t temp, uint64_t i1, uint64_t i2, uint64_t v1) {
//	uint64_t ret = i1;
//	ret = ret << midOff;
//	ret = ret | i2;
//	ret = ret << midOff;
//	ret = ret | v1;
//	ret = ret << flagOff;
//	ret = ret << tempOff;
//	ret = ret | temp;
//	return ret;
	return ((((((i1 << midOff) | i2) << midOff) | v1) << flagOff) << tempOff) | temp;
}

uint64_t FeatureEncoder::genCodeIIVVF(uint64_t temp, uint64_t i1, uint64_t i2, uint64_t v1, uint64_t v2) {
//	uint64_t ret = i1;
//	ret = ret << midOff;
//	ret = ret | i2;
//	ret = ret << midOff;
//	ret = ret | v1;
//	ret = ret << midOff;
//	ret = ret | v2;
//	ret = ret << flagOff;
//	ret = ret << tempOff;
//	ret = ret | temp;
//	return ret;
	return ((((((((i1 << midOff) | i2) << midOff) | v1) << midOff) | v2) << flagOff) << tempOff) | temp;
}

uint64_t FeatureEncoder::genCodeIIVVPF(uint64_t temp, uint64_t i1, uint64_t i2, uint64_t v1, uint64_t v2, uint64_t p1) {
//	uint64_t ret = i1;
//	ret = ret << midOff;
//	ret = ret | i2;
//	ret = ret << midOff;
//	ret = ret | v1;
//	ret = ret << midOff;
//	ret = ret | v2;
//	ret = ret << midOff;
//	ret = ret | p1;
//	ret = ret << flagOff;
//	ret = ret << tempOff;
//	ret = ret | temp;
//	return ret;
	return ((((((((((i1 << midOff) | i2) << midOff) | v1) << midOff) | v2) << midOff) | p1) << flagOff) << tempOff) | temp;
}

uint64_t FeatureEncoder::genCodeIIVPF(uint64_t temp, uint64_t i1, uint64_t i2, uint64_t v1, uint64_t p1) {
//	uint64_t ret = i1;
//	ret = ret << midOff;
//	ret = ret | i2;
//	ret = ret << midOff;
//	ret = ret | v1;
//	ret = ret << midOff;
//	ret = ret | p1;
//	ret = ret << flagOff;
//	ret = ret << tempOff;
//	ret = ret | temp;
//	return ret;
	return ((((((((i1 << midOff) | i2) << midOff) | v1) << midOff) | p1) << flagOff) << tempOff) | temp;
}

uint64_t FeatureEncoder::genCodeIIVPPF(uint64_t temp, uint64_t i1, uint64_t i2, uint64_t v1, uint64_t p1, uint64_t p2) {
//	uint64_t ret = i1;
//	ret = ret << midOff;
//	ret = ret | i2;
//	ret = ret << midOff;
//	ret = ret | v1;
//	ret = ret << midOff;
//	ret = ret | p1;
//	ret = ret << midOff;
//	ret = ret | p2;
//	ret = ret << flagOff;
//	ret = ret << tempOff;
//	ret = ret | temp;
//	return ret;
	return ((((((((((i1 << midOff) | i2) << midOff) | v1) << midOff) | p1) << midOff) | p2) << flagOff) << tempOff) | temp;
}

uint64_t FeatureEncoder::genCodeIIVVPPF(uint64_t temp, uint64_t i1, uint64_t i2, uint64_t v1, uint64_t v2, uint64_t p1, uint64_t p2) {
//	uint64_t ret = i1;
//	ret = ret << midOff;
//	ret = ret | i2;
//	ret = ret << midOff;
//	ret = ret | v1;
//	ret = ret << midOff;
//	ret = ret | v2;
//	ret = ret << midOff;
//	ret = ret | p1;
//	ret = ret << midOff;
//	ret = ret | p2;
//	ret = ret << flagOff;
//	ret = ret << tempOff;
//	ret = ret | temp;
//	return ret;
	return ((((((((((((i1 << midOff) | i2) << midOff) | v1) << midOff) | v2)
			<< midOff) | p1) << midOff) | p2) << flagOff) << tempOff) | temp;
}

} /* namespace segparser */
