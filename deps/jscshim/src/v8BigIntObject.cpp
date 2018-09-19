/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"
#include "shim/helpers.h"

#include <JavaScriptCore/BigIntObject.h>
#include <JavaScriptCore/JSCInlines.h>

#define GET_JSC_THIS() v8::jscshim::GetJscCellFromV8<JSC::BigIntObject>(this)

namespace v8
{

Local<Value> BigIntObject::New(Isolate* isolate, int64_t value)
{
	jscshim::GlobalObject * global = jscshim::GetGlobalObjectForV8Isolate(isolate);

	JSC::JSBigInt * bigInt = JSC::JSBigInt::createFrom(global->vm(), value);
	JSC::JSObject * bigIntObject = JSC::BigIntObject::create(global->vm(), global, bigInt);

	return Local<Value>::New(JSC::JSValue(bigIntObject));
}

Local<BigInt> BigIntObject::ValueOf() const
{
	return Local<BigInt>::New(JSC::JSValue(GET_JSC_THIS()->internalValue()));
}

} // v8