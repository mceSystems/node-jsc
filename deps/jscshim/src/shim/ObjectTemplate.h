/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include "v8.h"
#include "Template.h"
#include "InterceptorInfo.h"

#include <memory>

namespace v8 { namespace jscshim
{
class FunctionTemplate;
class Object;

class ObjectTemplate : public Template {
private:
	friend class Object;
	friend class ObjectWithInterceptors;

	int m_internalFieldCount;
	std::unique_ptr<NamedInterceptorInfo> m_namedInterceptors;
	std::unique_ptr<IndexedInterceptorInfo> m_indexedInterceptors;
	JSC::WriteBarrier<JSC::Structure> m_objectStructure;
	JSC::WriteBarrier<FunctionTemplate> m_constructor;

public:
	typedef Template Base;

	template<typename CellType>
	static JSC::IsoSubspace * subspaceFor(JSC::VM& vm)
	{
		jscshim::Isolate * currentIsolate = jscshim::Isolate::GetCurrent();
		RELEASE_ASSERT(&currentIsolate->VM() == &vm);

		return currentIsolate->ObjectTemplateSpace();
	}

	static ObjectTemplate* create(JSC::VM& vm, JSC::Structure * structure, JSC::ExecState * exec, FunctionTemplate * constructor)
	{
		GlobalObject * global = GetGlobalObject(exec);

		ObjectTemplate* newTemplate = new (NotNull, JSC::allocateCell<ObjectTemplate>(vm.heap)) ObjectTemplate(vm, structure, global->isolate());
		newTemplate->finishCreation(vm, constructor);
		return newTemplate;
	}

	DECLARE_INFO;

	static JSC::Structure * createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
	{
		return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::InternalFunctionType, StructureFlags), info());
	}

	jscshim::Object * makeNewObjectInstance(JSC::ExecState * exec, JSC::JSValue newTarget, Function * constructorInstance = nullptr);

	int internalFieldCount() const;

	void setInternalFieldCount(int newCount);

	void setInterceptors(const NamedPropertyHandlerConfiguration& configuration);
	void setInterceptors(const IndexedPropertyHandlerConfiguration& configuration);

	void setCallAsFunctionHandler(v8::FunctionCallback callback, JSC::JSValue data);

	jscshim::FunctionTemplate * constructorTemplate() const { return m_constructor.get(); }

private:
	// TODO: Function pointers
	ObjectTemplate(JSC::VM& vm, JSC::Structure * structure, Isolate * isolate) : Base(vm, structure, isolate, DummyCallback, DummyCallback),
		m_internalFieldCount(0)
	{
	}

	static JSC_HOST_CALL JSC::EncodedJSValue DummyCallback(JSC::ExecState * exec);

	void finishCreation(JSC::VM& vm, FunctionTemplate * constructor);
	static void visitChildren(JSC::JSCell*, JSC::SlotVisitor&);

	static JSC::EncodedJSValue JSC_HOST_CALL instanceFunctionCall(JSC::ExecState * exec);
	static JSC::EncodedJSValue JSC_HOST_CALL instanceConstructorCall(JSC::ExecState * exec);

	static inline JSC::EncodedJSValue JSC_HOST_CALL handleInstanceCall(JSC::ExecState * exec, bool isConstructor);

	JSC::Structure * createObjectStructure(jscshim::GlobalObject * global, bool withInterceptors, JSC::JSValue prototype);
	JSC::Structure * getDefaultObjectStructure(jscshim::GlobalObject * global, bool withInterceptors);

	JSC::Structure * getStructrueForNewInstance(JSC::ExecState * exec, JSC::JSValue newTarget, bool haveInterceptors, Function * constructorInstance);

	void applyPropertiesIncludingParentsToInstance(JSC::ExecState * exec, JSC::JSObject * instance);
};

}} // v8::jscshim