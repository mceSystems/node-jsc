/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"

#include <JavaScriptCore/StringObject.h>
#include <JavaScriptCore/JSCInlines.h>

#define GET_JSC_THIS() v8::jscshim::GetJscCellFromV8<JSC::StringObject>(this)

namespace v8
{

Local<Value> StringObject::New(Local<String> value)
{
	jscshim::GlobalObject * global = jscshim::GetCurrentGlobalObject();

	JSC::JSObject * newString = JSC::constructString(global->vm(), global, jscshim::GetValue(*value));
	return Local<Value>::New(JSC::JSValue(newString));
}

Local<String> StringObject::ValueOf() const
{
	return Local<String>::New(GET_JSC_THIS()->internalValue());
}

} // v8