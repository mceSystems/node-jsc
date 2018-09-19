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

int64_t Integer::Value() const
{
	return static_cast<int64_t>(jscshim::GetValue(this).asNumber());
}

int32_t Int32::Value() const
{
	return jscshim::GetValue(this).asInt32();
}

uint32_t Uint32::Value() const
{
	// JSC might store unsigned integers as doubles (see JSValue::JSValue(unsigned i))
	JSC::JSValue thisValue = jscshim::GetValue(this);
	if (thisValue.isDouble())
	{
		return static_cast<uint32_t>(thisValue.asDouble());
	}

	return thisValue.asUInt32();
}

} // v8