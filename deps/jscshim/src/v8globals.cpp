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

Local<Primitive> Undefined(Isolate* isolate)
{
	return Local<Primitive>::New(JSC::jsUndefined());
}

Local<Primitive> Null(Isolate* isolate)
{
	return Local<Primitive>::New(JSC::jsNull());
}

Local<Boolean> True(Isolate* isolate)
{
	return Local<Boolean>::New(JSC::JSValue(JSC::JSValue::JSTrue));
}

Local<Boolean> False(Isolate* isolate)
{
	return Local<Boolean>::New(JSC::JSValue(JSC::JSValue::JSFalse));
}

} // v8