/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include <v8.h>
#include "ObjectTemplate.h"
#include "FunctionTemplate.h"
#include "EmbeddedFieldsContainer.h"

#include <JavaScriptCore/JSDestructibleObject.h>
#include <wtf/Vector.h>

namespace v8 { namespace jscshim
{

class ObjectTemplate;

class Object : public JSC::JSDestructibleObject {
private:
	JSC::WriteBarrier<ObjectTemplate> m_template;
	EmbeddedFieldsContainer<false> m_internalFields;

public:
	typedef JSDestructibleObject Base;

	static Object* create(JSC::VM& vm, JSC::Structure* structure, ObjectTemplate * objectTemplate)
	{
		Object* object = new (NotNull, JSC::allocateCell<Object>(vm.heap)) Object(vm, structure, objectTemplate->m_internalFieldCount);
		object->finishCreation(vm, objectTemplate);
		return object;
	}

	DECLARE_INFO;

	static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype, bool isCallable)
	{
		unsigned int flags = isCallable ? (StructureFlags | JSC::OverridesGetCallData ) : StructureFlags;
		return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, flags), info());
	}

	static JSC::CallType getCallData(JSC::JSCell * cell, JSC::CallData& callData);
	static JSC::ConstructType getConstructData(JSC::JSCell * cell, JSC::ConstructData& constructData);

	ObjectTemplate * objectTemplate() const { return m_template.get(); }
	EmbeddedFieldsContainer<false>& internalFields() { return m_internalFields; }

	jscshim::Function * getConstructor(JSC::VM& vm) const;

	bool shouldBeHiddenAsPrototype()
	{
		FunctionTemplate * constructorTemplate = m_template->constructorTemplate();
		return constructorTemplate && constructorTemplate->hiddenPrototype();
	}

protected:
	Object(JSC::VM& vm, JSC::Structure* structure, int internalFieldCount) : Base(vm, structure),
		m_internalFields(internalFieldCount)
	{
	}

	void finishCreation(JSC::VM& vm, ObjectTemplate * objectTemplate);

	static void visitChildren(JSC::JSCell*, JSC::SlotVisitor&);

	static WTF::String className(const JSObject * object);
};

}} // v8::jscshim