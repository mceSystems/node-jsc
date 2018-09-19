/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include <stdint.h>

/* In order to properly define a "lite" version of JSValue, we need WTF's JSVALUE32_64\JSVALUE64 
 * definitions. Including wtf/Platform.h will provide the needed values, but will also 
 * include a lot of WTF macros\definitions into v8.h and node. For now, in order to avoid that, 
 * we'll just copy the macros\definitions we need, and "undefine" them at the end of this header.
 *
 * So,the following macros\definitions were taken from wtf/Platform.h, replacing the 
 * COMPILER macros from wtf/Compiler.h with "manual" checks.
 */

#if !defined(USE_JSVALUE64) && !defined(USE_JSVALUE32_64)
	#if defined(__GNUC__)
		/* __LP64__ is not defined on 64bit Windows since it uses LLP64. Using __SIZEOF_POINTER__ is simpler. */
		#if __SIZEOF_POINTER__ == 8
			#define USE_JSVALUE64 1
		#elif __SIZEOF_POINTER__ == 4
			#define USE_JSVALUE32_64 1
		#else
			#error "Unsupported pointer width"
		#endif
	#elif defined(_MSC_VER)
		#if defined(_WIN64)
			#define USE_JSVALUE64 1
		#else
			#define USE_JSVALUE32_64 1
		#endif
	#else
		/* This is the most generic way. But in OS(DARWIN), Platform.h can be included by sandbox definition file (.sb).
		* At that time, we cannot include "stdint.h" header. So in the case of known compilers, we use predefined constants instead. */
		#include <stdint.h>
		#if UINTPTR_MAX > UINT32_MAX
			#define USE_JSVALUE64 1
		#else
			#define USE_JSVALUE32_64 1
		#endif
	#endif
#endif /* !defined(USE_JSVALUE64) && !defined(USE_JSVALUE32_64) */

#if defined(__GNUC__)
	#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		#define WTF_CPU_BIG_ENDIAN 1
	#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		#define WTF_CPU_LITTLE_ENDIAN 1
	#elif __BYTE_ORDER__ == __ORDER_PDP_ENDIAN__
		#define WTF_CPU_MIDDLE_ENDIAN 1
	#else
		#error "Unknown endian"
	#endif
#else
	#if defined(WIN32) || defined(_WIN32)
		/* Windows only have little endian architecture. */
		#define WTF_CPU_LITTLE_ENDIAN 1
	#else
		#include <sys/types.h>
		#if __has_include(<endian.h>)
			#include <endian.h>
			#if __BYTE_ORDER == __BIG_ENDIAN
				#define WTF_CPU_BIG_ENDIAN 1
			#elif __BYTE_ORDER == __LITTLE_ENDIAN
				#define WTF_CPU_LITTLE_ENDIAN 1
			#elif __BYTE_ORDER == __PDP_ENDIAN
				#define WTF_CPU_MIDDLE_ENDIAN 1
			#else
				#error "Unknown endian"
			#endif
		#else
			#if __has_include(<machine/endian.h>)
				#include <machine/endian.h>
			#else
				#include <sys/endian.h>
			#endif
			#if BYTE_ORDER == BIG_ENDIAN
				#define WTF_CPU_BIG_ENDIAN 1
			#elif BYTE_ORDER == LITTLE_ENDIAN
				#define WTF_CPU_LITTLE_ENDIAN 1
			#elif BYTE_ORDER == PDP_ENDIAN
				#define WTF_CPU_MIDDLE_ENDIAN 1
			#else
				#error "Unknown endian"
			#endif
		#endif
	#endif
#endif

#define USE(WTF_FEATURE) (defined USE_##WTF_FEATURE  && USE_##WTF_FEATURE)
#define CPU(WTF_FEATURE) (defined WTF_CPU_##WTF_FEATURE  && WTF_CPU_##WTF_FEATURE)

namespace JSC {

class JSCell;

// See EncodedValueDescriptor in JSCJSValue.h for the original union
union EncodedValueDescriptor {
	int64_t asInt64;
#if USE(JSVALUE32_64)
	double asDouble;
#elif USE(JSVALUE64)
	JSCell* ptr;
#endif

#if CPU(BIG_ENDIAN)
	struct {
		int32_t tag;
		int32_t payload;
	} asBits;
#else
	struct {
		int32_t payload;
		int32_t tag;
	} asBits;
#endif
};

class JSValue {
private:
	EncodedValueDescriptor u;

public:
	inline JSValue(const JSValue& other)
	{
		u.asInt64 = other.u.asInt64;
	}

	inline bool operator==(const JSValue& other) const
	{
		return u.asInt64 == other.u.asInt64;
	}

	inline bool operator!=(const JSValue& other) const
	{
		return u.asInt64 != other.u.asInt64;
	}

#if USE(JSVALUE32_64)
	enum { EmptyValueTag = 0xfffffffa };

	inline JSValue()
	{
		u.asBits.tag = EmptyValueTag;
		u.asBits.payload = 0;
	}

	inline bool isEmpty() const { return u.asBits.tag == EmptyValueTag; }

#else // !USE(JSVALUE32_64) i.e. USE(JSVALUE64)
	#define ValueEmpty   0x0ll

	inline JSValue()
	{
		u.asInt64 = ValueEmpty;
	}

	inline bool isEmpty() const
	{
		return u.asInt64 == ValueEmpty;
	}

	#undef ValueEmpty

#endif

#undef USE
#undef CPU
#undef WTF_CPU_LITTLE_ENDIAN
#undef WTF_CPU_BIG_ENDIAN
#undef WTF_CPU_MIDDLE_ENDIAN
#undef USE_JSVALUE32_64
#undef USE_JSVALUE64

};

} // JSC