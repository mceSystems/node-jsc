/**
 * This source code is licensed under the terms found in the LICENSE file in
 * node-jsc's root directory.
 */

#pragma once

#include <JavaScriptCore/JSBigInt.h>

/* JSC::JSBigInt's interface is currently a bit limited and doesn't expose much to the public.
 * Some definitions and functionality like accessing digits directly is currently not part
 * of JSBigInt's public api. Other functionality we need, like uint64_t conversion functions,
 * isn't currently implemented. But, as JSBigInt still seems to be under develompment,
 * I prefer not to modify it for now.
 *
 * Thus, this header contains "dirty" JSBigInt helpers and consts. It relies on:
 * - Consts and definitions from JSBigInt.
 * - JSBigInt's internal layout and implementation, mainly:
 *    - Digits are pointer size.
 *    - The digits are stored in an array which follows the JSBigInt instance in memory,
 *      and is not part of the JSBigInt object itself.
 **/

namespace v8 { namespace jscshim
{

// Based on definitions from JSC::JSBigInt
constexpr unsigned int BITS_PER_BYTE = 8;

using JSBigIntDigit = uintptr_t;
constexpr unsigned JSBigIntDigitBits = sizeof(JSBigIntDigit) * BITS_PER_BYTE;

constexpr unsigned JSBigIntMaxLength = 1024 * 1024 / (sizeof(void *) * BITS_PER_BYTE);

// Based on JSC::JSBigInt::offsetOfData
constexpr size_t JSBigIntOffsetOfData = WTF::roundUpToMultipleOf<sizeof(JSBigIntDigit)>(sizeof(JSC::JSBigInt));

// Based JSC::JSBigInt::dataStorage()
JSBigIntDigit * GetJSBigIntData(JSC::JSBigInt * bigInt)
{	
	return reinterpret_cast<JSBigIntDigit *>(reinterpret_cast<char *>(bigInt) + JSBigIntOffsetOfData);
}

// Based on JSC::JSBigInt::createFrom(VM& vm, int64_t value)
JSC::JSBigInt * createBigIntFromUint64(JSC::VM& vm, uint64_t value)
{
	if (!value)
	{
		return JSC::JSBigInt::createZero(vm);
	}

	if (8 == sizeof(jscshim::JSBigIntDigit))
	{
		JSC::JSBigInt * bigInt = JSC::JSBigInt::createWithLength(vm, 1);

		jscshim::GetJSBigIntData(bigInt)[0] = static_cast<jscshim::JSBigIntDigit>(value);
		bigInt->setSign(false);

		return bigInt;
	}

	JSC::JSBigInt * bigInt = JSC::JSBigInt::createWithLength(vm, 2);
	bigInt->setSign(false);

	// Low bits, then high bits
	jscshim::JSBigIntDigit * digits = jscshim::GetJSBigIntData(bigInt);
	digits[0] = static_cast<jscshim::JSBigIntDigit>(value & 0xffffffff);
	digits[1] = static_cast<jscshim::JSBigIntDigit>((value >> 32) & 0xffffffff);

	return bigInt;
}

// Based on v8's MutableBigInt::GetRawBits (objects/bigint.cc)
uint64_t GetBigIntRawBits(JSC::JSBigInt * x, bool * lossless)
{
	if (lossless)
	{
		*lossless = true;
	}

	int len = x->length();
	if (0 == len)
	{
		return 0;
	}

	if (lossless && (len > (64 / JSBigIntDigitBits)))
	{
		*lossless = false;
	}

	jscshim::JSBigIntDigit * digits = jscshim::GetJSBigIntData(x);

	uint64_t raw = static_cast<uint64_t>(digits[0]);
	if ((32 == JSBigIntDigitBits) && (len > 1))
	{
		raw |= static_cast<uint64_t>(digits[1]) << 32;
	}

	// Simulate two's complement. MSVC dislikes "-raw".
	return x->sign() ? ((~raw) + 1u) : raw;
}

}} // v8::jscshim