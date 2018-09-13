/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "Function.h"
#include "Object.h"

#include <JavaScriptCore/SlotVisitorInlines.h>
#include <JavaScriptCore/JSCInlines.h>
#include <wtf/text/WTFString.h>

namespace v8 { namespace jscshim
{

/* Note: Changed class name from "JSCShimFunction" to "Function" to appear like regular functions when converted to a string
 * (as part of an exception message, for example */
const JSC::ClassInfo Function::s_info = { "Function", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(Function) };

void Function::finishCreation(JSC::VM& vm, int length, FunctionTemplate * functionTemplate, JSC::JSString * name)
{
	/* TODO: InternlFunction's finishCreation will create a new JSString for the name, which is a waste.
	 * Should we add a new InternlFunction::finishCreation variation which accepts an existing JSString? */
	Base::finishCreation(vm, name->tryGetValue());

	m_functionTemplate.set(vm, this, functionTemplate);

	// Set "js" visible properties, except the "prototype" property which is set by our FunctionTemplate
	putDirect(vm, vm.propertyNames->length, JSC::jsNumber(length), ReadOnly | DontEnum);
}

void Function::visitChildren(JSC::JSCell* cell, JSC::SlotVisitor& visitor)
{
	Base::visitChildren(cell, visitor);

	Function * thisObject = JSC::jsCast<Function *>(cell);
	visitor.append(thisObject->m_functionTemplate);
	visitor.append(thisObject->m_objectsStructure);
}

void Function::setName(JSC::VM& vm, JSC::JSString * name)
{
	m_originalName.set(vm, this, name);
	putDirect(vm, vm.propertyNames->name, name, PropertyAttribute::ReadOnly | PropertyAttribute::DontEnum);
}

JSC::EncodedJSValue JSC_HOST_CALL Function::functionCall(JSC::ExecState * exec)
{
	Function * instance = JSC::jsDynamicCast<Function *>(exec->vm(), exec->jsCallee());
	ASSERT(instance);

	/* We let function templates handle regular function calls since it possible to call them
	 * directly (when setting accessors on a Template) */
	return instance->functionTemplate()->handleFunctionCall(exec);
}

JSC::EncodedJSValue JSC_HOST_CALL Function::constructorCall(JSC::ExecState * exec)
{
	Function * instance = JSC::jsDynamicCast<Function *>(exec->vm(), exec->jsCallee());
	ASSERT(instance);

	FunctionTemplate * functionTemplate = instance->m_functionTemplate.get();

	/* Create a new object using our instance template (getInstnaceTemplate
	 * will create one if needed) */
	jscshim::ObjectTemplate * instanceTemplate = functionTemplate->getInstnaceTemplate(exec);

	// Create our new instance object
	JSC::JSObject * newInstance = instanceTemplate->makeNewObjectInstance(exec, exec->newTarget(), instance);

	// If we don't have a callback (which is possible when creating a FunctionTemplate) - just return our new instance
	if (nullptr == functionTemplate->m_callAsFunctionCallback)
	{
		return JSC::JSValue::encode(newInstance);
	}

	JSC::JSValue thisValue(newInstance);
	JSC::JSValue returnValue = functionTemplate->forwardCallToCallback(exec, 
																	   thisValue, 
																	   thisValue, 
																	   true);
	
	// This is what v8's HandleApiCallHelper (builtins-api.cc) basically does
	if (returnValue.isEmpty() || (false == returnValue.isObject()))
	{
		returnValue = newInstance;
	}

	return JSC::JSValue::encode(returnValue);
}

}} // v8::jscshim