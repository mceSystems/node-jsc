/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include "v8.h"

#include <JavaScriptCore/CustomAPIValue.h>
#include <JavaScriptCore/Structure.h>

namespace v8 { namespace jscshim
{

class FunctionTemplate;

/* TODO: isSpecialDataProperty handling: In v8, the "SetAccessor" and "SetNativeDataProperty" apis
 * (on Objects or Templates) are similar, and provide almost the same functionality. The documentation in v8.h
 * doesn't really help in understanding the differences. Internally, they all call 
 * MakeAccessorInfo (in api.cc), which creates an AccessorInfo, with one difference: SetNativeDataProperty
 * set MakeAccessorInfo's is_special_data_property argument to true. Looking at MakeAccessorInfo:
 * - If is_special_data_property is set to true and there is no setter (null), an internal setter called
 *   i::Accessors::ReconfigureToDataProperty will be set instead, which as it's name suggests, 
 *   replaces the accessor with a regular data property (with the value passed to the setter).
 * - The AccessorInfo's is_special_data_property is set to true. Searching through v8's code,
 *   the following places reference this property:
 *   - LookupIterator::PrepareTransitionToDataProperty: A debug check (verifying that if this is an
 *     AccessorInfo it's special_data_property is set).
 *   - Object::SetPropertyInternal: It seems the if an accessor is found in the prototype chain,
 *     but not directly on the receiver or a hidden prototype, it will be ignored and overriden 
 *     if it's special_data_property is set.
 *   - StoreIC::ComputeHandler: Seem to perform the same check as Object::SetPropertyInternal,
 *     but I didn't go through they code.
 *   
 * There's also a comment in v8's https://github.com/v8/v8/commit/1da951ad0bd1a3c2247863060b87adeaef68fbd8
 * commit, saying "properties set by SetNativeDataProperty without kReadOnly flag can be replaced".
 * So basically, it seems that SetNativeDataProperty properties are "just" replaceables accessors. */
class APIAccessor final : public JSC::CustomAPIValue
{
private:
	// TODO: Poison
	v8::AccessorNameGetterCallback m_getterCallback;
	v8::AccessorNameSetterCallback m_setterCallback;

	JSC::WriteBarrier<JSC::JSCell> m_name;
	JSC::WriteBarrier<JSC::Unknown> m_data;
	JSC::WriteBarrier<FunctionTemplate> m_signature;

public:
	typedef JSC::CustomAPIValue Base;

	static APIAccessor * create(JSC::VM&					   vm, 
								JSC::Structure				   * structure,
								JSC::JSCell					   * name,
								v8::AccessorNameGetterCallback getterCallback,
								v8::AccessorNameSetterCallback setterCallback,
								JSC::JSValue				   data,
								FunctionTemplate			   * signature,
								bool						   isSpecialDataProperty)
	{
		Setter setter = (!setterCallback && isSpecialDataProperty) ? reconfigureToDataPropertySetter : APIAccessor::setter;

		APIAccessor * apiAccessor = new (NotNull, JSC::allocateCell<APIAccessor>(vm.heap)) APIAccessor(vm, 
																									   structure, 
																									   getterCallback, 
																									   setterCallback, 
																									   setter);
		apiAccessor->finishCreation(vm, name, data, signature);

		return apiAccessor;
	}

	static JSC::Structure * createStructure(JSC::VM& vm, JSC::JSGlobalObject * globalObject, JSC::JSValue prototype)
	{
		return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::CustomAPIValueType, StructureFlags), info());
	}

	DECLARE_INFO;

private:
	APIAccessor(JSC::VM&					   vm, 
				JSC::Structure				   * structure, 
				v8::AccessorNameGetterCallback getterCallback,
				v8::AccessorNameSetterCallback setterCallback,
				Setter						   setter = APIAccessor::setter) : Base(vm, structure, APIAccessor::getter, setter),
		m_getterCallback(getterCallback),
		m_setterCallback(setterCallback)
	{
	}

	void finishCreation(JSC::VM& vm, JSC::JSCell * name, JSC::JSValue data, FunctionTemplate * signature);
	
	static void visitChildren(JSCell *, JSC::SlotVisitor&);

	static JSC::JSValue getter(Base				 * thisApiValue, 
							   JSC::ExecState	 * exec,
							   JSC::JSObject	 * slotBase, 
							   JSC::JSValue		 receiver, 
							   JSC::PropertyName propertyName);

	static bool setter(Base				 * thisApiValue,
					   JSC::ExecState	 * exec,
					   JSC::JSObject	 * slotBase, 
					   JSC::JSValue		 receiver, 
					   JSC::PropertyName propertyName, 
					   JSC::JSValue		 value,
					   bool				 isStrictMode);

	static bool reconfigureToDataPropertySetter(Base			 * thisApiValue,
											    JSC::ExecState	 * exec,
											    JSC::JSObject	 * slotBase, 
											    JSC::JSValue	  receiver, 
											    JSC::PropertyName propertyName, 
											    JSC::JSValue	  value,
											    bool			  isStrictMode);

	JSC::JSValue throwIncompatibleMethodReceiverError(JSC::ExecState * exec, const JSC::JSValue& receiver);
};

}} // v8::jscshim