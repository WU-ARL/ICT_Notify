// This file is based on 2014 Gavin Andresen implementation of
// Invertible Bloom Fiter (IBF)
//
// see https://github.com/gavinandresen/IBLT_Cplusplus for more details

#ifndef MURMURHASH3_H
#define MURMURHASH3_H

#include <inttypes.h>
#include <vector>

extern uint32_t MurmurHash3(uint32_t nHashSeed, const std::vector<unsigned char>& vDataToHash);

#endif /* MURMURHASH3_H */
