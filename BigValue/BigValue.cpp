/////////////////////////////////////
//
// CBigValue : class for handling simple
// arbitrary-size arithmetics and conversions.
//
// Author: Ilkka Prusi, 2011
// Contact: ilkka.prusi@gmail.com
// Copyright (c): Ilkka Prusi
//
// Two main cases this class is meant to help with:
// 1) some databases and such support huge values,
// upto 30 bytes in some cases for which there is no native-type support..
// In principle, there is or should not be upper limit 
// on value size (except available memory), 
// but in practice values are always much smaller than that..
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


#include "BigValue.h"

#include <memory>


////////// protected methods

void CBigValue::CreateBuffer(const size_t nBufSize)
{
	if (m_pBuffer != nullptr)
	{
		delete m_pBuffer;
	}

	m_pBuffer = new uint8_t[nBufSize];
	m_nBufferSize = nBufSize;
	::memset(m_pBuffer, 0, m_nBufferSize);
}

void CBigValue::GrowBuffer(const size_t nBufSize)
{
	if (m_pBuffer == nullptr)
	{
		CreateBuffer(nBufSize);
		return;
	}

	if (nBufSize > m_nBufferSize)
	{
		uint8_t *pBuffer = new uint8_t[nBufSize];
		::memset(pBuffer, 0, nBufSize);
		::memcpy(pBuffer, m_pBuffer, m_nBufferSize);
		delete m_pBuffer;
		m_pBuffer = pBuffer;
		m_nBufferSize = nBufSize;
		return;
	}

	/*
	else // not larger -> do nothing
	*/
}

// shared way of handling IEEE-format mantissa of varying lengths:
// 24, 52, 64, 112 bits, including normalization-bit (handle it).
// IEEE formats define sign as sign of mantissa (not sign of exponent as with FFP)
// -> sign handled with exponent
void CBigValue::fromIEEEMantissa(const uint8_t *mantissa, const size_t size)
{
	bool isNormalized = (mantissa[0] & (1 << 7)) ? true : false;

	// TODO: check for odd-number of bits..
	// mantissa/exponent might not align to bytes (single-precision at least)
	bool oddSize = false;
	int count = (size/8);
	if ((size % 8) != 0)
	{
		oddSize = true;
		count += 1;
	}

	// TODO: convert complement values if negative?

	// note: check for byteorder.. (swap also? -> reverse)
	/*
	for (int i = 0, j = count; i < count && j > 0; i++, j--)
	{
		m_pBuffer[i] = (mantissa[j-1] & 0xFF);
	}
	*/

	for (int i = 0; i < count; i++)
	{
		if (i == 0
			&& oddSize == false)
		{
			// drop normalization-bit to get raw-value
			m_pBuffer[i] = (mantissa[0] ^ (1 << 7));
		}
		else if (i == 0
				&& oddSize == true)
		{
			// last byte needs masking (less than byte),
			// generate mask of suitable size
			int bits = (size - ((size/8)*i)) -1;
			int mask = 1;
			for (int shift = 1; shift < bits; shift++)
			{
				mask |= (1 << shift);
			}
			m_pBuffer[i] = (mantissa[i] & mask);
		}
		else
		{
			m_pBuffer[i] = (mantissa[i] & 0xFF);
		}
	}
}


///////// public methods

CBigValue::CBigValue(const int64_t value)
	: m_pBuffer(nullptr)
	, m_nBufferSize(0)
	, m_nScale(0)
	, m_bNegative(false)
{
	CreateBuffer(sizeof(int64_t));

	uint8_t *data = (uint8_t*)(&value);
	m_bNegative = (data[7] & (1 << 7)) ? true : false;
	m_nScale = 0;

	// TODO: need +1 in some byte when negative since sign takes one bit..
	// (we get a different absolute-value now..)
	uint16_t carry = (m_bNegative) ? 1 : 0;

	// byte-for-byte..
	const int count = sizeof(value);
	for (int i = 0; i < count; i++)
	{
		// change complement values if negative
		if (m_bNegative == true)
		{
			// use carry for simple increment
			carry += ((uint8_t)~(data[i] & 0xFF));
			m_pBuffer[i] = (carry & 0xFF);
			carry >>= 8;
		}
		else if (m_bNegative == false)
		{
			m_pBuffer[i] = (data[i] & 0xFF);
		}
	}
}

CBigValue::CBigValue(const uint64_t value)
	: m_pBuffer(nullptr)
	, m_nBufferSize(0)
	, m_nScale(0)
	, m_bNegative(false)
{
	CreateBuffer(sizeof(uint64_t));

	uint8_t *data = (uint8_t*)(&value);
	m_bNegative = false;
	m_nScale = 0;

	// byte-for-byte..
	for (int i = 0; i < sizeof(value); i++)
	{
		m_pBuffer[i] = (data[i] & 0xFF);
	}
}

CBigValue::CBigValue(const double value)
	: m_pBuffer(nullptr)
	, m_nBufferSize(0)
	, m_nScale(0)
	, m_bNegative(false)
{
	CreateBuffer(8); // guess..

	// deconstruct value to buffer..

	uint8_t *data = (uint8_t*)(&value);

	m_bNegative = (data[7] & (1 << 7)) ? true : false;

	// 11 bits
	m_nScale = (data[7] ^ (1 << 7));
	m_nScale <<= 1;
	m_nScale += ((data[6] & (0xF << 4)) >> 4);

	// 52 bits -> even number of bits
	fromIEEEMantissa(data + 1, 52);
}

CBigValue::CBigValue(const float value)
	: m_pBuffer(nullptr)
	, m_nBufferSize(0)
	, m_nScale(0)
	, m_bNegative(false)
{
	CreateBuffer(4); // guess..

	// deconstruct value to buffer..

	uint8_t *data = (uint8_t*)(&value);

	m_bNegative = (data[3] & (1 << 31)) ? true : false;
	//m_nScale = ((value & (0xFF << 23)) >> 23) ^ (1 << 23);
	m_nScale = (data[3] ^ (1 << 7));
	m_nScale <<= 1;
	m_nScale |= (data[2] & (1 << 7));

	// 23 bits -> odd number of bits
	fromIEEEMantissa(data + 1, 23);
}

CBigValue::CBigValue(const CBigValue &other)
	: m_pBuffer(nullptr)
	, m_nBufferSize(0)
	, m_nScale(0)
	, m_bNegative(false)
{
	// use helper..
	fromBuffer(other.m_pBuffer, other.m_nBufferSize, other.m_bNegative, other.m_nScale);
}

CBigValue::CBigValue(void)
	: m_pBuffer(nullptr)
	, m_nBufferSize(0)
	, m_nScale(0)
	, m_bNegative(false)
{}

CBigValue::~CBigValue(void)
{
	if (m_pBuffer != nullptr)
	{
		delete m_pBuffer;
		m_pBuffer = nullptr;
	}
}

// scale value to given scale
// TODO: do we keep scale as base-2 or base-10..?
CBigValue& CBigValue::scaleTo(const size_t nScale)
{
	if (nScale == m_nScale)
	{
		// same -> do nothing
		return *this;
	}

	if (nScale < m_nScale)
	{
		// check lower bytes how much "downwards" we can scale?
		// or trust user and allow loss of precision?
		// -> trust user..
		size_t diff = (m_nScale-nScale);

		// something like this maybe..
		::memmove(m_pBuffer, m_pBuffer + diff, m_nBufferSize - diff);

		m_nScale = diff;

		return *this;
	}
	else
	{
		// just scale "upwards" (add zero bytes if necessary)
	}

	return *this;
}


// expecting 4 bytes (32 bits), non-IEEE compatible (Q23.8?),
// always big-endian value
//
// exponent: power of two, excess-64, two's-complement values are adjusted upward
// by 64, thus changing $40 (-64) through $3F (+63) to $00 through $7F
//
// note: sign-bit is highest in exponent (sign of exponent), not in mantissa:
// MMMM MMMM MMMM MMMM MMMM MMMM SEEE EEEE
// 31     24 23                8 7       0
// (description from Paul Overaa's Amiga Assembler book)
// -> try to find Motorola's manuals also..
//
// Also see descriptions:
// -> http://gega.homelinux.net/AmigaDevDocs/lib_35.html
// -> http://amigadev.elowar.com/read/ADCD_2.1/Libraries_Manual_guide/node047D.html
// -> http://www.tbs-software.com/guide/index.php?guide=rkm_libraries.doc%2Flib_35.guide
//
CBigValue& CBigValue::fromFFP32(const uint8_t *data)
{
	// 24 bits for mantissa, 8 for exponent
	CreateBuffer(4); // guess..

	// if all are zero we have (positive) zero -> nothing more to do
	if (data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 0)
	{
		// buffer should be zero already
		m_nScale = 0; // verify this too
		m_bNegative = true; // positive zero
		return *this;
	}

	// guessing sign pos..
	m_bNegative = (data[3] & (1 << 7)) ? true : false;
	m_nScale = (data[3] ^ (1 << 7)); // get exponent, withough sign-bit

	/*
	uint32_t base = 0; // mantissa
	base += (data[0] ^ (1 << 7));
	base <<= 8;
	base += (data[1]);
	base <<= 8;
	base += (data[2]);
	base <<= 8;
	*/

	m_pBuffer[2] = (data[0] ^ (1 << 7));
	m_pBuffer[1] = (data[1] & 0xFF);
	m_pBuffer[0] = (data[2] & 0xFF);

	return *this;
}

/*
// expecting 8 bytes
CBigValue& CBigValue::fromFFP64(const uint8_t *data)
{
}
*/

// expecting 80 bits in "long double" format
// note: avoid using typedef here since 
// some compilers mismanage that (silent truncation/conversion)
// (such as MSVC++ just silently truncates it to common double..),
// always big-endian value
//
// -> see http://www.mactech.com/articles/mactech/Vol.06/06.01/SANENormalized/index.html
//
CBigValue& CBigValue::fromExtended(const uint8_t *data)
{
	CreateBuffer(8); // guess..
	m_bNegative = (data[0] & (1 << 7)) ? true : false;

	// 15-bit exponent
	uint32_t scale = 0;
	scale = ((data[0] & 0x7F) << 8) ^ (1 << 7);
	scale += data[1];
	m_nScale = scale;

	// note: explicit integer bit exists
	// in highest bit of significand (mantissa)
	// between exponent and significand
	// ->
	// 63 bits -> odd number of bits
	fromIEEEMantissa(data + 2, 63);

	return *this;
}

// 128-bits (16 bytes, SPARC/PowerPC)
// note: avoid using typedef here since 
// some compilers mismanage that (silent truncation/conversion),
// always big-endian value
//
CBigValue& CBigValue::fromQuadruple(const uint8_t *data)
{
	CreateBuffer(16);
	m_bNegative = (data[0] & (1 << 7)) ? true : false;

	// 15-bit exponent
	uint32_t scale = 0;
	scale = ((data[0] & 0x7F) << 8) ^ (1 << 7);
	scale += data[1];
	m_nScale = scale;

	// 112 bits -> even number of bits
	fromIEEEMantissa(data + 2, 112);

	return *this;
}

// expecting buffer-format close to same..
CBigValue& CBigValue::fromBuffer(const uint8_t *pData, const size_t nSize, const bool bIsNegative, size_t nScale)
{
	CreateBuffer(nSize);
	::memcpy(m_pBuffer, pData, nSize);
	m_bNegative = bIsNegative;
	m_nScale = nScale;

	return *this;
}

CBigValue& CBigValue::operator = (const CBigValue &other)
{
	// avoid self-assignment
	if (&other == this)
	{
		return *this;
	}
	fromBuffer(other.m_pBuffer, other.m_nBufferSize, other.m_bNegative, other.m_nScale);
	return *this;
}

CBigValue CBigValue::operator + (const CBigValue &other) const
{
	// we don't know output size or scale yet
	// so we need to check our and given data for estimate..

	// TODO:
	// also need to check scaling of both,
	// if either is negative, buffer sizes etc.

	CBigValue value;
	value.CreateBuffer(m_nBufferSize+1); // guess sufficient size for now..

	uint8_t *pother = other.m_pBuffer;
	/*
	if (m_nScale > other.m_nScale)
	{
		// need temp and change scaling,
		// memmove() to align values..
		//pother = new uint8_t...
		//other.scaleTo(m_nScale);
	}
	*/

	// for larger than single element, keep overflow to next
	uint16_t carry = 0;

	// TODO: likely different buffer sizes also..
	for (size_t i = 0, j = 0; i < m_nBufferSize || j < other.m_nBufferSize; i++, j++)
	{
		if (i >= m_nBufferSize)
		{
			carry += pother[j];
		}
		else if (j >= other.m_nBufferSize)
		{
			carry += m_pBuffer[i];
		}
		else
		{
			carry += (m_pBuffer[i] + pother[j]);
		}
		value.m_pBuffer[i] += (carry & 0xFF);
		carry >>= 8;
	}

	if (carry > 0)
	{
		// overflow to destination (which should be larger)
		value.m_pBuffer[value.m_nBufferSize -1] += (carry &0xFF);
	}

	return value;
}

CBigValue CBigValue::operator - (const CBigValue &other) const
{
	// we don't know output size or scale yet
	// so we need to check our and given data for estimate..

	CBigValue value;

	return value;
}

CBigValue::operator uint64_t() const
{
	// we know output limits so that simplifies..
	uint64_t value = 0;

	// scale&calculate, copy from buffer to value,
	// may have loss of precision..

	// test, just give absolute value without sign..
	// (this might not be what is wanted but for testing),
	// also scaling is not done now..
	/*
	CBigValue *pSrc = this;
	if (m_nScale != 0)
	{
		pSrc = new CBigValue(this);
		pSrc->scaleTo(0);
	}
	*/

	uint8_t *data = (uint8_t*)(&value);

	// byte-for-byte.. just set absolute value,
	// user might want something else some day..
	//
	for (int i = 0; i < sizeof(value); i++)
	{
		data[i] = m_pBuffer[i];
	}

	return value;
}

CBigValue::operator double() const
{
	// we know output limits so that simplifies..
	double value = 0.0;

	// scale&calculate, copy from buffer to value,
	// may have loss of precision..

	return value;
}

