/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "TemplateProperty.h"

#include "FunctionTemplate.h"
#include "ObjectTemplate.h"
#include "Function.h"
#include "Object.h"

#include <JavaScriptCore/Symbol.h>
#include <JavaScriptCore/JSObject.h>
#include <JavaScriptCore/JSString.h>
#include <JavaScriptCore/JSCInlines.h>

namespace v8 { namespace jscshim
{

TemplateProperty::TemplateProperty(JSC::ExecState * exec, JSC::JSCell * owner, JSC::JSCell * name, unsigned int attributes) :
	m_propertyName(name->isSymbol() ? JSC::PropertyName(JSC::Identifier::fromUid(JSC::jsCast<JSC::Symbol *>(name)->privateName())) :
									  JSC::PropertyName(JSC::jsCast<JSC::JSString *>(name)->toIdentifier(exec))),
	m_attributes(attributes)
{
	ASSERT(name->isString() || name->isSymbol());

	JSC::VM& vm = exec->vm();
	m_name.set(vm, owner, name);
}

TemplateValueProperty::TemplateValueProperty(JSC::ExecState * exec,
											 JSC::JSCell	* owner, 
											 JSC::JSCell	* name, 
											 unsigned int	attributes, 
											 JSC::JSValue	value) :
	TemplateProperty(exec, owner, name, attributes)
{
	JSC::VM& vm = exec->vm();

	m_value.set(vm, owner, value);

	/* If this value is a FunctionTemplate, and we have a string name - update it's name property
	 * TODO: Handle Function instances?
	 * TODO: Handle symbol names? */
	if (name->isString())
	{
		if (jscshim::FunctionTemplate * asFunctionTemplate = JSC::jsDynamicCast<jscshim::FunctionTemplate *>(vm, value))
		{
			asFunctionTemplate->setName(vm, JSC::jsCast<JSC::JSString *>(name));
		}
	}
}

JSC::JSValue TemplateValueProperty::instantiate(JSC::VM& vm, JSC::ExecState * exec, bool isHiddenPrototype)
{
	JSC::JSValue value = m_value.get();

	// v8 supports passing ObjectTemplates to Template::Set
	ObjectTemplate * objTemplate = JSC::jsDynamicCast<ObjectTemplate *>(vm, value);
	return objTemplate ? objTemplate->makeNewObjectInstance(exec, JSC::JSValue()) : value;
}

TemplateAccessorProperty::TemplateAccessorProperty(JSC::ExecState			* exec,
												   JSC::JSCell				 * owner, 
												   JSC::JSCell				 * name, 
												   unsigned int				 attributes, 
												   jscshim::FunctionTemplate * getter, 
												   jscshim::FunctionTemplate * setter) :
	TemplateProperty(exec, owner, name, attributes | JSC::PropertyAttribute::Accessor)
{
	JSC::VM& vm = exec->vm();
	m_getter.setMayBeNull(vm, owner, getter);
	m_setter.setMayBeNull(vm, owner, setter);


	/* If we have a string name - Update the getter\setter names.
	 * TODO: Do this lazily
	 * TODO: Handle symbol names? */
	if (name->isString())
	{
		const WTF::String& baseName = JSC::jsCast<JSC::JSString *>(name)->tryGetValue();

		// TODO: Cache the "get " and "set " strings
		if (getter)
		{
			getter->setName(vm, JSC::jsString(&vm, WTF::makeString("get ", baseName)));
		}

		if (setter)
		{
			setter->setName(vm, JSC::jsString(&vm, WTF::makeString("set ", baseName)));
		}
	}
}

void TemplateAccessorProperty::visitAggregate(JSC::SlotVisitor& visitor)
{
	TemplateProperty::visitAggregate(visitor);

	// Note: Using "append" causes a compilation error
	visitor.append(m_getter);
	visitor.append(m_setter);
}

JSC::JSValue TemplateAccessorProperty::instantiate(JSC::VM& vm, JSC::ExecState * exec, bool isHiddenPrototype)
{
	JSC::JSGlobalObject * globalObject = exec->lexicalGlobalObject();

	JSC::JSObject * getter = nullptr;
	JSC::JSObject * setter = nullptr;

	// Getter
	jscshim::FunctionTemplate * getterTemplate = m_getter.get();
	if (getterTemplate)
	{
		getter = isHiddenPrototype ? static_cast<JSC::JSObject *>(getterTemplate->makeNewFunctionInstance(exec)) :
									 static_cast<JSC::JSObject *>(getterTemplate);
	}

	// Setter
	jscshim::FunctionTemplate * setterTemplate = m_setter.get();
	if (setterTemplate)
	{
		setter = isHiddenPrototype ? static_cast<JSC::JSObject *>(setterTemplate->makeNewFunctionInstance(exec)) :
									 static_cast<JSC::JSObject *>(setterTemplate);
	}

	return JSC::GetterSetter::create(vm, globalObject, getter, setter);
}

TemplateAccessor::TemplateAccessor(JSC::ExecState				  * exec,
								   JSC::JSCell					  * owner, 
								   JSC::JSCell					  * name, 
								   unsigned int					  attributes, 
								   v8::AccessorNameGetterCallback getter, 
								   v8::AccessorNameSetterCallback setter,
								   JSC::JSValue					  data,
								   FunctionTemplate				  * signature,
								   bool							  isSpecialDataProperty) :
	TemplateProperty(exec, owner, name, attributes),
	m_getter(getter),
	m_setter(setter),
	m_isSpecialDataProperty(isSpecialDataProperty)
{
	JSC::VM& vm = exec->vm();
	m_data.set(vm, owner, data);
	m_signature.setMayBeNull(vm, owner, signature);
}

void TemplateAccessor::visitAggregate(JSC::SlotVisitor& visitor)
{
	TemplateProperty::visitAggregate(visitor);

	// Note: Using "append" for m_signature causes a compilation error
	visitor.append(m_data);
	visitor.append(m_signature);
}

JSC::JSValue TemplateAccessor::instantiate(JSC::VM& vm, JSC::ExecState * exec, bool isHiddenPrototype)
{
	return APIAccessor::create(vm,
							   jscshim::GetGlobalObject(exec)->shimAPIAccessorStructure(),
							   m_name.get(),
							   m_getter,
							   m_setter,
							   m_data.get(),
							   m_signature.get(),
							   m_isSpecialDataProperty);
}

}} // v8::jscshim