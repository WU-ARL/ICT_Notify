// This file is based on 2014 Gavin Andresen implementation of
// Invertible Bloom Fiter (IBF)
//
// see https://github.com/gavinandresen/IBFT_Cplusplus for more details
#ifndef IBFT_H
#define IBFT_H

#include "common.hpp"
#include <inttypes.h>
#include <set>
#include <vector>
#include <ndn-cxx/encoding/tlv-nfd.hpp>

//
// Invertible Bloom Lookup Table implementation
// References:
//
// "What's the Difference? Efficient Set Reconciliation
// without Prior Context" by Eppstein, Goodrich, Uyeda and
// Varghese
//
// "Invertible Bloom Lookup Tables" by Goodrich and
// Mitzenmacher
//
namespace notificationLib
{

class IBFT
{
public:
    IBFT(size_t _expectedNumEntries, size_t _ValueSize);
    IBFT(const IBFT& other);
    // IBFT(const std::string& strIBF, size_t _valueSize);
    // IBFT(const char* buffer, size_t bufferSize, size_t _valueSize);
    IBFT(std::shared_ptr<ndn::Buffer>, size_t _expectedNumEntries, size_t _valueSize);
    virtual ~IBFT();

    void insert(uint64_t k, const std::vector<uint8_t> v);
    void erase(uint64_t k, const std::vector<uint8_t> v);

    // Returns true if a result is definitely found or not
    // found. If not found, result will be empty.
    // Returns false if overloaded and we don't know whether or
    // not k is in the table.
    bool get(uint64_t k, std::vector<uint8_t>& result) const;

    // Adds entries to the given sets:
    //  positive is all entries that were inserted
    //  negative is all entreis that were erased but never added (or
    //   if the IBFT = A-B, all entries in B that are not in A)
    // Returns true if all entries could be decoded, false otherwise.
    bool listEntries(std::set<std::pair<uint64_t,std::vector<uint8_t> > >& positive,
        std::set<std::pair<uint64_t,std::vector<uint8_t> > >& negative) const;

    // Subtract two IBFTs
    IBFT operator-(const IBFT& other) const;

    // For debugging:
    std::string DumpTable() const;

    // get HashTable data size
    size_t getIBFSize() const;

    uint8_t* getIBFBuffer() const;

    std::string dumpItems() const;

    // for encoding and decoding
    //template<bool T>
    template<encoding::Tag T> size_t
    wireEncode(EncodingImpl<T>& encoder) const;

    Block wireEncode() const;

    void
    wireDecode(const Block& wire);

private:
    void _insert(int plusOrMinus, uint64_t k, const std::vector<uint8_t> v);

    size_t valueSize;
    //size_t numOfStoredElements;
    class HashTableEntry
    {
    public:
        int32_t count;
        uint64_t keySum;
        uint32_t keyCheck;
        std::vector<uint8_t> valueSum;

        bool isPure() const;
        bool empty() const;
        void addValue(const std::vector<uint8_t> v);
    };
    std::vector<HashTableEntry> m_hashTable;
};
} // namespace NotificationLib
#endif /* IBFT_H */
