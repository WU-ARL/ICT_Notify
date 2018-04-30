// This file is based on 2014 Gavin Andresen implementation of
// Invertibe Bloom Fiter (IBF)
//
// see https://github.com/gavinandresen/IBFT_Cplusplus for more details
#include <cassert>
#include <iostream>
#include <list>
#include <sstream>
#include <utility>
#include "ibft.hpp"
#include "murmurhash3.hpp"
#include "notificationData.hpp"

namespace notificationLib {

static const size_t N_HASH = 4;
static const size_t N_HASHCHECK = 11;

template<typename T>
std::vector<uint8_t> ToVec(T number)
{
  std::vector<uint8_t> v(sizeof(T));
  for (size_t i = 0; i < sizeof(T); i++) {
      v.at(i) = (number >> i*8) & 0xff;
  }
  return v;
}

bool IBFT::HashTableEntry::isPure() const
{
  if (count == 1 || count == -1) {
      uint32_t check = MurmurHash3(N_HASHCHECK, ToVec(keySum));
      return (keyCheck == check);
  }
  return false;
}

bool IBFT::HashTableEntry::empty() const
{
  return (count == 0 && keySum == 0 && keyCheck == 0);
}

void IBFT::HashTableEntry::addValue(const std::vector<uint8_t> v)
{
  if (v.empty()) {
      return;
  }
  if (valueSum.size() < v.size()) {
      valueSum.resize(v.size());
  }
  for (size_t i = 0; i < v.size(); i++) {
      valueSum[i] ^= v[i];
  }
}

IBFT::IBFT(size_t _expectedNumEntries, size_t _valueSize) :
    valueSize(_valueSize)
{
  // 1.5x expectedNumEntries gives very low probability of
  // decoding failure
  size_t nEntries = _expectedNumEntries + _expectedNumEntries/2;

  // ... make nEntries exactly divisible by N_HASH
  while (N_HASH * (nEntries/N_HASH) != nEntries) ++nEntries;
  m_hashTable.resize(nEntries);
}

IBFT::IBFT(const IBFT& other)
{
  valueSize = other.valueSize;
  m_hashTable = other.m_hashTable;
}

IBFT::IBFT(std::shared_ptr<ndn::Buffer> buf, size_t _expectedNumEntries, size_t _valueSize)
  : IBFT(_expectedNumEntries, _valueSize)
{
  Block bufferBlock = Block(buf);
  wireDecode(bufferBlock);
}

IBFT::~IBFT()
{
}

void IBFT::_insert(int plusOrMinus, uint64_t k, const std::vector<uint8_t> v)
{
  assert(v.size() == valueSize);

  std::vector<uint8_t> kvec = ToVec(k);

  size_t bucketsPerHash = m_hashTable.size()/N_HASH;
  for (size_t i = 0; i < N_HASH; i++) {
    size_t startEntry = i*bucketsPerHash;

    uint32_t h = MurmurHash3(i, kvec);
    IBFT::HashTableEntry& entry = m_hashTable.at(startEntry + (h%bucketsPerHash));
    entry.count += plusOrMinus;
    entry.keySum ^= k;
    entry.keyCheck ^= MurmurHash3(N_HASHCHECK, kvec);
    if (entry.empty()) {
      entry.valueSum.clear();
    }
    else {
      entry.addValue(v);
    }
  }
  \
}

void IBFT::insert(uint64_t k, const std::vector<uint8_t> v)
{
  _insert(1, k, v);
}

void IBFT::erase(uint64_t k, const std::vector<uint8_t> v)
{
  _insert(-1, k, v);
}

bool IBFT::get(uint64_t k, std::vector<uint8_t>& result) const
{
  result.clear();

  std::vector<uint8_t> kvec = ToVec(k);

  size_t bucketsPerHash = m_hashTable.size()/N_HASH;
  for (size_t i = 0; i < N_HASH; i++) {
    size_t startEntry = i*bucketsPerHash;

    uint32_t h = MurmurHash3(i, kvec);
    const IBFT::HashTableEntry& entry = m_hashTable.at(startEntry + (h%bucketsPerHash));

    if (entry.empty()) {
      // Definitely not in table. Leave
      // result empty, return true.
      return true;
    }
    else if (entry.isPure()) {
      if (entry.keySum == k) {
        // Found!
        result.assign(entry.valueSum.begin(), entry.valueSum.end());
        return true;
      }
      else {
        // Definitely not in table.
        return true;
      }
    }
  }

  // Don't know if k is in table or not; "peel" the IBFT to try to find
  // it:
  IBFT peeled = *this;
  size_t nErased = 0;
  for (size_t i = 0; i < peeled.m_hashTable.size(); i++) {
    IBFT::HashTableEntry& entry = peeled.m_hashTable.at(i);
    if (entry.isPure()) {
      if (entry.keySum == k) {
        // Found!
        result.assign(entry.valueSum.begin(), entry.valueSum.end());
        return true;
      }
      ++nErased;
      peeled._insert(-entry.count, entry.keySum, entry.valueSum);
    }
  }
  if (nErased > 0) {
      // Recurse with smaller IBFT
      return peeled.get(k, result);
  }
  return false;
}

bool IBFT::listEntries(std::set<std::pair<uint64_t,std::vector<uint8_t> > >& positive,
                       std::set<std::pair<uint64_t,std::vector<uint8_t> > >& negative) const
{
  IBFT peeled = *this;

  size_t nErased = 0;
  do {
    nErased = 0;
    for (size_t i = 0; i < peeled.m_hashTable.size(); i++) {
      IBFT::HashTableEntry& entry = peeled.m_hashTable.at(i);
      if (entry.isPure()) {
        if (entry.count == 1) {
          positive.insert(std::make_pair(entry.keySum, entry.valueSum));
        }
        else {
          negative.insert(std::make_pair(entry.keySum, entry.valueSum));
        }
        peeled._insert(-entry.count, entry.keySum, entry.valueSum);
        ++nErased;
      }
    }
  } while (nErased > 0);

  // If any buckets for one of the hash functions is not empty,
  // then we didn't peel them all:
  for (size_t i = 0; i < peeled.m_hashTable.size()/N_HASH; i++) {
    if (peeled.m_hashTable.at(i).empty() != true) return false;
  }
  return true;
}

IBFT IBFT::operator-(const IBFT& other) const
{
  // IBFT's must be same params/size:
  assert(valueSize == other.valueSize);
  assert(m_hashTable.size() == other.m_hashTable.size());

  IBFT result(*this);
  for (size_t i = 0; i < m_hashTable.size(); i++) {
    IBFT::HashTableEntry& e1 = result.m_hashTable.at(i);
    const IBFT::HashTableEntry& e2 = other.m_hashTable.at(i);
    e1.count -= e2.count;
    e1.keySum ^= e2.keySum;
    e1.keyCheck ^= e2.keyCheck;
    if (e1.empty()) {
      e1.valueSum.clear();
    }
    else {
      e1.addValue(e2.valueSum);
    }
  }
  return result;
}

// For debugging during development:
std::string IBFT::DumpTable() const
{
  std::ostringstream result;
  result << "valueSize = " << valueSize << " \n";
  result << "capacity = " << m_hashTable.capacity() << " \n";
  result << "table size = " << m_hashTable.size() << " \n";

  result << "count keySum keyCheckMatch sizeofEntry \n";
  for (size_t i = 0; i < m_hashTable.size(); i++) {
    const IBFT::HashTableEntry& entry = m_hashTable.at(i);
    result << entry.count << " " << entry.keySum << " ";
    result << (MurmurHash3(N_HASHCHECK, ToVec(entry.keySum)) == entry.keyCheck ? "true" : "false");
    result << " " << sizeof(entry);
    result << "\n";
  }
  return result.str();
}

size_t IBFT::getIBFSize() const
{
  size_t totalSize = 0;
  for (size_t i = 0; i < m_hashTable.size(); i++)
  {
    totalSize += sizeof(m_hashTable.at(i));
  }
  return (totalSize);
  // alternative way to calculate the size
  // every entry (4+8+4+valueSize) * # of entries
  //return(m_hashTable.size() * (sizeof(int32_t)+sizeof(uint64_t)+sizeof(uint32_t)+valueSize));
  //return(m_hashTable.size() * sizeof(m_hashTable));

}

uint8_t* IBFT::getIBFBuffer() const
{
  //return(reinterpret_cast<uint8_t*>(m_hashTable.data()));
  return((uint8_t*)(m_hashTable.data()));
}

std::string IBFT::dumpItems() const
{
  std::ostringstream result;
  result << "items in IBF:\n";
  std::set<std::pair<uint64_t,std::vector<uint8_t> > > positive;
  std::set<std::pair<uint64_t,std::vector<uint8_t> > > negative;

  result << "can be resolved:" << listEntries(positive, negative) << "\n";
  std::set<std::pair<uint64_t,std::vector<uint8_t> > > :: iterator it; //iterator to manipulate set
  for (it = positive.begin(); it!=positive.end(); it++){
      std::pair<uint64_t,std::vector<uint8_t> > m = *it; // returns pair to m
      result << "key: " << m.first << "\n"; //showing the key
  }
  return result.str();
}

template<encoding::Tag T>
size_t
IBFT::wireEncode(EncodingImpl<T>& encoder) const
{
  size_t totalLength = 0;
  int i = 0;
  // go over m_hashTable
  for(auto const& itr: m_hashTable)
  {
    if(!itr.empty())
    {
      size_t entryLength = 0;
      entryLength += prependNonNegativeIntegerBlock(encoder, tlv::IBFEntryIndex, i);
      std::string countStr = std::to_string(itr.count);
      entryLength += prependStringBlock(encoder, tlv::IBFEntryCount, countStr);
      entryLength += prependNonNegativeIntegerBlock(encoder, tlv::IBFEntryKeySum, itr.keySum);
      entryLength += prependNonNegativeIntegerBlock(encoder, tlv::IBFEntryKeyCheck, itr.keyCheck);
      entryLength += encoder.prependByteArrayBlock(tlv::IBFEntryValueSum, reinterpret_cast<const uint8_t*>(itr.valueSum.data()), itr.valueSum.size());

      entryLength += encoder.prependVarNumber(entryLength);
      entryLength += encoder.prependVarNumber(tlv::IBFEntry);
      totalLength += entryLength;
    }
    ++i;
  }
  totalLength += encoder.prependVarNumber(totalLength);
  totalLength += encoder.prependVarNumber(tlv::IBFTable);
  return totalLength;
}

Block
IBFT::wireEncode() const
{
  Block block;

  EncodingEstimator estimator;
  size_t estimatedSize = wireEncode(estimator);

  EncodingBuffer buffer(estimatedSize);
  wireEncode(buffer);

  return buffer.block();
}

void
IBFT::wireDecode(const Block& wire)
{
  if (!wire.hasWire())
    std::cerr << "The supplied block does not contain wire format" << std::endl;

  if (wire.type() != tlv::IBFTable)
    std::cerr << "Unexpected TLV type when decoding reply: " +
      boost::lexical_cast<std::string>(wire.type()) <<std::endl;

  wire.parse();

  // for each entry
  for (Block::element_const_iterator it = wire.elements_begin();
       it != wire.elements_end(); it++)
  {
    if (it->type() == tlv::IBFEntry)
    {
      it->parse();

      Block::element_const_iterator entryIt = it->elements_begin();
      HashTableEntry entry;
      int index = 0;
      int verifyEntry = 0;

      while(entryIt != it->elements_end())
      {
        if (entryIt->type() == tlv::IBFEntryValueSum)
        {
          std::vector<uint8_t> v(entryIt->value(),
                                  entryIt->value()+ entryIt->value_size());
          entry.addValue(v);
          ++verifyEntry;
        }
        if (entryIt->type() == tlv::IBFEntryKeyCheck)
        {
          entry.keyCheck = readNonNegativeInteger(*entryIt);
          ++verifyEntry;
        }
        else if (entryIt->type() == tlv::IBFEntryKeySum)
        {
          entry.keySum = readNonNegativeInteger(*entryIt);
          ++verifyEntry;
        }
        else if (entryIt->type() == tlv::IBFEntryCount)
        {
          std::string tmp(readString(*entryIt));
          entry.count = std::atoi(tmp.c_str());
          ++verifyEntry;
        }
        else if (entryIt->type() == tlv::IBFEntryIndex)
        {
          index = readNonNegativeInteger(*entryIt);
          ++verifyEntry;
        }
        ++entryIt;
      }
      if(verifyEntry == 5)
      {
        m_hashTable.at(index) = entry;
      }
      else
        std::cerr << "Missing TLVs in hash entry" <<std::endl;
    }
    //std::cout << "Table after decoder" << DumpTable()<< std::endl;
  }
}
#if 0
IBFT::IBFT(const char* buffer, size_t bufferSize, size_t _valueSize)
  : valueSize(_valueSize)
{
  std::string fromBuf(buffer,bufferSize);
  std::cout << " fromBuf" << fromBuf << std::endl;
  std::cout << " bufferSize" << bufferSize << std::endl;
  unsigned int numOfEntries = bufferSize / sizeof(HashTableEntry);
  //m_hashTable.resize(numOfEntries);
  const HashTableEntry* entryPtr = reinterpret_cast<const HashTableEntry*>(buffer);
  //m_hashTable.insert(m_hashTable.begin(), entryPtr, entryPtr + numOfEntries);
  for(int i = 0; i < numOfEntries; ++i)
  {
       std::cout << i << std::endl;
       m_hashTable.push_back(*(entryPtr+i));
  }
  // std::cout << " numOfEntries" << numOfEntries << std::endl;
  // std::vector<HashTableEntry> tbl(entryPtr, entryPtr + numOfEntries );
  // std::cout << " init tbl" << numOfEntries << std::endl;
  // m_hashTable = tbl;
  // std::cout << " init m_hashTable" << numOfEntries << std::endl;

}
IBFT::IBFT(const std::string& strIBF, size_t _valueSize)
  //: IBFT(_expectedNumEntries, _valueSize)
  : valueSize(_valueSize)
{
  unsigned int numOfEntries = strIBF.size() / sizeof(HashTableEntry);
  std::cout << " numOfEntries" << numOfEntries << std::endl;
  const HashTableEntry* entryPtr = reinterpret_cast<const HashTableEntry*>(strIBF.c_str());
  std::cout << " entryPtr" << entryPtr<< std::endl;

  m_hashTable.insert(m_hashTable.begin(), entryPtr, entryPtr + numOfEntries);

  // for(int i = 0; i < numOfEntries; ++i)
  // {
  //      std::cout << i << std::endl;
  //      m_hashTable.push_back(*(entryPtr+i));
  // }
std::cout << " IBF end" << std::endl;
  // every entry (4+8+4+valueSize) * # of entries
  // size_t entrySize = sizeof(HashTableEntry::count)
  //                    + sizeof(HashTableEntry::keySum)
  //                    + sizeof(HashTableEntry::keyCheck)
  //                    + valueSize;
  //
  // int numOfEntries = strIBF.size()/entrySize;
  // const uint8_t* actual = reinterpret_cast<const uint8_t*>(strIBF.c_str());

  // m_hashTable.resize(m_hashTable.size() + numOfEntries);
  //std::memcpy(m_hashTable.data(),actual, strIBF.size());


  //m_hashTable.insert(m_hashTable.end(), &strIBF[0], strIBF.size());
//#if 0

  //std::cout << "numOfEntries: " << numOfEntries << std::endl;
  //uint8_t* destPos = reinterpret_cast<uint8_t*>(m_hashTable.data());
  #if 0
  size_t dstPos = 0;
  size_t srcPos = 0;
  size_t bytesCount = 0;
  int entry = 0;
  while(srcPos != strIBF.size())
  //for (int i = 0; i < numOfEntries; ++i )
  {
    int32_t count = 0;
    uint64_t keySum = 0;
    uint32_t keyCheck = 0;
    std::vector<uint8_t> valueSum;

    std::memcpy(reinterpret_cast<uint8_t*>(&count),actual+srcPos, sizeof(count));
    srcPos += sizeof(count);

    if(count == 0)
    {
      // skip to the next entry
      srcPos += sizeof(keySum) + sizeof(keyCheck) + 8;
    }
    else
    {
      std::memcpy(reinterpret_cast<uint8_t*>(&keySum),actual+srcPos, sizeof(keySum));
      srcPos += sizeof(keySum);
      std::memcpy(reinterpret_cast<uint8_t*>(&keyCheck),actual+srcPos, sizeof(keyCheck));
      srcPos += sizeof(keyCheck) + 8;
    }
    std::cout << "count = " << count << ", keySum = " << keySum
              << ", keyCheck = " << keyCheck << std::endl;
    //std::cout << "srcPos = " << srcPos << std::endl;
#if 0
    // std::memcpy(destPos,actual + (i*entrySize), entrySize-valueSize);
    // IBFT::HashTableEntry& e = m_hashTable.at(i);

    //std::vector<uint8_t> v(*(actual + (i*entrySize) + entrySize-valueSize), valueSize);
    //e.addValue(v);

    // destPos += entrySize;
    //std::memcpy(m_hashTable.data(),actual, strIBF.size());

    //size_t pos, bytesCount = 0;
    //IBFT::HashTableEntry& e = m_hashTable.at(i);

    // copy count
    //std::cout << "start pos: " << pos << std::endl;
    bytesCount = sizeof(HashTableEntry::count);
    int32_t count;
    std::memcpy(reinterpret_cast<uint8_t*>(&count),actual+srcPos, bytesCount);
  //  std::memcpy(m_hashTable.data()+dstPos,actual+srcPos, bytesCount);
    std::cout << "count = " << count << std::endl;
    if(count == 0)
    {
      // skip to the next entry
    }
    // copy count
    // std::copy(actual + pos,
    //           actual + pos + bytesCount,
    //           reinterpret_cast<uint8_t*>((&e.count)));
    // std::memcpy(&(e.count),
    //             actual+pos,
    //             bytesCount);
    //std::cout << "copied e.count: " << e.count << std::endl;

    // copy keySum
    srcPos += bytesCount;
    dstPos += bytesCount;
    bytesCount = sizeof(HashTableEntry::keySum);
    uint64_t keySum;
    std::memcpy(reinterpret_cast<uint8_t*>(&keySum),actual+srcPos, bytesCount);
    std::cout << "keySum = " << keySum << std::endl;
    //std::memcpy(m_hashTable.data()+dstPos,actual+srcPos, bytesCount);
    // std::copy(actual + pos,
    //           actual + pos + bytesCount,
    //           reinterpret_cast<uint8_t*>(&e.keySum));
// std::cout << "copied e.keySum: " << e.keySum << std::endl;
    // std::memcpy(reinterpret_cast<uint8_t*>(&(e.keySum)),
    //             actual+pos,
    //             bytesCount);


    // copy keyCheck
    srcPos += bytesCount;
    dstPos += bytesCount;
    bytesCount = sizeof(HashTableEntry::keyCheck);
    std::memcpy(m_hashTable.data()+dstPos,actual+srcPos, bytesCount);
//     std::copy(actual + pos,
//               actual + pos + bytesCount,
//               reinterpret_cast<uint8_t*>(&e.keyCheck));
// std::cout << "copied e.keyCheck: " << e.keyCheck << std::endl;
    // std::memcpy(&(e.keyCheck),
    //             actual+pos,
    //             bytesCount);

    srcPos += bytesCount;
    dstPos += bytesCount;
    // check if valueSum was resized Before
    if(count != 0)
    {
        // there is a value: need to copy valueSum

        // set dstPos for next itr
        srcPos += valueSize*2;
        dstPos += valueSize*2;

    }


    // pos += sizeof(HashTableEntry::keyCheck);
    // copy valueSum
    pos += bytesCount;
    std::vector<uint8_t> v(*(actual + pos), valueSize);
    // std::vector<uint8_t> v(*(actual+(i*entrySize + pos)), valueSize);

    //std::cout << "end: copied = " << pos + valueSize << std::endl;
    e.addValue(v);
    #endif
  }
  #endif
}
#endif
} // end namespace notificationLib
