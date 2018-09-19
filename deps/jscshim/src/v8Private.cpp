/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"

#include <JavaScriptCore/JSCInlines.h>
#include "wtf/text/SymbolRegistry.h"

#define GET_JSC_THIS() v8::jscshim::GetJscCellFromV8<JSC::Symbol>(this)

namespace v8
{

Local<Value> Private::Name() const
{
	// v8 users expect the exact name (JSC::Symbol::descriptiveString will return "Symbol(<name>)" format
	JSC::JSValue name(JSC::jsString(jscshim::GetCurrentExecState(), 
									WTF::String(GET_JSC_THIS()->privateName().uid())));
	return Local<Value>(name);
}

Local<Private> Private::New(Isolate* isolate, Local<String> name)
{
	jscshim::Isolate * shimIsolate = jscshim::V8IsolateToJscShimIsolate(isolate);
	JSC::JSString * jscName = jscshim::GetJscCellFromV8<JSC::JSString>(*name);

	JSC::Symbol * privateSymbol = nullptr;

	if (jscName)
	{
		// Note: The const_cast is ugly, but it seems that PrivateSymbolImpl::create won't change it
		WTF::StringImpl * nameImpl = const_cast<WTF::StringImpl *>(jscName->tryGetValueImpl());
		privateSymbol = JSC::Symbol::create(shimIsolate->VM(), 
											WTF::PrivateSymbolImpl::create(*nameImpl));
	}
	else
	{
		privateSymbol = JSC::Symbol::create(shimIsolate->VM(), WTF::PrivateSymbolImpl::createNullSymbol());
	}

	return Local<Private>(JSC::JSValue(privateSymbol));
}

Local<Private> Private::ForApi(Isolate* isolate, Local<String> name)
{
	jscshim::Isolate * shimIsolate = jscshim::V8IsolateToJscShimIsolate(isolate);
	JSC::JSString * jscName = jscshim::GetJscCellFromV8<JSC::JSString>(*name);

	JSC::Symbol * privateSymbol = JSC::Symbol::create(shimIsolate->VM(), 
													  shimIsolate->ApiPrivateSymbolRegistry().symbolForKey(jscName->tryGetValue()));

	return Local<Private>(JSC::JSValue(privateSymbol));
}

} // v8