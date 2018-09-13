/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#define _STRINGIFY(s) #s
#define STRINGIFY(s) _STRINGIFY(s)

// Based on folly's Portability.h (webkit/PerformanceTests/StitchMarker/folly/folly/Portability.h)
#ifdef _MSC_VER
	#define PACK_STRUCT(s) __pragma(pack(push, 1)) \
						   s; \
						   __pragma(pack(pop))
#elif defined(__clang__) || defined(__GNUC__)
	#define PACK_STRUCT(s) s __attribute__((__packed__));
#else
	#define PACK_STRUCT(s) s
#endif

#ifdef __BIG_ENDIAN__
	#define JSCHIM_HTONL(n) n
#else
	// Taken from https://stackoverflow.com/a/2182184
	#define JSCHIM_HTONL(n) ((((n) >> 24) & 0xff)     | \
							 (((n) << 8)  & 0xff0000) | \
							 (((n) >> 8)  & 0xff00)   | \
							 (((n) << 24) & 0xff000000))
#endif