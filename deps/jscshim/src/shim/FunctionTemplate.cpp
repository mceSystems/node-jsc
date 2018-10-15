/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "FunctionTemplate.h"
#include "Object.h"
#include "ObjectTemplate.h"

#include "Function.h"
#include "helpers.h"

#include <JavaScriptCore/ObjectConstructor.h>
#include <JavaScriptCore/JSCInlines.h>
#include <wtf/Assertions.h>

namespace v8 { namespace jscshim
{

// Note: Changed class name from "JSCShimFunction" to "Function" to appear like regular functions used directly as a function
const JSC::ClassInfo FunctionTemplate::s_info = { "Function", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(FunctionTemplate) };

Function * FunctionTemplate::makeNewFunctionInstance(JSC::ExecState * exec)
{
	jscshim::GlobalObject * global = GetGlobalObject(exec);
	JSC::VM& vm = global->vm();
	auto scope = DECLARE_THROW_SCOPE(vm);

	Function * newFunction = jscshim::Function::create(global->vm(), global->shimFunctionStructure(), this, m_originalName.get(), m_length);
	
	if (false == removePrototype())
	{
		// TODO: Could we lazy set the prototype if we don't have a prototype template\provider or a parent template?
		JSC::JSObject * newPrototype = instantiatePrototype(exec, newFunction);
		RETURN_IF_EXCEPTION(scope, nullptr);

		newFunction->putDirect(vm,
							   vm.propertyNames->prototype, 
							   newPrototype, 
							   readOnlyPrototype() ? (JSC::PropertyAttribute::DontEnum | JSC::PropertyAttribute::DontDelete | JSC::PropertyAttribute::ReadOnly) : 
													 (JSC::PropertyAttribute::DontEnum | JSC::PropertyAttribute::DontDelete));
	
		// Note: See ProbablyIsConstructor's documentation for more information about this
		if (m_flags & static_cast<unsigned char>(TemplateFlags::ProbablyIsConstructor))
		{
			// If we don't have a class name specified by the API user - use and cache the prototype's class name
			if (!m_templateClassName)
			{
				m_templateClassName.set(vm, this, JSC::jsOwnedString(exec, newPrototype->classInfo(vm)->className));
			}

			newFunction->putDirect(vm,
								   vm.propertyNames->displayName,
								   m_templateClassName.get(),
								   JSC::PropertyAttribute::DontEnum | JSC::PropertyAttribute::DontDelete | JSC::PropertyAttribute::ReadOnly);
		}
	}
	
	applyPropertiesToInstance(exec, newFunction);
	
	return newFunction;
}

bool FunctionTemplate::isTemplateFor(jscshim::Object * object)
{
	// Based on v8's FunctionTemplateInfo::IsTemplateFor
	FunctionTemplate * currentTemplate = object->objectTemplate()->constructorTemplate();
	while (currentTemplate)
	{
		if (this == currentTemplate)
		{
			return true;
		}

		currentTemplate = currentTemplate->parentTemplate();
	}

	return false;
}

bool FunctionTemplate::isTemplateFor(JSC::JSValue value)
{
	jscshim::Object * object = JSC::jsDynamicCast<jscshim::Object *>(*vm(), value);
	if (nullptr == object)
	{
		return false;
	}

	return isTemplateFor(object);
}

void FunctionTemplate::setCallHandler(v8::FunctionCallback callback, JSC::JSValue data)
{
	ensureNotInstantiated("v8::FunctionTemplate::SetCallHandler");

	m_callAsFunctionCallback = callback;
	m_callAsFunctionCallbackData.set(*vm(), this, data);
}

void FunctionTemplate::setLength(int length)
{
	ensureNotInstantiated("v8::FunctionTemplate::SetLength");
	m_length = length;
}

void FunctionTemplate::setTemplateClassName(JSC::VM& vm, JSC::JSString * className)
{
	ensureNotInstantiated("v8::FunctionTemplate::SetClassName");
	m_templateClassName.set(vm, this, className);
	m_flags |= static_cast<unsigned char>(TemplateFlags::ProbablyIsConstructor);
}

void FunctionTemplate::setParentTemplate(JSC::VM& vm, FunctionTemplate * parent)
{
	// v8 does this
	ensureNotInstantiated("v8::FunctionTemplate::Inherit");
	RELEASE_ASSERT(!m_prototypeProviderTemplate);

	m_parentTemplate.set(vm, this, parent);
}

void FunctionTemplate::setName(JSC::VM& vm, JSC::JSString * name)
{
	/* TODO: It seems right to do this check here, but in v8's ReceiverConversionForAccessors test,
	 * the same template is used once ... */
	// ensureNotInstantiated("FunctionTemplate::setName");
	
	m_originalName.set(vm, this, name);
	putDirect(vm, vm.propertyNames->name, name, PropertyAttribute::ReadOnly | PropertyAttribute::DontEnum);
}

/* TODO: v8 doesn't call EnsureNotInstantiated in FunctionTemplate::SetPrototypeProviderTemplate,
 *		 is it on purpose or just a bug? */
void FunctionTemplate::setPrototypeProviderTemplate(JSC::ExecState * exec, FunctionTemplate * prototypeProviderTemplate)
{
	// v8 does this
	RELEASE_ASSERT(!m_prototypeTemplate);
	RELEASE_ASSERT(!m_parentTemplate);

	m_prototypeProviderTemplate.set(exec->vm(), this, prototypeProviderTemplate);
}

/* TODO: v8 doesn't verify there's no prototype provider template here,
 *		 is it on purpose or just a bug? 
 * TODO: This is not thread safe */
ObjectTemplate * FunctionTemplate::getPrototypeTemplate(JSC::ExecState * exec)
{
	if (!m_prototypeTemplate)
	{
		JSC::VM& vm = exec->vm();

		JSC::Structure * templateStructure = ObjectTemplate::createStructure(vm, GetGlobalObject(exec), JSC::JSValue());
		m_prototypeTemplate.set(vm, this, ObjectTemplate::create(vm, templateStructure, exec, nullptr));
		m_flags |= static_cast<unsigned char>(TemplateFlags::ProbablyIsConstructor);
	}

	return m_prototypeTemplate.get();
}

// TODO: This is not thread safe
ObjectTemplate * FunctionTemplate::getInstnaceTemplate(JSC::ExecState * exec)
{
	if (!m_instanceTemplate)
	{
		JSC::VM& vm = exec->vm();

		JSC::Structure * templateStructure = ObjectTemplate::createStructure(vm, GetGlobalObject(exec), JSC::JSValue());
		m_instanceTemplate.set(vm, this, ObjectTemplate::create(vm, templateStructure, exec, this));
		m_flags |= static_cast<unsigned char>(TemplateFlags::ProbablyIsConstructor);
	}

	return m_instanceTemplate.get();
}

void FunctionTemplate::setRemovePrototype(bool value)
{
	ensureNotInstantiated("v8::FunctionTemplate::SetRemovePrototype");
	m_flags |= value ? static_cast<unsigned char>(TemplateFlags::RemovePrototype) : ~static_cast<unsigned char>(TemplateFlags::RemovePrototype);
	
	// In v8, functions without a "prototype" property can't be constructors 
	m_flags |= ~static_cast<unsigned char>(TemplateFlags::ProbablyIsConstructor);
}

void FunctionTemplate::setReadOnlyPrototype(bool value)
{
	ensureNotInstantiated("v8::FunctionTemplate::SetReadOnlyPrototype");
	m_flags |= value ? static_cast<unsigned char>(TemplateFlags::ReadOnlyPrototype) : ~static_cast<unsigned char>(TemplateFlags::ReadOnlyPrototype);
}

void FunctionTemplate::setHiddenPrototype(bool value)
{
	ensureNotInstantiated("v8::FunctionTemplate::SetHiddenPrototype");
	m_flags |= value ? static_cast<unsigned char>(TemplateFlags::HiddenPrototype) : ~static_cast<unsigned char>(TemplateFlags::HiddenPrototype); \
}

void FunctionTemplate::finishCreation(JSC::VM& vm, int length, JSC::JSValue data, FunctionTemplate * signature)
{
	Base::finishCreation(vm, this->classInfo()->className);

	m_callAsFunctionCallbackData.set(vm, this, data);
	m_signature.setMayBeNull(vm, this, signature);

	// Set "js" visible properties
	putDirect(vm, vm.propertyNames->length, JSC::jsNumber(length), ReadOnly | DontEnum);
}

void FunctionTemplate::visitChildren(JSC::JSCell* cell, JSC::SlotVisitor& visitor)
{
	Base::visitChildren(cell, visitor);

	FunctionTemplate * thisObject = JSC::jsCast<FunctionTemplate *>(cell);
	visitor.appendUnbarriered(thisObject->m_parentTemplate.get());
	visitor.appendUnbarriered(thisObject->m_prototypeProviderTemplate.get());
	visitor.appendUnbarriered(thisObject->m_prototypeTemplate.get());
	visitor.appendUnbarriered(thisObject->m_instanceTemplate.get());
	visitor.append(thisObject->m_templateClassName);
}

// TODO: Runtime checks instead of ASSERT?
JSC::EncodedJSValue JSC_HOST_CALL FunctionTemplate::functionCall(JSC::ExecState * exec)
{
	FunctionTemplate * instance = JSC::jsDynamicCast<FunctionTemplate *>(exec->vm(), exec->jsCallee());
	ASSERT(instance);

	return instance->handleFunctionCall(exec);
}

// Based on v8's GetCompatibleReceiver (builtins-api.cc)
JSC::JSValue FunctionTemplate::GetCompatibleReceiver(JSC::VM& vm, JSC::JSValue receiver, v8::jscshim::FunctionTemplate * signature)
{
	/* Valid receivers must we one of the following:
	 * - jscshim::Objects.
	 * - Object with a (direct) hidden, jscshim::Object, prototype.
	 */
	JSC::JSObject * receiverObject = receiver.getObject();
	if (nullptr == receiverObject)
	{
		return JSC::JSValue();
	}

	if (receiverObject->inherits(vm, jscshim::Object::info()))
	{
		// Check if the receiver itself matches the signature
		if (signature->isTemplateFor(static_cast<jscshim::Object *>(receiverObject)))
		{
			return receiver;
		}
	}

	// TODO: Call receiverObject->getPrototype? this will mainly effect Proxies
	jscshim::Object * prototype = JSC::jsDynamicCast<jscshim::Object *>(vm, receiverObject->getPrototypeDirect(vm));

	// Check if one of the receiver's hidden prototypes match the signature
	while (prototype && prototype->shouldBeHiddenAsPrototype())
	{
		if (signature->isTemplateFor(prototype))
		{
			return prototype;
		}

		// TODO: Call prototype->getPrototype? this will mainly effect Proxies
		prototype = JSC::jsDynamicCast<jscshim::Object *>(vm, receiverObject->getPrototypeDirect(vm));
	}

	return JSC::JSValue();
}

/* We handle regular function calls here since it possible to call them directly (when setting accessors on a Template)
 * TODO: Should we call Isolate::HandleThrownException here? If we're being called from JS\API, 
 *       then v8::Script::Run or the API call will forward the exception to Isolate::HandleThrownException, So it's probably 
 *       not necessary. */
JSC::EncodedJSValue FunctionTemplate::handleFunctionCall(JSC::ExecState * exec)
{
	JSC::JSValue thisValue = exec->thisValue().toThis(exec, jscshim::GetECMAMode(exec));

	JSC::JSValue holder;
	if (m_signature)
	{
		holder = GetCompatibleReceiver(exec->vm(), thisValue, m_signature.get());
		if (!holder)
		{
			// v8's HandleApiCallHelper will throw a type error. The error message was also taken from v8 (messages.h)
			auto scope = DECLARE_THROW_SCOPE(exec->vm());
			return JSC::JSValue::encode(JSC::throwTypeError(exec, scope, "Illegal invocation"_s));
		}
	}
	else
	{
		holder = thisValue;
	}

	JSC::JSValue returnValue = forwardCallToCallback(exec, thisValue, holder, false);

	return JSC::JSValue::encode(returnValue ? returnValue : JSC::jsUndefined());
}

JSC::JSObject * FunctionTemplate::instantiatePrototype(JSC::ExecState * exec, jscshim::Function * constructor)
{
	auto scope = DECLARE_THROW_SCOPE(exec->vm());
	jscshim::GlobalObject * global = GetGlobalObject(exec);
	JSC::VM& vm = global->vm();

	// The logic here follows v8's InstantiateFunction (api-natives.cc) in general
	JSC::JSObject * newPrototype = nullptr;
	if (m_prototypeTemplate)
	{
		newPrototype = m_prototypeTemplate.get()->makeNewObjectInstance(exec, JSC::JSValue());
	}
	else if (m_prototypeProviderTemplate)
	{
		newPrototype = m_prototypeProviderTemplate->makeNewFunctionInstance(exec)->getDirect(vm, vm.propertyNames->prototype).getObject();
		ASSERT(newPrototype);
	}
	else
	{
		newPrototype = JSC::constructEmptyObject(exec);
	}
	RETURN_IF_EXCEPTION(scope, nullptr);

	// inherit from parent template (if we have one), following v8's\chakrashimm's flow
	if (m_parentTemplate)
	{
		// TODO: Pass the prototype when creating newPrototype (make makeNewObjectInstance get the prototype)?
		JSC::JSValue parentPrototype = m_parentTemplate->makeNewFunctionInstance(exec)->getDirect(vm, vm.propertyNames->prototype);
		RETURN_IF_EXCEPTION(scope, nullptr);

		newPrototype->setPrototypeDirect(vm, parentPrototype);
		RETURN_IF_EXCEPTION(scope, nullptr);
	}

	// Set the prototype's "constructor" property
	newPrototype->putDirect(vm, vm.propertyNames->constructor, constructor, DontEnum);

	newPrototype->didBecomePrototype();
	return newPrototype;
}

}} // v8::jscshim
