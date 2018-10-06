/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include <JavaScriptCore/JSObject.h>
#include <JavaScriptCore/JSGlobalObject.h>

namespace v8 { namespace jscshim
{

class External final : public JSC::JSNonFinalObject {
private:
	void * m_value;

public:
	using Base = JSNonFinalObject;

	static External* create(JSC::VM& vm, JSC::ExecState * exec, JSC::Structure * structure, void * value)
	{
		External* object = new (NotNull, JSC::allocateCell<External>(vm.heap)) External(vm, structure, value);
		object->finishCreation(vm);
		return object;
	}

	DECLARE_INFO;

	static JSC::Structure * createStructure(JSC::VM& vm, JSC::JSGlobalObject * globalObject, JSC::JSValue prototype)
	{
		return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
	}

	void * value() const { return m_value; }

private:
	External(JSC::VM& vm, JSC::Structure* structure, void * value) : Base(vm, structure),
		m_value(value)
	{
	}
};

}} // v8::jscshim