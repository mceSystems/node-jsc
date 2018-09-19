/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include "v8.h"
#include "FunctionTemplate.h"

#include <JavaScriptCore/InternalFunction.h>
#include <JavaScriptCore/ObjectPrototype.h>

namespace v8 { namespace jscshim
{

class Function : public JSC::InternalFunction
{
private:
	JSC::WriteBarrier<FunctionTemplate> m_functionTemplate;
	JSC::WriteBarrier<JSC::Structure> m_objectsStructure;

public:
	typedef InternalFunction Base;

	template<typename CellType>
	static JSC::IsoSubspace * subspaceFor(JSC::VM& vm)
	{
		jscshim::Isolate * currentIsolate = jscshim::Isolate::GetCurrent();
		RELEASE_ASSERT(&currentIsolate->VM() == &vm);

		return currentIsolate->FunctionSpace();
	}

	static Function* create(JSC::VM& vm, JSC::Structure* structure, FunctionTemplate * functionTemplate, JSC::JSString * name, int length)
	{
		/* In v8, functions without the "prototype" property can't be a constructor (v8::Object::IsConstructor
		 * should return false for these functions). Passing callHostFunctionAsConstructor will cause our getConstructData
	     * (Inherited from InternalFunction) to return ConstructType::None. */
		JSC::NativeFunction constructorFunction = functionTemplate->removePrototype() ? JSC::callHostFunctionAsConstructor : constructorCall;

		Function* cell = new (NotNull, JSC::allocateCell<Function>(vm.heap)) Function(vm, structure, constructorFunction);
		cell->finishCreation(vm, length, functionTemplate, name);
		return cell;
	}

	DECLARE_INFO;

	static JSC::Structure * createStructure(JSC::VM& vm, JSC::JSGlobalObject * globalObject, JSC::JSValue prototype)
	{
		return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::InternalFunctionType, StructureFlags), info());
	}

	FunctionTemplate * functionTemplate() const { return m_functionTemplate.get(); }

	JSC::Structure * structureForObjects() const { return m_objectsStructure.get(); };
	void setStructureForObjects(JSC::VM& vm, JSC::Structure * structure) { m_objectsStructure.set(vm, this, structure); }
	void setName(JSC::VM& vm, JSC::JSString * name);

private:
	Function(JSC::VM& vm, JSC::Structure* structure, JSC::NativeFunction constructorFunction) : Base(vm, structure, functionCall, constructorFunction)
	{
	}

	static JSC::EncodedJSValue JSC_HOST_CALL functionCall(JSC::ExecState * exec);
	static JSC::EncodedJSValue JSC_HOST_CALL constructorCall(JSC::ExecState * exec);

	void finishCreation(JSC::VM& vm, int length, FunctionTemplate * functionTemplate, JSC::JSString * name);

	static void visitChildren(JSC::JSCell*, JSC::SlotVisitor&);
};

}} // v8::jscshim