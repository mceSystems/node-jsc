/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"
#include "shim/helpers.h"

#include <JavaScriptCore/DateInstance.h>
#include <JavaScriptCore/JSCInlines.h>

#define GET_JSC_THIS() v8::jscshim::GetJscCellFromV8<JSC::DateInstance>(this)

namespace v8
{

MaybeLocal<Value> Date::New(Local<Context> context, double time)
{
	JSC::MarkedArgumentBuffer argList;
	argList.append(JSC::jsNumber(time));

	jscshim::GlobalObject * global = jscshim::GetGlobalObjectForV8Context(*context);

	JSC::JSObject * newDate = JSC::DateInstance::create(global->vm(), global->dateStructure(), time);
	return Local<Value>::New(JSC::JSValue(newDate));
}

double Date::ValueOf() const
{
	return GET_JSC_THIS()->internalNumber();
}

} // v8