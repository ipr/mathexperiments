/////////////////////////////////////
//
// CBigValue : class for handling simple
// arbitrary-size arithmetics and conversions.
//
// Author: Ilkka Prusi, 2011
// Contact: ilkka.prusi@gmail.com
//
// Two main cases this class is meant to help with:
// 1) some databases and such support huge values,
// upto 30 bytes in some cases for which there is no native-type support..
//
// 2) some native variable types are not supported in other 
// platforms and conversion helper is necessary:
// reduction of precision may be acceptable in some cases
// but not in all of them, customized handling may be needed..
//
//
// Some type information for reference:
// * int64_t/uint64_t 
//   - signed/unsigned 64bit integer
// * FFP "fast floating-point"
//   - 32-bit non-IEEE value with mantissa and exponent
//   - considered fixed-point, always normalized (no hidden bit)
//   - sign-bit is sign of exponent
//   - exponent is power of two, excess-64 notation
// * float 
//   - IEEE compatible 32-bit floating point (single precision)
//   - mantissa includes hidden bit for normalized value
//   - sign-bit is sign of mantissa
// * double 
//   - IEEE compatible 64-bit floating point (double precision)
//   - mantissa includes hidden bit for normalized value
//   - sign-bit is sign of mantissa
// * extended - "long double" 80-bit IEEE compatible floating pointer
//   - IEEE compatible 80-bit floating point (extended precision)
//   - mantissa includes hidden bit for normalized value
//   - sign-bit is sign of mantissa
// * quadruple - 128-bit format (SPARC/PowerPC)
//   - IEEE compatbile 128-bit floating point (quadruple precision)
//   - mantissa includes hidden bit for normalized value
//   - sign-bit is sign of mantissa
//   - note: separate non-IEEE 128-bit format found in IBM System/370..
//

#ifndef BIGVALUE_H
#define BIGVALUE_H

#include <stdint.h>


// for future, allow external arithmetic operators
// for extensions (log, square etc.)
//class CBigOperator;

/*
// TODO: ffp float
// avoid standard "float" as that's IEEE format float..
typedef uint32_t FFPFloat;

// TODO: extended double
// "extended" keyword is not supported by VC++
// -> we might define it to hold 10 bytes..
union extended
{
	struct ext
	{
		uint32_t low;
		uint32_t mid;
		uint16_t hi;
	} ext;
	uint8_t data[10];
}

// TODO: quadruple
// could be as __float128 if compiler supports.. (which it doesn't?)
union quadruple
{
	struct ext
	{
		uint32_t low;
		uint32_t midlo;
		uint32_t midhi;
		uint32_t hi;
	} ext;
	uint8_t data[16];
}
*/


class CBigValue
{
protected:
	uint8_t *m_pBuffer;
	size_t m_nBufferSize;

	//size_t m_nUsedSize; // is this needed?

	size_t m_nScale; // power of 10 scale
	bool m_bNegative; // if negative

	void CreateBuffer(const size_t nBufSize);
	void GrowBuffer(const size_t nBufSize);

	void fromIEEEMantissa(const uint8_t *mantissa, const size_t size);

public:
	explicit CBigValue(const int64_t value);
	explicit CBigValue(const uint64_t value);
	explicit CBigValue(const double value);
	explicit CBigValue(const float value);

	CBigValue(const CBigValue &other);

	CBigValue(void);
	~CBigValue(void);

	// scale value to given
	CBigValue& scaleTo(const size_t nScale);

	// "fast-floating point" format
	// expecting 4 bytes
	CBigValue& fromFFP32(const uint8_t *data);
	// expecting 8 bytes
	//CBigValue& fromFFP64(const uint8_t *data);

	// expecting 80 bits in "long double" format
	// note: avoid using typedef here since 
	// some compilers mismanage that (silent truncation/conversion)
	CBigValue& fromExtended(const uint8_t *data);

	// 128-bits (16 bytes, SPARC/PowerPC)
	// note: avoid using typedef here since 
	// some compilers mismanage that (silent truncation/conversion)
	CBigValue& fromQuadruple(const uint8_t *data);

	// other buffer "as-is" ?
	CBigValue& fromBuffer(const uint8_t *pData, const size_t nSize, const bool bIsNegative, size_t nScale = 0);

	CBigValue& operator = (const CBigValue &other) const;

	CBigValue operator + (const CBigValue &other) const;
	CBigValue operator - (const CBigValue &other) const;

	// TODO: for extending artihmetics etc.
	//CBigValue operand(CBigOperator *pOp) const;

	uint64_t operator() const;
	double operator() const;

	friend class CBigValue;
};

// pure virtual operand type
// for extending artihmetics etc.
/*
class CBigOperator
{
};
*/

#endif // BIGVALUE_H

