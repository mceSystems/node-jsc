/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"
#include <JavaScriptCore/JSArray.h>
#include <JavaScriptCore/JSCInlines.h>

#define GET_JSC_THIS() v8::jscshim::GetJscCellFromV8<JSC::JSArray>(this)

namespace v8
{

uint32_t Array::Length() const
{
	return GET_JSC_THIS()->length();
}

Local<Array> Array::New(Isolate * isolate, int length)
{
	DECLARE_SHIM_EXCEPTION_SCOPE(isolate);

	// Taken from v8's Array::New
	unsigned int realLegnth = length > 0 ? length : 0;

	JSC::JSObject * newArray = JSC::constructEmptyArray(jscshim::GetExecStateForV8Isolate(isolate), 
														nullptr, 
														realLegnth);
	return Local<Array>::New(JSC::JSValue(newArray));
}

} // v8