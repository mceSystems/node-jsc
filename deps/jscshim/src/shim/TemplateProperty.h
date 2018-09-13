/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include "v8.h"
#include "APIAccessor.h"

#include <JavaScriptCore/CallFrame.h>

namespace v8 { namespace jscshim
{

// TOOD: Is it really OK to cache the PropertyName?
class TemplateProperty
{
protected:
	JSC::WriteBarrier<JSC::JSCell> m_name;
	JSC::PropertyName m_propertyName;
	unsigned int m_attributes;

public:
	virtual void visitChildren(JSC::SlotVisitor& visitor)
	{
		// Note: Using "append" causes a compilation error
		visitor.appendUnbarriered(m_name.get());
	}

	virtual JSC::JSValue instantiate(JSC::VM& vm, JSC::ExecState * exec, bool isHiddenPrototype) = 0;

	const JSC::PropertyName& propertyName() const { return m_propertyName; }
	unsigned int attributes() const { return m_attributes; }
	
protected:
	TemplateProperty(JSC::ExecState * exec, JSC::JSCell * owner, JSC::JSCell * name, unsigned int attributes);
};

class TemplateValueProperty : public TemplateProperty
{
private:
	JSC::WriteBarrier<JSC::Unknown> m_value;

public:
	TemplateValueProperty(JSC::ExecState * exec, 
						  JSC::JSCell	 * owner, 
						  JSC::JSCell	 * name, 
						  unsigned int	 attributes, 
						  JSC::JSValue	 value);

	virtual void visitChildren(JSC::SlotVisitor& visitor) override
	{
		TemplateProperty::visitChildren(visitor);

		// Note: Using "append" causes a compilation error
		visitor.appendUnbarriered(m_value.get());
	}

	virtual JSC::JSValue instantiate(JSC::VM& vm, JSC::ExecState * exec, bool isHiddenPrototype) override;
};

class TemplateAccessorProperty : public TemplateProperty
{
private:
	JSC::WriteBarrier<jscshim::FunctionTemplate> m_setter;
	JSC::WriteBarrier<jscshim::FunctionTemplate> m_getter;

public:
	TemplateAccessorProperty(JSC::ExecState				* exec,
							 JSC::JSCell				* owner, 
							 JSC::JSCell				* name, 
							 unsigned int				attributes, 
							 jscshim::FunctionTemplate	* getter, 
							 jscshim::FunctionTemplate	* setter);

	virtual void visitChildren(JSC::SlotVisitor& visitor) override;

	virtual JSC::JSValue instantiate(JSC::VM& vm, JSC::ExecState * exec, bool isHiddenPrototype) override;
};

class TemplateAccessor : public TemplateProperty
{
private:
	// TODO: Poison?
	v8::AccessorNameGetterCallback m_getter;
	v8::AccessorNameSetterCallback m_setter;
	JSC::WriteBarrier<JSC::Unknown> m_data;
	JSC::WriteBarrier<jscshim::FunctionTemplate> m_signature;
	bool m_isSpecialDataProperty;
	
public:
	TemplateAccessor(JSC::ExecState					* exec,
					 JSC::JSCell					* owner, 
					 JSC::JSCell					* name, 
					 unsigned int					attributes, 
					 v8::AccessorNameGetterCallback	getter, 
					 v8::AccessorNameSetterCallback	setter,
					 JSC::JSValue					data,
					 FunctionTemplate				* signature,
					 bool							isSpecialDataProperty);

	virtual void visitChildren(JSC::SlotVisitor& visitor) override;

	virtual JSC::JSValue instantiate(JSC::VM& vm, JSC::ExecState * exec, bool isHiddenPrototype) override;
};

}} // v8::jscshim