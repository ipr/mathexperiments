/////////////////////////////////////
//
// CBigValue : class for handling simple
// arbitrary-size arithmetics and conversions.
//
// Author: Ilkka Prusi, 2011
// Contact: ilkka.prusi@gmail.com
//
// Some type information for reference:
// * int64_t/uint64_t - signed/unsigned 64bit integer
// * FFP - 32-bit non-IEEE value with mantissa and exponent, "fast floating-point"
// * float - IEEE compatible 32-bit floating point (single precision)
// * double - IEEE compatible 64-bit floating point (double precision)
// * extended - "long double" 80-bit IEEE compatible floating pointer
// * quadruple - 128-bit format (SPARC/PowerPC)
//


#include "BigValue.h"


////////// protected methods

void CBigValue::CreateBuffer(const size_t nBufSize)
{
	m_pBuffer = new uint8_t[nBufSize];
	m_nBufferSize = nBufSize;
	::memset(m_pBuffer, 0, m_nBufferSize);
}

void CBigValue::GrowBuffer(const size_t nBufSize)
{
	if (m_pBuffer == nullptr)
	{
		CreateBuffer(nBufSize);
	}
	else if (nBufSize > m_nBufferSize)
	{
		uint8_t *pBuffer = new uint8_t[nBufSize];
		::memset(pBuffer, 0, nBufSize);
		::memcpy(pBuffer, m_pBuffer, m_nBufferSize);
		delete m_pBuffer;
		m_pBuffer = pBuffer;
		m_nBufferSize = nBufSize;
	}
	/*
	else // not larger -> do nothing
	{
		
	}
	*/
}


///////// public methods

CBigValue::CBigValue(const int64_t value)
	: m_pBuffer(nullptr)
	, m_nBufferSize(0)
	, m_nScale(0)
	, m_bNegative(false)
{
	CreateBuffer(sizeof(int64_t));

	m_bNegative = (value & (1 << 63)) ? true : false;
	m_nScale = 0;

	// TODO: change complement values if negative?

	uint64_t bitmask = 0xFF;
	int maskshift = 0;
	const int count = sizeof(int64_t);
	for (int i = 0; i < count; i++)
	{
		if (m_bNegative == true)
		{
			m_pBuffer[i] = ~((value & bitmask) >> (maskshift*8));
		}
		else
		{
			m_pBuffer[i] = ((value & bitmask) >> (maskshift*8));
		}

		/*
		if (m_pBuffer[i] == 0)
		{
			// increase scale and overwrite with next byte
			m_nScale += 1;
		}
		*/
		bitmask <<= 8;
		maskshift += 1;
	}

	// we don't want sign-bit remaining there..
	m_pBuffer[count-1] = (m_pBuffer[count-1] ^ (1 << 7);
}

CBigValue::CBigValue(const uint64_t value)
	: m_pBuffer(nullptr)
	, m_nBufferSize(0)
	, m_nScale(0)
	, m_bNegative(false)
{
	CreateBuffer(sizeof(uint64_t));
	//::memcpy(m_pBuffer, &value, sizeof(uint64_t));

	uint64_t bitmask = 0xFF;
	int maskshift = 0;
	const int count = sizeof(uint64_t);
	for (int i = 0; i < count; i++)
	{
		m_pBuffer[i] = ((value & bitmask) >> (maskshift*8));
		/*
		if (m_pBuffer[i] == 0)
		{
			// increase scale and overwrite with next byte
			m_nScale += 1;
		}
		*/
		bitmask <<= 8;
		maskshift += 1;
	}
}

CBigValue::CBigValue(const double value)
	: m_pBuffer(nullptr)
	, m_nBufferSize(0)
	, m_nScale(0)
	, m_bNegative(false)
{
	CreateBuffer(8); // guess..
	// deconstruct double to buffer..
	m_bNegative = (value & (1 << 63)) ? true : false;
	m_nScale = ((value & (0x7FF << 53)) >> 53) ^ (1 << 53);

	// TODO: convert complement values if negative?

	// 52 bits -> even number of bits
	uint64_t bitmask = 0xFF;
	int maskshift = 0;
	const int count = (52/8);
	for (int i = 0; i < count; i++)
	{
		m_pBuffer[i] = ((value & bitmask) >> (maskshift*8));
		bitmask <<= 8;
		maskshift += 1;
	}

	/*
	uint64_t base = (value & 0xFFFFFFFFFFFFF); // 52 bits
	for (int i = 0; i < (52/8); i++)
	{
		m_pBuffer[i] = ((base & bitmask) >> (maskshift*8));
		bitmask <<= 8;
		maskshift += 1;
	}
	*/
}

CBigValue::CBigValue(const float value)
	: m_pBuffer(nullptr)
	, m_nBufferSize(0)
	, m_nScale(0)
	, m_bNegative(false)
{
	CreateBuffer(4); // guess..

	// deconstruct double to buffer..
	m_bNegative = (value & (1 << 31)) ? true : false;
	m_nScale = ((value & (0xFF << 23)) >> 23) ^ (1 << 23);

	// TODO: convert complement values if negative?

	// 23 bits -> odd number of bits
	uint32_t base = (value & 0x7FFFFF);
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

// expecting 4 bytes (32 bits), non-IEEE compatible (Q23.8?)
//
// exponent: power of two, excess-64, two's-complement values are adjusted upward
// by 64, thus changing $40 (-64) through $3F (+63) to $00 through $7F
//
// -> http://amigadev.elowar.com/read/ADCD_2.1/Libraries_Manual_guide/node047D.html
CBigValue& CBigValue::fromFFP32(const uint8_t *data)
{
	// 24 bits for mantissa, 8 for exponent
	CreateBuffer(4); // guess..

	// guessing sign pos..
	m_bNegative = (data[0] & (1 << 7)) ? true : false;

	// if all are zero we have (positive) zero -> nothing more to do
	if (data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 0)
	{
		m_nScale = 0; // verify this too, buffer should be zero already
		return *this;
	}

	uint8_t scale = data[3];

	uint32_t base = 0; // mantissa
	base += (data[0] ^ (1 << 7));
	base <<= 8;
	base += (data[1]);
	base <<= 8;
	base += (data[2]);
	base <<= 8;


	return *this;
}

/*
// expecting 8 bytes
CBigValue& CBigValue::fromFFP64(const uint8_t *data)
{
}
*/

// expecting 80 bits in "long double" format,
// not supported by MSVC++..
CBigValue& CBigValue::fromExtended(const uint8_t *data)
{

	return *this;
}

// 128-bits (16 bytes, SPARC/PowerPC)
CBigValue& CBigValue::fromQuadruple(const uint8_t *data)
{
	CreateBuffer(16);
	m_bNegative = (data[0] & (1 << 7)) ? true : false;

	uint32_t scale = 0;
	scale = ((data[0] & 0x7F) << 8) ^ (1 << 7);
	scale += data[1];
	m_nScale = scale;

	return *this;
}

// expecting buffer-format close to same..
CBigValue& CBigValue::fromBuffer(const uint8_t *pData, const size_t nSize, const bool bIsNegative, size_t nScale = 0)
{
	CreateBuffer(nSize);
	::memcpy(m_pBuffer, pData, nSize);
	m_bNegative = bIsNegative;
	m_nScale = nScale;

	return *this;
}

CBigValue CBigValue::operator + (const CBigValue &other) const
{
	// we don't know output size or scale yet
	// so we need to check our and given data for estimate..

	CBigValue value;

	return value;
}

CBigValue CBigValue::operator - (const CBigValue &other) const
{
	// we don't know output size or scale yet
	// so we need to check our and given data for estimate..

	CBigValue value;

	return value;
}

uint64_t CBigValue::operator() const
{
	// we know output limits so that simplifies..
	uint64_t value = 0;

	// scale&calculate, copy from buffer to value,
	// may have loss of precision..

	return value;
}

double CBigValue::operator() const
{
	// we know output limits so that simplifies..
	double value = 0.0;

	// scale&calculate, copy from buffer to value,
	// may have loss of precision..

	return value;
}

