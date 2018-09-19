/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/External.h"
#include "shim/helpers.h"

#include <JavaScriptCore/JSCInlines.h>

namespace v8
{

Local<External> External::New(Isolate * isolate, void * value)
{
	JSC::ExecState * exec = jscshim::GetExecStateForV8Isolate(isolate);

	JSC::JSValue external(jscshim::External::create(exec->vm(), 
													exec, 
													jscshim::GetGlobalObject(exec)->shimExternalStructure(), 
													value));
	return Local<External>(external);
}

void * External::Value() const
{
	return v8::jscshim::GetJscCellFromV8<jscshim::External>(this)->value();
}

} // v8