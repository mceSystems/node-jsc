/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"
#include "shim/helpers.h"

#include <JavaScriptCore/BooleanObject.h>
#include <JavaScriptCore/JSCInlines.h>

#define GET_JSC_THIS() v8::jscshim::GetJscCellFromV8<JSC::BooleanObject>(this)

namespace v8
{

Local<Value> BooleanObject::New(Isolate* isolate, bool value)
{
	jscshim::GlobalObject * global = jscshim::GetGlobalObjectForV8Isolate(isolate);
	JSC::VM& vm = global->vm();

	JSC:: BooleanObject * obj = JSC::BooleanObject::create(vm, global->booleanObjectStructure());
	obj->setInternalValue(vm, JSC::jsBoolean(value));

	return Local<Value>::New(JSC::JSValue(obj));
}

bool BooleanObject::ValueOf() const
{
	return GET_JSC_THIS()->internalValue().asBoolean();
}

} // v8