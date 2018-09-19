/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"

#include <JavaScriptCore/JSCInlines.h>

#define GET_JSC_THIS() v8::jscshim::GetJscCellFromV8<JSC::Symbol>(this)

namespace v8
{

Local<Value> Symbol::Name() const
{
	// v8 users expect the exact name (JSC::Symbol::descriptiveString will return "Symbol(<name>)" format
	JSC::JSValue name(JSC::jsString(jscshim::GetCurrentExecState(), 
									WTF::String(GET_JSC_THIS()->privateName().uid())));
	return Local<Value>(name);
}

Local<Symbol> Symbol::New(Isolate * isolate, Local<String> name)
{
	JSC::Symbol * symbol = name.val_ ? JSC::Symbol::create(jscshim::GetExecStateForV8Isolate(isolate), JSC::asString(name.val_)) :
									   JSC::Symbol::create(jscshim::V8IsolateToJscShimIsolate(isolate)->VM());
	return Local<Symbol>(JSC::JSValue(symbol));
}

Local<Symbol> Symbol::For(Isolate * isolate, Local<String> name)
{
	JSC::VM& vm = jscshim::V8IsolateToJscShimIsolate(isolate)->VM();
	JSC::JSString * jscName = jscshim::GetJscCellFromV8<JSC::JSString>(*name);

	JSC::Symbol * symbol = JSC::Symbol::create(vm, vm.symbolRegistry().symbolForKey(jscName->tryGetValue()));
	return Local<Symbol>(JSC::JSValue(symbol));
}

Local<Symbol> Symbol::ForApi(Isolate * isolate, Local<String> name)
{
	jscshim::Isolate * shimIsolate = jscshim::V8IsolateToJscShimIsolate(isolate);
	JSC::JSString * jscName = jscshim::GetJscCellFromV8<JSC::JSString>(*name);

	JSC::Symbol * symbol = JSC::Symbol::create(shimIsolate->VM(), 
											   shimIsolate->ApiSymbolRegistry().symbolForKey(jscName->tryGetValue()));
	return Local<Symbol>(JSC::JSValue(symbol));
}

// Based on v8's implementation
#define WELL_KNOWN_SYMBOLS(V)               \
  V(HasInstance, hasInstance)               \
  V(IsConcatSpreadable, isConcatSpreadable) \
  V(Iterator, iterator)                     \
  V(Match, match)                           \
  V(Replace, replace)                       \
  V(Search, search)                         \
  V(Split, split)                           \
  V(ToPrimitive, toPrimitive)               \
  V(ToStringTag, toStringTag)               \
  V(Unscopables, unscopables)

#define SYMBOL_GETTER(Name, name) \
	Local<Symbol> Symbol::Get##Name(Isolate* isolate) \
	{ \
		JSC::VM& vm = jscshim::V8IsolateToJscShimIsolate(isolate)->VM(); \
		JSC::Symbol * symbol = JSC::Symbol::create(vm, static_cast<SymbolImpl&>(*vm.propertyNames->name##Symbol.impl())); \
		return Local<Symbol>(JSC::JSValue(symbol)); \
	}

WELL_KNOWN_SYMBOLS(SYMBOL_GETTER)

#undef SYMBOL_GETTER
#undef WELL_KNOWN_SYMBOLS

} // v8