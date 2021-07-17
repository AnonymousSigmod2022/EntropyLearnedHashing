#pragma once

#include <cstdint>
#include <cmath> 
#include <cstdlib>
#include "self_block_bloom.hh"    

#if !defined(forceinline)
#define forceinline inline __attribute__((always_inline))
#endif

//constexpr std::uint_fast8_t mask3block{ 0b0000'0111 };

/*
 * This class supports Bloom Filters up to 2 TB in size. This should be way more than enough in practice. 
 */ 
template<class ValueType, class Hash = std::hash<ValueType>> 
class selfBlockBloom64 {
public:
	selfBlockBloom64(size_t sizeInBits, size_t numElementsExpected, const Hash& hashfn = Hash());
	selfBlockBloom64(const selfBlockBloom64<ValueType, Hash> &selfBlockBloom64);
	selfBlockBloom64(const selfBlockBloom64<ValueType, Hash> &&selfBlockBloom64);
	~selfBlockBloom64();
	selfBlockBloom64<ValueType, Hash>& operator=(const selfBlockBloom64<ValueType, Hash>& otherBloom);
	selfBlockBloom64<ValueType, Hash>& operator=(const selfBlockBloom64<ValueType, Hash>&& otherBloom);
	bool testValue(const ValueType& x) const;
	void addValue(const ValueType& x);
	double getFPRatMaxCapacity();
	size_t sizeInBits;

private:
	uint64_t* words;
	size_t numElementsExpected;
	double fprAtExpectedElements;
	unsigned int numHashes;
	unsigned int numBlocks;
	void setBits(const size_t block, const size_t positionInBlock);
	bool testBits(const size_t block, const size_t positionInBlock) const;
	const Hash hash;
};

/*double poisson_pmf(const double numHashes, const double lambda) {
    return exp(numHashes * log(lambda) - lgamma(numHashes + 1.0) - lambda);
}

double fpr_standard(const double numHashes, const double numElementsExpected, const double bitsPerElement) {
	double probabilityBitNotSet = pow(1 - (1.0 / (numElementsExpected * bitsPerElement)), numHashes * numElementsExpected);
	return pow(1 - probabilityBitNotSet, numHashes);
} */

double calculateFPRGivenK64(double bitsPerElement, unsigned int k) {
	double threshold = pow(10.0, -12);
	double fpr = 0.0;
	unsigned int i = 0;
	double meanPerBlock = 64.0 / bitsPerElement;
	while (true) {
		double fprStandard = fpr_standard(k, i, (64.0/i));
		double iPMF = poisson_pmf(i, meanPerBlock);
		fpr += fprStandard * iPMF;
		if (iPMF < threshold && i > 150) {
			break;
		}
		i += 1;
	}
	return fpr;
}

template<class ValueType, class Hash>
double selfBlockBloom64<ValueType, Hash>::getFPRatMaxCapacity() {
	return this->fprAtExpectedElements;
}

// Constructors and assignment operators
template<class ValueType, class Hash>
selfBlockBloom64<ValueType, Hash>::selfBlockBloom64(size_t sizeInBits, size_t numElementsExpected, const Hash& hashfn): hash(hashfn) {
	size_t sizeInBytes = (sizeInBits + (512 - (sizeInBits % 512))) / 8;
	this->words = (uint64_t*) aligned_alloc(64, sizeInBytes);
	if (this->words == nullptr) {
		std::cout << "selfBlockBloom error1 in aligned alloc. Error code: " << strerror(errno) << std::endl;
	}
	memset(this->words, 0, sizeInBytes);	
	this->numElementsExpected = numElementsExpected;
	this->sizeInBits = 8 * sizeInBytes;
	this->fprAtExpectedElements = 1.0;
	// 3 plus 24.0 bits -> around 3% fpr. 
	for (unsigned int numHashes = 1; numHashes < 3; numHashes++) {
		double tempFPR = calculateFPRGivenK64(this->sizeInBits / this->numElementsExpected, numHashes);
		if (tempFPR < this->fprAtExpectedElements) {
			this->fprAtExpectedElements = tempFPR;
			this->numHashes = numHashes;
		}
	}
	//std::cout << "expected FPR register (biased approximation): " << this->fprAtExpectedElements << std::endl;
	//std::cout << "num hashes RBBF" << this->numHashes << std::endl;
	assert(numHashes >= 1);
	// 64 bit blocks
	this->numBlocks = this->sizeInBits / 64;
	assert((this->sizeInBits / 64) == (sizeInBytes / 8));
}

template<class ValueType, class Hash>
selfBlockBloom64<ValueType, Hash>::selfBlockBloom64(const selfBlockBloom64<ValueType, Hash> &otherBloom) {
	this->sizeInBits = otherBloom.sizeInBits;
	this->words = (uint8_t*) aligned_alloc(64, otherBloom.sizeInBits/8);
	if (this->words == nullptr) {
		std::cout << "selfBlockBloom error in aligned alloc. Error code: " << strerror(errno);
	}
	this->numElementsExpected = otherBloom.numElementsExpected;
	std::memcpy(this->words, otherBloom.words, otherBloom.sizeInBits/8);
	this->fprAtExpectedElements = otherBloom.fprAtExpectedElements;
	this->numHashes = otherBloom.numHashes;
	this->numBlocks = otherBloom.numBlocks;
	this->blockUsages = new int[numBlocks];
	this->hash = otherBloom.hash;
}

template<class ValueType, class Hash>
selfBlockBloom64<ValueType, Hash>::selfBlockBloom64(const selfBlockBloom64<ValueType, Hash> &&otherBloom) {
	this->sizeInBits = otherBloom.sizeInBits;
	this->words = otherBloom->words;
	this->numElementsExpected = otherBloom.numElementsExpected;
	this->fprAtExpectedElements = otherBloom.fprAtExpectedElements;
	this->numHashes = otherBloom.numHashes;
	this->numBlocks = otherBloom.numBlocks;
	otherBloom->words = nullptr;
	this->hash = otherBloom.hash;
}

template<class ValueType, class Hash>
selfBlockBloom64<ValueType, Hash>& selfBlockBloom64<ValueType, Hash>::operator=(const selfBlockBloom64<ValueType, Hash>& otherBloom) { 
	if (this == &otherBloom) {
		return *this;
	}
	uint64_t* new_bytes = (uint64_t*) aligned_alloc(64, otherBloom->sizeInBits/8);
	std::memcpy(new_bytes, otherBloom->words, otherBloom->sizeInBits/8);
	if (this->words != NULL) {
		delete []this->words;
	}
	this->words = new_bytes;
	this->sizeInBits = otherBloom->sizeInBits;
	this->numElementsExpected = otherBloom->numElementsExpected;
	this->fprAtExpectedElements = otherBloom->fprAtExpectedElements;
	this->numHashes = otherBloom->numHashes;
	this->numBlocks = otherBloom->numBlocks;
	this->hash = otherBloom->hash;
	return *this;
}

template<class ValueType, class Hash>
selfBlockBloom64<ValueType, Hash>& selfBlockBloom64<ValueType, Hash>::operator=(const selfBlockBloom64<ValueType, Hash>&& otherBloom) {
	if (this != &otherBloom) {
		if (this->words != NULL) {
			delete []this->words;
		}
		this->words = otherBloom->words;
		this->sizeInBits = otherBloom->sizeInBits;
		this->numElementsExpected = otherBloom->numElementsExpected;
		this->fprAtExpectedElements = otherBloom->fprAtExpectedElements;
		this->numHashes = otherBloom->numHashes;
		this->numBlocks = otherBloom->numBlocks;
		this->hash = otherBloom->hash;
		otherBloom->words=nullptr;
		return *this;
	}	
}

template<class ValueType, class Hash>
selfBlockBloom64<ValueType, Hash>::~selfBlockBloom64() {
	delete []this->words;
}

/*forceinline
uint32_t mult_reduce_block(const uint32_t hash, const uint32_t n) {
  // http://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
  	return (uint32_t)(((uint64_t)hash * (uint64_t)n) >> 32);
} */

template<class ValueType, class Hash>
forceinline
void selfBlockBloom64<ValueType, Hash>::addValue(const ValueType& x) {
	// compute 2 hash values and use double hashing. 
	uint64_t hash_full = hash.operator()(x);
	uint32_t hash1 = (uint32_t)(hash_full);
	uint32_t hash2 = (uint32_t)(hash_full >> 32); 
	// do first hash function
	uint32_t wordNum = mult_reduce_block(hash1, this->numBlocks);
	uint64_t testWord = 0;
	for (unsigned int i = 1; i <= numHashes; i++) {
		uint32_t positionInBlock = hash2 & (63);
		//uint32_t double_hash = hash1 + i * hash2;
		// which position to set
		//uint32_t positionInBlock = double_hash & (63);
		testWord |= 1 << positionInBlock;
		hash2 = hash2 >> 6;
	}
	words[wordNum] |= testWord;
}

template<class ValueType, class Hash>
forceinline
bool selfBlockBloom64<ValueType, Hash>::testValue(const ValueType& x) const{
	// compute 2 hash values and use double hashing. 
	uint64_t hash_full = hash.operator()(x);
	uint32_t hash1 = (uint32_t)(hash_full);
	uint32_t hash2 = (uint32_t)(hash_full >> 32); 
	// do first hash function
	uint32_t wordNum = mult_reduce_block(hash1, this->numBlocks);
	uint64_t testWord = 0;
	for (unsigned int i = 1; i < numHashes; i++) {
		uint32_t positionInBlock = hash2 & (63);
		//uint32_t double_hash = hash1 + i * hash2;
		// which position to set
		//uint32_t positionInBlock = double_hash & (63);
		testWord |= 1 << positionInBlock;
		hash2 = hash2 >> 6;
	}
	uint32_t positionInBlock = hash2 & (63);
	testWord |= 1 << positionInBlock;
	return (testWord & words[wordNum]) == testWord;
}