/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"
#include "shim/helpers.h"
#include "shim/BigIntHelpers.h"

#include <JavaScriptCore/JSCInlines.h>

#define GET_JSC_THIS() v8::jscshim::GetJscCellFromV8<JSC::JSBigInt>(this)

namespace v8
{

Local<BigInt> BigInt::New(Isolate* isolate, int64_t value)
{
	jscshim::Isolate * shimIsolate = jscshim::V8IsolateToJscShimIsolate(isolate);

	JSC::JSBigInt * bigInt = JSC::JSBigInt::createFrom(shimIsolate->VM(), value);
	return Local<BigInt>::New(JSC::JSValue(bigInt));
}

Local<BigInt> BigInt::NewFromUnsigned(Isolate* isolate, uint64_t value)
{
	jscshim::Isolate * shimIsolate = jscshim::V8IsolateToJscShimIsolate(isolate);
	return Local<BigInt>(jscshim::createBigIntFromUint64(shimIsolate->VM(), value));
}

// Based in v8's BigInt::FromWords64 (objects/bigint.cc)
MaybeLocal<BigInt> BigInt::NewFromWords(Local<Context> context, int sign_bit, int word_count, const uint64_t * words)
{
	jscshim::GlobalObject * global = jscshim::GetGlobalObjectForV8Context(*context);

	if ((word_count < 0) || (word_count > (jscshim::JSBigIntMaxLength / (64 / jscshim::JSBigIntDigitBits)))) {
		// Exception message equals to v8's MessageTemplate::kBigIntTooBig
		DECLARE_SHIM_EXCEPTION_SCOPE(*context);
		SHIM_THROW_EXCEPTION(JSC::createRangeError(global->globalExec(), ASCIILiteral("Maximum BigInt size exceeded")));

		return MaybeLocal<BigInt>();
	}

	if (0 == word_count)
	{
		JSC::JSBigInt * bigInt = JSC::JSBigInt::createZero(global->vm());
		return Local<BigInt>::New(JSC::JSValue(bigInt));
	}

	unsigned int length = (64 / jscshim::JSBigIntDigitBits) * word_count;
	if ((32 == jscshim::JSBigIntDigitBits) && (words[word_count - 1] <= (1ULL << 32)))
	{
		length--;
	}

	JSC::JSBigInt * bigInt = JSC::JSBigInt::createWithLength(global->vm(), length);
	bigInt->setSign(sign_bit);

	jscshim::JSBigIntDigit * digits = jscshim::GetJSBigIntData(bigInt);
	if (64 == jscshim::JSBigIntDigitBits)
	{
		memcpy(digits, words, word_count * sizeof(uint64_t));
	}
	else
	{
		for (int i = 0; i < length; i += 2)
		{
			jscshim::JSBigIntDigit lo = static_cast<jscshim::JSBigIntDigit>(words[i / 2]);
			jscshim::JSBigIntDigit hi = static_cast<jscshim::JSBigIntDigit>(words[i / 2] >> 32);
			digits[i] = lo;
			if (i + 1 < length)
			{
				digits[i + 1] = hi;
			}
		}
	}
	
	return Local<BigInt>::New(JSC::JSValue(bigInt));
}

// Based on v8's BigInt::AsUint64 (objects/bigint.cc)
uint64_t BigInt::Uint64Value(bool * lossless) const
{
	JSC::JSBigInt * thisBigInt = GET_JSC_THIS();

	uint64_t result = jscshim::GetBigIntRawBits(thisBigInt, lossless);
	if (lossless && thisBigInt->sign())
	{
		*lossless = false;
	}

	return result;
}

// Based on v8's BigInt::AsInt64 (objects/bigint.cc)
int64_t BigInt::Int64Value(bool * lossless) const
{
	JSC::JSBigInt * thisBigInt = GET_JSC_THIS();

	int64_t result = static_cast<int64_t>(jscshim::GetBigIntRawBits(thisBigInt, lossless));
	if (lossless && ((result < 0) != thisBigInt->sign()))
	{
		*lossless = false;
	}

	return result;
}

// Based on v8's BigInt::Words64Count (objects/bigint.cc)
int BigInt::WordCount() const
{
	unsigned int lengthInDigits = GET_JSC_THIS()->length();

	return (lengthInDigits / (64 / jscshim::JSBigIntDigitBits)) +
		   ((32 == jscshim::JSBigIntDigitBits) && (1 == lengthInDigits % 2) ? 1 : 0);
}

// Based on v8's BigInt::ToWordsArray64 (objects/bigint.cc)
void BigInt::ToWordsArray(int * sign_bit, int * word_count, uint64_t * words) const
{
	ASSERT(sign_bit);
	ASSERT(word_count);

	JSC::JSBigInt * thisBigInt = GET_JSC_THIS();

	*sign_bit = thisBigInt->sign();

	int availableWords = *word_count;
	*word_count = WordCount();
	if (availableWords == 0)
	{
		return;
	}
	ASSERT(words);

	unsigned int len = thisBigInt->length();
	jscshim::JSBigIntDigit * digits = jscshim::GetJSBigIntData(thisBigInt);
	if (64 == jscshim::JSBigIntDigitBits)
	{
		for (int i = 0; (i < len) && (i < availableWords); ++i)
		{
			words[i] = digits[i];
		}
	}
	else
	{
		for (int i = 0; (i < len) && (availableWords > 0); i += 2) 
		{
			uint64_t lo = digits[i];
			uint64_t hi = (i + 1) < len ? digits[i + 1] : 0;
			words[i / 2] = lo | (hi << 32);
			availableWords--;
		}
	}
}

} // v8