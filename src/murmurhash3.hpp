/* -*- Mode:C++; c-file-style:"bsd"; indent-tabs-mode:nil; -*- */
/**
 * Copyright 2020 Washington University in St. Louis
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. 
 */

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
