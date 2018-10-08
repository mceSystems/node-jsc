/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "Template.h"
#include "TemplateProperty.h"
#include "FunctionTemplate.h"
#include "Function.h"
#include "Object.h"
#include "helpers.h"

#include <JavaScriptCore/BatchedTransitionOptimizer.h>
#include <JavaScriptCore/JSCInlines.h>

namespace v8 { namespace jscshim
{

const JSC::ClassInfo Template::s_info = { "JSCShimTemplate", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(Template) };

void Template::SetProperty(JSC::ExecState * exec, JSC::JSCell * name, JSC::JSValue value, int attributes)
{
	JSC::VM& vm = exec->vm();

	/* If this value is a FunctionTemplate, and we have a string name - update it's name property
	 * TODO: Handle Function instances?
	 * TODO: Handle symbol names? */
	if (name->isString())
	{
		jscshim::FunctionTemplate * asFunctionTemplate = JSC::jsDynamicCast<jscshim::FunctionTemplate *>(vm, value);
		if (asFunctionTemplate)
		{
			asFunctionTemplate->setName(vm, JSC::jsCast<JSC::JSString *>(name));
		}
	}

	m_properties.append(std::make_unique<TemplateValueProperty>(exec, this, name, attributes, value));
}

void Template::SetAccessorProperty(JSC::ExecState			 * exec,
								   JSC::JSCell				 * name,
								   jscshim::FunctionTemplate * getter,
								   jscshim::FunctionTemplate * setter,
								   int						 attributes)
{
	m_properties.append(std::make_unique<TemplateAccessorProperty>(exec,
																   this,
																   name,
																   attributes,
																   getter,
																   setter));
}

void Template::SetAccessor(JSC::ExecState				  * exec,
						   JSC::JSCell					  * name,
						   v8::AccessorNameGetterCallback getter,
						   v8::AccessorNameSetterCallback setter,
						   JSC::JSValue					  data,
						   jscshim::FunctionTemplate	  * signatureReceiver,
						   int							  attributes,
						   bool							  isSpecialDataProperty)
{
	m_properties.append(std::make_unique<TemplateAccessor>(exec,
														   this,
														   name,
														   attributes,
														   getter,
														   setter,
														   data,
														   signatureReceiver,
														   isSpecialDataProperty));
}

void Template::applyPropertiesToInstance(JSC::ExecState * exec, JSC::JSObject * instance)
{
	JSC::VM& vm = *this->vm();

	// Note: BatchedTransitionOptimize use was taken from reifyStaticProperties in JSC's Lookup.h
	JSC::BatchedTransitionOptimizer transitionOptimizer(vm, instance);
	for (auto& property : m_properties)
	{
		// TODO: Fix terminology mismatch (isHiddenPrototype/m_forceFunctionInstantiation
		JSC::JSValue value = property->instantiate(vm, exec, m_forceFunctionInstantiation);
		const JSC::PropertyName& propertyName = property->propertyName();
		unsigned int attributes = property->attributes();

		// Note that we're not using putDirectMayBeIndex since it doesn't accept property attributes
		if (property->attributes() & JSC::PropertyAttribute::Accessor)
		{
			instance->putDirectAccessor(exec, propertyName, JSC::jsCast<JSC::GetterSetter *>(value), attributes);
		}
		else if (std::optional<uint32_t> index = JSC::parseIndex(propertyName))
		{
			instance->putDirectIndex(exec, 
									 index.value(), 
									 value, 
									 attributes, 
									 JSC::PutDirectIndexLikePutDirect);
		}
		else if (value.isCustomAPIValue())
		{
			instance->putDirectCustomAPIValue(vm, propertyName, static_cast<JSC::CustomAPIValue *>(value.asCell()), attributes);
		}
		else
		{
			instance->putDirect(vm, propertyName, value, attributes);
		}
	}

	m_instantiated = true;
}

JSC::JSValue Template::forwardCallToCallback(JSC::ExecState		  * exec,
											 const JSC::JSValue&  thisValue, 
											 const JSC::JSValue&  holder, 
											 bool				  isConstructorCall)
{
	if (nullptr == m_callAsFunctionCallback)
	{
		/* It seems that in v8, HandleApiCallHelper (builtins-api.cc) will return the js receiver if 
		 * the function has no "call code".
		 * If thisValue is the global object, it seems that it is the actual global object rather than it's proxy.
		 * Should we really check for this and return the proxy? according to v8's EmptyApiCallback test, it should
		 * be the proxy. For now we'll do it only here, but we might need to add this check in other places.
		 */
		if (thisValue.isCell() && thisValue.asCell() == exec->lexicalGlobalObject())
		{
			return static_cast<JSC::JSGlobalObject *>(thisValue.asCell())->globalThis();
		}

		return thisValue;
	}

	// See the documentation of FunctionCallbackInfo in our v8.h for more info on accessing arguments
	JSC::JSValue * arguments = reinterpret_cast<JSC::JSValue *>(exec + exec->argumentOffset(0));

	// This is what v8's Builtins::InvokeApiFunction does
	JSC::JSValue newTarget = isConstructorCall ? exec->newTarget() : JSC::jsUndefined();

	JSC::JSValue returnValue;
	v8::FunctionCallbackInfo<v8::Value> callbackInfo(arguments,
													 exec->argumentCount(),
													 Local<v8::Object>(thisValue),
													 Local<v8::Object>(holder),
													 Local<Value>(newTarget),
													 isConstructorCall,
													 Local<Value>(m_callAsFunctionCallbackData.get()),
													 reinterpret_cast<v8::Isolate *>(m_isolate),
													 &returnValue);
	m_callAsFunctionCallback(callbackInfo);

	return returnValue;
}

void Template::finishCreation(JSC::VM& vm, const WTF::String& name)
{
	Base::finishCreation(vm, name);
}

void Template::visitChildren(JSC::JSCell* cell, JSC::SlotVisitor& visitor)
{
	Base::visitChildren(cell, visitor);

	Template * thisObject = JSC::jsCast<Template *>(cell);

	visitor.append(thisObject->m_callAsFunctionCallbackData);
	for (auto& property : thisObject->m_properties)
	{
		property->visitChildren(visitor);
	}	
}

void Template::destroy(JSC::JSCell* cell)
{
	static_cast<Template*>(cell)->~Template();
}

void Template::ensureNotInstantiated(const char * v8Caller) const
{
	if (m_instantiated)
	{
		// TODO: Match v8's output exactly? (might matter for unit tests)
		jscshim::ReportApiFailure(v8Caller, "Template already instantiated");
	}
}

}} // v8::jscshim