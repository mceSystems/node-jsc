/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"

#include <JavaScriptCore/JSCInlines.h>

namespace v8
{

Local<Integer> v8::Integer::New(Isolate * isolate, int32_t value)
{
	return Local<Integer>::New(JSC::jsNumber(value));
}

Local<Integer> Integer::NewFromUnsigned(Isolate * isolate, uint32_t value)
{
	return Local<Integer>::New(JSC::jsNumber(value));
}

} // v8