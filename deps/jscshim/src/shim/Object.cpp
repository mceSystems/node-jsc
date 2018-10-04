/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "Object.h"
#include "ObjectTemplate.h"
#include "Function.h"

#include "helpers.h"

#include <JavaScriptCore/JSObject.h>
#include <JavaScriptCore/CatchScope.h>
#include <JavaScriptCore/JSCInlines.h>

namespace v8 { namespace jscshim
{

/* Note: Changed class name from "JSCShimObject" to "Object" to appear like regular objects when converted to a string
 * (as part of an exception message, for example */
const JSC::ClassInfo Object::s_info = { "Object", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(Object) };

void Object::finishCreation(JSC::VM& vm, ObjectTemplate * objectTemplate)
{
	Base::finishCreation(vm);

	m_template.set(vm, this, objectTemplate);
}

void Object::visitChildren(JSC::JSCell * cell, JSC::SlotVisitor& visitor)
{
	Base::visitChildren(cell, visitor);

	Object * thisObject = JSC::jsCast<Object *>(cell);
	visitor.append(thisObject->m_template);
}

JSC::CallType Object::getCallData(JSC::JSCell * cell, JSC::CallData& callData)
{
	Object * object = JSC::jsCast<Object*>(cell);
	if (object->m_template->m_callAsFunctionCallback)
	{
		callData.native.function = ObjectTemplate::instanceFunctionCall;
		return JSC::CallType::Host;
	}

	return JSC::CallType::None;
}

JSC::ConstructType Object::getConstructData(JSC::JSCell * cell, JSC::ConstructData& constructData)
{
	Object * object = JSC::jsCast<Object*>(cell);
	if (object->m_template->m_callAsFunctionCallback)
	{
		constructData.native.function = ObjectTemplate::instanceConstructorCall;
		return JSC::ConstructType::Host;
	}

	return JSC::ConstructType::None;
}

// Based on JSC's JSObject::calculatedClassName (which we can't use since it calls className)
WTF::String Object::className(const JSC::JSObject * object, JSC::VM& vm)
{
	const Object * shimObject = JSC::jsDynamicCast<const Object *>(vm, object);
	ASSERT(shimObject);

	Function * constructorFunction = shimObject->getConstructor(vm);
	if (constructorFunction)
	{
		return constructorFunction->calculatedDisplayName(vm);
	}

	return "Object"_s;
}

jscshim::Function * Object::getConstructor(JSC::VM& vm) const
{
	return JSC::jsDynamicCast<jscshim::Function *>(vm, GetObjectConstructor(vm, this));
}

}} // v8::jscshim