/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "APIAccessor.h"

#include "FunctionTemplate.h"
#include "helpers.h"

#include <CatchScope.h>
#include <JavaScriptCore/JSCInlines.h>

namespace v8 { namespace jscshim
{

const JSC::ClassInfo APIAccessor::s_info = { "JSCShimAPIAccessor", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(APIAccessor) };

void APIAccessor::finishCreation(JSC::VM& vm, JSC::JSCell * name, JSC::JSValue data, FunctionTemplate * signature)
{
	Base::finishCreation(vm);

	m_name.set(vm, this, name);
	m_data.set(vm, this, data);
	m_signature.setMayBeNull(vm, this, signature);
}
	
void APIAccessor::visitChildren(JSC::JSCell * cell, JSC::SlotVisitor& visitor)
{
	Base::visitChildren(cell, visitor);

	APIAccessor * thisObject = JSC::jsCast<APIAccessor *>(cell);
	visitor.append(thisObject->m_name);
	visitor.append(thisObject->m_data);
	visitor.append(thisObject->m_signature);
}

/* TODO: Should we call Isolate::HandleThrownException in getter\setter? If we're being called from JS, 
 *       then v8::Script::Run will forward the exception to Isolate::HandleThrownException. Same for being 
 *       called from an API call. So it's probably not necessary. */

JSC::JSValue APIAccessor::getter(Base				* thisApiValue,
								 JSC::ExecState		* exec,
								 JSC::JSObject		* slotBase, 
								 JSC::JSValue		receiver, 
								 JSC::PropertyName	propertyName)
{
	APIAccessor * accessorInstance = JSC::jsDynamicCast<APIAccessor *>(exec->vm(), thisApiValue);
	ASSERT(accessorInstance);

	// Get "this" value and verify it with our signature (if needed)
	JSC::JSValue thisValue = receiver.toThis(exec, jscshim::GetECMAMode(exec));
	if (accessorInstance->m_signature)
	{
		if (false == accessorInstance->m_signature->isTemplateFor(thisValue))
		{
			return accessorInstance->throwIncompatibleMethodReceiverError(exec, thisValue);
		}
	}

	if (!accessorInstance->m_getterCallback)
	{
		return JSC::jsUndefined();
	}
	
	// Build the PropertyCallbackInfo for the callback
	JSC::JSValue returnValue = JSC::jsUndefined();
	v8::Isolate * isolate = reinterpret_cast<v8::Isolate *>(jscshim::GetGlobalObject(exec)->isolate());
	v8::PropertyCallbackInfo<v8::Value> callbackInfo(Local<v8::Object>(thisValue),
													 Local<v8::Object>(JSC::JSValue(slotBase)),
													 Local<Value>(accessorInstance->m_data.get()),
													 isolate,
													 &returnValue,
													 false); // Always false for getters according to v8's docs

	// Call the user's getter
	Local<Name> property(accessorInstance->m_name.get());
	accessorInstance->m_getterCallback(property, callbackInfo);

	return returnValue;
}

bool APIAccessor::setter(Base				* thisApiValue,
						 JSC::ExecState		* exec,
						 JSC::JSObject		* slotBase, 
						 JSC::JSValue		receiver, 
						 JSC::PropertyName	propertyName, 
						 JSC::JSValue		value,
						 bool				isStrictMode)
{
	APIAccessor * accessorInstance = JSC::jsDynamicCast<APIAccessor *>(exec->vm(), thisApiValue);
	ASSERT(accessorInstance);

	// Get "this" value and verify it with our signature (if needed)
	JSC::JSValue thisValue = receiver.toThis(exec, jscshim::GetECMAMode(exec));
	if (accessorInstance->m_signature)
	{
		if (false == accessorInstance->m_signature->isTemplateFor(thisValue))
		{
			accessorInstance->throwIncompatibleMethodReceiverError(exec, thisValue);
			return false;
		}
	}

	if (!accessorInstance->m_setterCallback)
	{
		return true;
	}
	
	/* Build the PropertyCallbackInfo for the callback.
	 * Note that according to v8's docs, setters should "throw on error" in strict mode. */
	JSC::JSValue returnValue = JSC::jsUndefined();
	v8::Isolate * isolate = reinterpret_cast<v8::Isolate *>(jscshim::GetGlobalObject(exec)->isolate());
	v8::PropertyCallbackInfo<void> callbackInfo(Local<v8::Object>(thisValue),
												Local<v8::Object>(JSC::JSValue(slotBase)),
												Local<Value>(accessorInstance->m_data.get()),
												isolate,
												&returnValue,
												isStrictMode);

	
	auto scope = DECLARE_CATCH_SCOPE(exec->vm());
	Local<Name> property(accessorInstance->m_name.get());
	accessorInstance->m_setterCallback(property, Local<Value>(value), callbackInfo);
	ASSERT(returnValue.isUndefined());

	return !scope.exception();
}

bool APIAccessor::reconfigureToDataPropertySetter(Base				* thisApiValue,
												  JSC::ExecState	* exec,
												  JSC::JSObject		* slotBase, 
												  JSC::JSValue		receiver, 
												  JSC::PropertyName propertyName, 
												  JSC::JSValue		value,
												  bool				isStrictMode)
{
	// TODO: attributes?
	return slotBase->putDirect(exec->vm(), propertyName, value);
}

JSC::JSValue APIAccessor::throwIncompatibleMethodReceiverError(JSC::ExecState * exec, const JSC::JSValue& receiver)
{
	JSC::VM& vm = exec->vm();

	/* v8's Object::GetPropertyWithAccessor\SetPropertyWithAccessor will throw a type error.
	 * The error message was also taken from v8 (kIncompatibleMethodReceiver in messages.h) */
	auto scope = DECLARE_THROW_SCOPE(exec->vm());

	WTF::String accessorName = JSC::JSValue(m_name.get()).toWTFString(exec);
	WTF::String receiverName = receiver.toWTFString(exec);
	WTF::String errorMessage = WTF::String::format("Method %s called on incompatible receiver %s", 
												   accessorName.ascii().data(),
												   receiverName.ascii().data());
	
	return JSC::throwTypeError(exec, scope, errorMessage);
}


}} // v8::jscshim