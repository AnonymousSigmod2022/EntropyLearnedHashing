#pragma once

#include <cstdint>
#include <cmath>      

#include <cassert>
#include<cstring>

#if !defined(forceinline)
#define forceinline inline __attribute__((always_inline))
#endif

constexpr std::uint_fast8_t mask3{ 0b0000'0111 }; // mask to get remainder mod 8 of hash (bit location within byte)

class selfBloomUtils {
public:
	static size_t generateNumBits(double bitsPerElement, size_t numElementsExpected); 
};

/*
 * This class supports Bloom Filters up to 512 MB in size. Any larger requires a different implementation with more hashing. 
 */ 
template<class ValueType, class Hash = std::hash<ValueType>> 
class selfBloom {
public:
	double calculateFPR(size_t size_in_bytes, size_t numElementsExpected);
	explicit selfBloom(size_t sizeInBits, size_t numElementsExpected, const Hash& hashfn = Hash());
	selfBloom(const selfBloom<ValueType, Hash> &otherBloom);
	selfBloom(const selfBloom<ValueType, Hash> &&otherBloom);
	~selfBloom();	
	selfBloom<ValueType, Hash>& operator=(const selfBloom<ValueType, Hash>& otherBloom);
	selfBloom<ValueType, Hash>& operator=(const selfBloom<ValueType, Hash>&& otherBloom);
	bool testValue(const ValueType& x) const;
	void addValue(const ValueType& x);
	size_t sizeInBits;

private:
	uint8_t* bytes;
	size_t numElementsExpected;
	double fprAtExpectedElements;
	unsigned int numHashes;
	void setBit(const size_t val);
	bool testBit(const size_t val) const;
	const Hash hash;
};

// uses approximation that FPR is e
template<class ValueType, class Hash>
double selfBloom<ValueType, Hash>::calculateFPR(size_t size_in_bytes, size_t numElementsExpected) {
	double ln2 = log(2);
	double bitsPerElement = ((double) size_in_bytes * 8) / numElementsExpected;
	return pow(0.5, bitsPerElement * ln2);
}

/// Constructors, Destructors, Copy Assigment, Move Assignment

template<class ValueType, class Hash>
selfBloom<ValueType, Hash>::selfBloom(size_t sizeInBits, size_t numElementsExpected, const Hash& hashfn) : hash(hashfn) {
	int addByte = 0;
	if (sizeInBits % 8 != 0) {
		addByte = 1;
	}
	size_t sizeInBytes = (sizeInBits / 8) + addByte;
	//std::cout << "size in bytes: " << sizeInBytes << std::endl;
	this->bytes = new uint8_t[sizeInBytes];
	memset(this->bytes, 0, sizeInBytes);
	this->numElementsExpected = numElementsExpected;
	this->sizeInBits = 8 * sizeInBytes;
	this->fprAtExpectedElements = calculateFPR(sizeInBytes, numElementsExpected);
	numHashes = std::max(1.0, log(2) * ((double)sizeInBytes * 8) / numElementsExpected);
	//std::cout << "num hashes BF" << this->numHashes << std::endl;
	//std::cout << "expected FPR: " << fprAtExpectedElements << std::endl;
	assert(numHashes >= 1);
	//std::cout << "num hashes is: " << this->numHashes << std::endl;
}

template<class ValueType, class Hash>
selfBloom<ValueType, Hash>::selfBloom(const selfBloom<ValueType, Hash> &otherBloom) {
	this->sizeInBits = otherBloom.sizeInBits;
	this->bytes = new uint8_t[sizeInBits / 8];
	this->numElementsExpected = otherBloom.numElementsExpected;
	std::memcpy(this->bytes, otherBloom.bytes, sizeInBits/8);
	this->fprAtExpectedElements = otherBloom.fprAtExpectedElements;
	this->numHashes = otherBloom.numHashes;
	this->hash = otherBloom.hash;
}

template<class ValueType, class Hash>
selfBloom<ValueType, Hash>::selfBloom(const selfBloom<ValueType, Hash> &&otherBloom) {
	this->sizeInBits = otherBloom.sizeInBits;
	this->bytes = otherBloom->bytes;
	this->numElementsExpected = otherBloom.numElementsExpected;
	this->fprAtExpectedElements = otherBloom.fprAtExpectedElements;
	this->numHashes = otherBloom.numHashes;
	this->hash = otherBloom.hash;
	otherBloom->bytes = nullptr;
}

template<class ValueType, class Hash>
selfBloom<ValueType, Hash>& selfBloom<ValueType, Hash>::operator=(const selfBloom<ValueType, Hash>& otherBloom) { 
	if (this == &otherBloom) {
		return *this;
	}
	uint8_t* new_bytes = new uint8_t[otherBloom->sizeInBits / 8];
	std::memcpy(new_bytes, otherBloom->bytes, otherBloom->sizeInBits/8);
	if (this->bytes != NULL) {
		delete []this->bytes;
	}
	this->bytes = new_bytes;
	this->sizeInBits = otherBloom->sizeInBits;
	this->numElementsExpected = otherBloom->numElementsExpected;
	this->fprAtExpectedElements = otherBloom->fprAtExpectedElements;
	this->numHashes = otherBloom->numHashes;
	this->hash = otherBloom->hash;
	return *this;
}

template<class ValueType, class Hash>
selfBloom<ValueType, Hash>& selfBloom<ValueType, Hash>::operator=(const selfBloom<ValueType, Hash>&& otherBloom) {
	if (this != &otherBloom) {
		if (this->bytes != NULL) {
			delete []this->bytes;
		}
		this->bytes = otherBloom->bytes;
		this->sizeInBits = otherBloom->sizeInBits;
		this->numElementsExpected = otherBloom->numElementsExpected;
		this->fprAtExpectedElements = otherBloom->fprAtExpectedElements;
		this->numHashes = otherBloom->numHashes;
		this->hash = otherBloom->hash;
		otherBloom->bytes=nullptr;
		return *this;
	}	
}


template<class ValueType, class Hash>
selfBloom<ValueType, Hash>::~selfBloom() {
	delete []this->bytes;
}
size_t selfBloomUtils::generateNumBits(double bitsPerElement, size_t numElementsExpected) {
	size_t sizeInBits = (int)(numElementsExpected * bitsPerElement);
	return sizeInBits;
}

forceinline
uint32_t mult_reduce(const uint32_t hash, const uint32_t n) {
  // http://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
  	return (uint32_t)(((uint64_t)hash * n) >> 32);
}

template<class ValueType, class Hash>
forceinline
void selfBloom<ValueType, Hash>::setBit(const size_t position) {
	size_t byte = position >> 3;
	unsigned int mask = 1 << (position & mask3);
	bytes[byte] |= mask;
}

template<class ValueType, class Hash>
forceinline
bool selfBloom<ValueType, Hash>::testBit(const size_t position) const {
	size_t byte = position >> 3;
	unsigned int mask = 1 << (position & mask3);
	return (bytes[byte] & mask) != 0;
}

template<class ValueType, class Hash>
forceinline
void selfBloom<ValueType, Hash>::addValue(const ValueType& x) {
	// compute 2 hash values and use double hashing. 
	uint64_t hash_full = hash.operator()(x);
	uint32_t hash1 = (uint32_t)(hash_full);
	uint32_t hash2 = (uint32_t)(hash_full >> 32); 
	// do first hash function
	uint32_t position1 = mult_reduce(hash1, this->sizeInBits);
	setBit(position1);
	for (unsigned int i = 1; i < numHashes; i++) {
		uint32_t double_hash = hash1 + i * hash2;
		uint32_t position = mult_reduce(double_hash, this->sizeInBits);
		setBit(position);
	}
}

template<class ValueType, class Hash>
forceinline
bool selfBloom<ValueType, Hash>::testValue(const ValueType& x) const{
	// compute 2 hash values and use double hashing. 
	uint64_t hash_full = hash.operator()(x);
	uint32_t hash1 = (uint32_t)(hash_full);
	uint32_t hash2 = (uint32_t)(hash_full >> 32);
	// do first hash function
	uint32_t position1 = mult_reduce(hash1, this->sizeInBits);
	bool returnVal = testBit(position1);
	unsigned int i = 1;
	while (returnVal == true && i < numHashes) {
		uint32_t double_hash = hash1 + i * hash2;
		uint32_t position = mult_reduce(double_hash, this->sizeInBits);
		returnVal = testBit(position);
		i += 1;
	}
	return returnVal;
}

