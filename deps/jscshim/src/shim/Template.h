/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include "v8.h"

#include "TemplateProperty.h"
#include "helpers.h"

#include <JavaScriptCore/JSDestructibleObject.h>
#include <JavaScriptCore/GetterSetter.h>
#include <wtf/Vector.h>

#include <memory>

namespace v8 { namespace jscshim
{
class Function;
class FunctionTemplate;

class Template : public JSC::InternalFunction {
protected:
	Isolate * m_isolate;
	bool m_forceFunctionInstantiation;

	v8::FunctionCallback m_callAsFunctionCallback;
	JSC::WriteBarrier<JSC::Unknown> m_callAsFunctionCallbackData;

private:
	bool m_instantiated;
	WTF::Vector<std::unique_ptr<TemplateProperty>> m_properties;
	
public:
	typedef InternalFunction Base;

	DECLARE_INFO;

	static JSC::Structure * createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
	{
		return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::InternalFunctionType, StructureFlags), info());
	}

	void SetProperty(JSC::ExecState * exec, JSC::JSCell * name, JSC::JSValue value, int attributes);
	void SetAccessorProperty(JSC::ExecState			   * exec, 
							 JSC::JSCell			   * name, 
							 jscshim::FunctionTemplate * getter, 
							 jscshim::FunctionTemplate * setter, 
							 int					   attributes);
	void SetAccessor(JSC::ExecState					* exec, 
					 JSC::JSCell					* name, 
					 v8::AccessorNameGetterCallback	getter,
					 v8::AccessorNameSetterCallback	setter, 
					 JSC::JSValue					data,
					 jscshim::FunctionTemplate		* signatureReceiver,
					 int							attributes,
					 bool							isSpecialDataProperty);

protected:
	Template(JSC::VM&			 vm,
			 JSC::Structure		 * structure,
			 Isolate			 * isolate,
			 JSC::NativeFunction functionForCall,
			 JSC::NativeFunction functionForConstruct) : Base(vm, structure, functionForCall, functionForConstruct),
		m_isolate(isolate),
		m_forceFunctionInstantiation(false),
		m_callAsFunctionCallback(nullptr),
		m_instantiated(false)
	{
	}

	void finishCreation(JSC::VM& vm, const WTF::String& name);

	static void visitChildren(JSC::JSCell*, JSC::SlotVisitor&);
	static void destroy(JSC::JSCell*);

	void applyPropertiesToInstance(JSC::ExecState * exec, JSC::JSObject * instance);

	JSC::JSValue forwardCallToCallback(JSC::ExecState		* exec, 
									   const JSC::JSValue&	thisValue, 
									   const JSC::JSValue&	holder, 
									   bool					isConstructorCall);

	// Will crash if instantiated
	void ensureNotInstantiated(const char * v8Caller) const;
};

}}