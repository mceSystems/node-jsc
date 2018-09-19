/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"

#include <JavaScriptCore/SymbolObject.h>
#include <JavaScriptCore/JSCInlines.h>

#define GET_JSC_THIS() v8::jscshim::GetJscCellFromV8<JSC::SymbolObject>(this)

namespace v8
{

Local<Value> SymbolObject::New(Isolate * isolate, Local<Symbol> value)
{
	jscshim::Isolate * shimIsolate = jscshim::V8IsolateToJscShimIsolate(isolate);
	jscshim::GlobalObject * global = shimIsolate->GetCurrentContext();

	JSC::Symbol * symbol = JSC::asSymbol(value.val_);
	JSC::JSObject * newSymbolObject = JSC::SymbolObject::create(shimIsolate->VM(), global->symbolObjectStructure(), symbol);

	return Local<Value>::New(JSC::JSValue(newSymbolObject));
}

Local<Symbol> SymbolObject::ValueOf() const
{
	return Local<Symbol>::New(JSC::JSValue(GET_JSC_THIS()->internalValue()));
}

} // v8