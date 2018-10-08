/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include "v8.h"

#include <JavaScriptCore/JSObject.h>

namespace v8 { namespace jscshim
{

class CallSitePrototype final : public JSC::JSNonFinalObject {
public:
	typedef JSNonFinalObject Base;

	static CallSitePrototype * create(JSC::VM& vm, JSC::Structure * structure, JSC::JSGlobalObject * globalObject)
	{
		CallSitePrototype * callSitePrototype = new (NotNull, JSC::allocateCell<CallSitePrototype>(vm.heap)) CallSitePrototype(vm, structure);
		callSitePrototype->finishCreation(vm, globalObject);
		return callSitePrototype;
	}

	DECLARE_INFO;

	static JSC::Structure * createStructure(JSC::VM& vm, JSC::JSGlobalObject * globalObject, JSC::JSValue prototype)
	{
		return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
	}

private:
	CallSitePrototype(JSC::VM& vm, JSC::Structure * structure) : Base(vm, structure)
	{
	}

	void finishCreation(JSC::VM& vm, JSC::JSGlobalObject * globalObject);
};

}} // v8::jscshim