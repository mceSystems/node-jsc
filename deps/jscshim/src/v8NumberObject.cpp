/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"

#include <JavaScriptCore/NumberObject.h>
#include <JavaScriptCore/JSCInlines.h>

#define GET_JSC_THIS() v8::jscshim::GetJscCellFromV8<JSC::NumberObject>(this)

namespace v8
{

Local<Value> NumberObject::New(Isolate* isolate, double value)
{
	jscshim::GlobalObject * global = jscshim::GetGlobalObjectForV8Isolate(isolate);
	return Local<Value>::New(JSC::constructNumber(global->v8ContextExec(), global, JSC::jsNumber(value)));
}

double NumberObject::ValueOf() const
{
	return GET_JSC_THIS()->internalValue().asNumber();
}

} // v8