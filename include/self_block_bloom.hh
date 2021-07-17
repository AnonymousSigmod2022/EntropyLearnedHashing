#pragma once

#include <cstdint>
#include <cmath> 
#include <cstdlib>    

#if !defined(forceinline)
#define forceinline inline __attribute__((always_inline))
#endif

constexpr std::uint_fast8_t mask3block{ 0b0000'0111 };

/*
 * This class supports Bloom Filters up to 2 TB in size. This should be way more than enough in practice. 
 */ 
template<class ValueType, class Hash = std::hash<ValueType>> 
class selfBlockBloom512 {
public:
	selfBlockBloom512(size_t sizeInBits, size_t numElementsExpected, const Hash& hashfn = Hash());
	selfBlockBloom512(const selfBlockBloom512<ValueType, Hash> &selfBlockBloom512);
	selfBlockBloom512(const selfBlockBloom512<ValueType, Hash> &&selfBlockBloom512);
	~selfBlockBloom512();
	selfBlockBloom512<ValueType, Hash>& operator=(const selfBlockBloom512<ValueType, Hash>& otherBloom);
	selfBlockBloom512<ValueType, Hash>& operator=(const selfBlockBloom512<ValueType, Hash>&& otherBloom);
	bool testValue(const ValueType& x) const;
	void addValue(const ValueType& x);
	double getFPRatMaxCapacity();
	size_t sizeInBits;

private:
	uint8_t* bytes;
	size_t numElementsExpected;
	double fprAtExpectedElements;
	unsigned int numHashes;
	unsigned int numBlocks;
	void setBit(const size_t block, const size_t positionInBlock);
	bool testBit(const size_t block, const size_t positionInBlock) const;
	const Hash hash;
};

double poisson_pmf(const double numHashes, const double lambda) {
    return exp(numHashes * log(lambda) - lgamma(numHashes + 1.0) - lambda);
}

double fpr_standard(const double numHashes, const double numElementsExpected, const double bitsPerElement) {
	double probabilityBitNotSet = pow(1 - (1.0 / (numElementsExpected * bitsPerElement)), numHashes * numElementsExpected);
	return pow(1 - probabilityBitNotSet, numHashes);
}

double calculateFPRGivenK(double bitsPerElement, unsigned int k) {
	double threshold = pow(10.0, -12);
	double fpr = 0.0;
	unsigned int i = 0;
	double meanPerBlock = 512.0 / bitsPerElement;
	while (true) {
		double fprStandard = fpr_standard(k, i, (512.0/i));
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
double selfBlockBloom512<ValueType, Hash>::getFPRatMaxCapacity() {
	return this->fprAtExpectedElements;
}

// Constructors and assignment operators
template<class ValueType, class Hash>
selfBlockBloom512<ValueType, Hash>::selfBlockBloom512(size_t sizeInBits, size_t numElementsExpected, const Hash& hashfn): hash(hashfn) {
	size_t sizeInBytes = (sizeInBits + (512 - (sizeInBits % 512))) / 8;
	//std::cout << "size in bytes: " << sizeInBytes << std::endl;
	this->bytes = (uint8_t*) aligned_alloc(64, sizeInBytes);
	memset(this->bytes, 0, sizeInBytes);
	if (this->bytes == nullptr) {
		std::cout << "selfBlockBloom error1 in aligned alloc. Error code: " << strerror(errno);
	}
	this->numElementsExpected = numElementsExpected;
	this->sizeInBits = 8 * sizeInBytes;
	this->fprAtExpectedElements = 1.0;
	for (unsigned int numHashes = 1; numHashes < 5; numHashes++) {
		double tempFPR = calculateFPRGivenK(this->sizeInBits / this->numElementsExpected, numHashes);
		if (tempFPR < this->fprAtExpectedElements) {
			this->fprAtExpectedElements = tempFPR;
			this->numHashes = numHashes;
		}
	}
	//std::cout << "expected FPR Block512 (biased approximation): " << this->fprAtExpectedElements << std::endl;
	//std::cout << "num hashes BBF" << this->numHashes << std::endl;
	assert(numHashes >= 1);
	this->numBlocks = this->sizeInBits / 512;
	assert((this->sizeInBits / 512) == (sizeInBytes / 64));
}

template<class ValueType, class Hash>
selfBlockBloom512<ValueType, Hash>::selfBlockBloom512(const selfBlockBloom512<ValueType, Hash> &otherBloom) {
	this->sizeInBits = otherBloom.sizeInBits;
	this->bytes = (uint8_t*) aligned_alloc(64, otherBloom.sizeInBits/8);
	if (this->bytes == nullptr) {
		std::cout << "selfBlockBloom error in aligned alloc. Error code: " << strerror(errno);
	}
	this->numElementsExpected = otherBloom.numElementsExpected;
	std::memcpy(this->bytes, otherBloom.bytes, otherBloom.sizeInBits/8);
	this->fprAtExpectedElements = otherBloom.fprAtExpectedElements;
	this->numHashes = otherBloom.numHashes;
	this->numBlocks = otherBloom.numBlocks;
	this->blockUsages = new int[numBlocks];
	this->hash = otherBloom.hash;
}

template<class ValueType, class Hash>
selfBlockBloom512<ValueType, Hash>::selfBlockBloom512(const selfBlockBloom512<ValueType, Hash> &&otherBloom) {
	this->sizeInBits = otherBloom.sizeInBits;
	this->bytes = otherBloom->bytes;
	this->numElementsExpected = otherBloom.numElementsExpected;
	this->fprAtExpectedElements = otherBloom.fprAtExpectedElements;
	this->numHashes = otherBloom.numHashes;
	this->numBlocks = otherBloom.numBlocks;
	otherBloom->bytes = nullptr;
	this->hash = otherBloom.hash;
}

template<class ValueType, class Hash>
selfBlockBloom512<ValueType, Hash>& selfBlockBloom512<ValueType, Hash>::operator=(const selfBlockBloom512<ValueType, Hash>& otherBloom) { 
	if (this == &otherBloom) {
		return *this;
	}
	uint8_t* new_bytes = (uint8_t*) aligned_alloc(64, otherBloom->sizeInBits/8);
	std::memcpy(new_bytes, otherBloom->bytes, otherBloom->sizeInBits/8);
	if (this->bytes != NULL) {
		delete []this->bytes;
	}
	this->bytes = new_bytes;
	this->sizeInBits = otherBloom->sizeInBits;
	this->numElementsExpected = otherBloom->numElementsExpected;
	this->fprAtExpectedElements = otherBloom->fprAtExpectedElements;
	this->numHashes = otherBloom->numHashes;
	this->numBlocks = otherBloom->numBlocks;
	this->hash = otherBloom->hash;
	return *this;
}

template<class ValueType, class Hash>
selfBlockBloom512<ValueType, Hash>& selfBlockBloom512<ValueType, Hash>::operator=(const selfBlockBloom512<ValueType, Hash>&& otherBloom) {
	if (this != &otherBloom) {
		if (this->bytes != NULL) {
			delete []this->bytes;
		}
		this->bytes = otherBloom->bytes;
		this->sizeInBits = otherBloom->sizeInBits;
		this->numElementsExpected = otherBloom->numElementsExpected;
		this->fprAtExpectedElements = otherBloom->fprAtExpectedElements;
		this->numHashes = otherBloom->numHashes;
		this->numBlocks = otherBloom->numBlocks;
		this->hash = otherBloom->hash;
		otherBloom->bytes=nullptr;
		return *this;
	}	
}

template<class ValueType, class Hash>
selfBlockBloom512<ValueType, Hash>::~selfBlockBloom512() {
	delete []this->bytes;
}

template<class ValueType, class Hash>
forceinline
void selfBlockBloom512<ValueType, Hash>::setBit(const size_t blockOffset, const size_t positionInBlock) {
	size_t byte = positionInBlock >> 3;
	size_t full_byte = byte + blockOffset; 
	unsigned int mask = 1 << (positionInBlock & mask3block);
	bytes[full_byte] |= mask;
}

template<class ValueType, class Hash>
forceinline
bool selfBlockBloom512<ValueType, Hash>::testBit(const size_t blockOffset, const size_t positionInBlock) const {
	size_t byte = positionInBlock >> 3;
	size_t full_byte = byte + blockOffset;
	unsigned int mask = 1 << (positionInBlock & mask3block);
	return (bytes[full_byte] & mask) != 0;
}

forceinline
uint32_t mult_reduce_block(const uint32_t hash, const uint32_t n) {
  // http://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
  	return (uint32_t)(((uint64_t)hash * (uint64_t)n) >> 32);
}

template<class ValueType, class Hash>
forceinline
void selfBlockBloom512<ValueType, Hash>::addValue(const ValueType& x) {
	// compute 2 hash values and use double hashing. 
	uint64_t hash_full = hash.operator()(x);
	uint32_t hash1 = (uint32_t)(hash_full);
	uint32_t hash2 = (uint32_t)(hash_full >> 32); 
	// do first hash function
	uint32_t blockOffset = 64 * mult_reduce_block(hash1, this->numBlocks);
	for (unsigned int i = 1; i <= numHashes; i++) {
		uint32_t double_hash = hash1 + i * hash2;
		// which position to set
		uint32_t positionInBlock = double_hash & (511);
		setBit(blockOffset, positionInBlock);
	}
}

template<class ValueType, class Hash>
forceinline
bool selfBlockBloom512<ValueType, Hash>::testValue(const ValueType& x) const{
	// compute 2 hash values and use double hashing. 
	uint64_t hash_full = hash.operator()(x);
	uint32_t hash1 = (uint32_t)(hash_full);
	uint32_t hash2 = (uint32_t)(hash_full >> 32);
	// do first hash function
	uint32_t blockOffset = 64 * mult_reduce_block(hash1, this->numBlocks);
	unsigned int i = 1;
	bool returnVal = true;
	uint32_t double_hash = hash1;
	while (returnVal == true && i <= numHashes) {
		double_hash += hash2;
		uint32_t positionInBlock = double_hash & (512-1);
		returnVal = testBit(blockOffset, positionInBlock);
		i += 1;
	}
	//std::cout << i;
	return returnVal;
}

