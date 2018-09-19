/*
 * Copyright (C) 2018 Koby Boyango <koby.b@mce.systems>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "JSCell.h"
#include "JSCJSValue.h"
#include "PropertyName.h"
#include "JSCPoison.h"

namespace JSC {

class JSGlobalObject;
class JSObject;
class Structure;

class CustomAPIValue : public JSCell {
public:
	using Base = JSCell;
	static const unsigned StructureFlags = Base::StructureFlags | StructureIsImmortal;

	using Getter = JSValue (*)(CustomAPIValue * thisApiValue, ExecState* exec, JSObject* slotBase, JSValue receiver, PropertyName propertyName);
	using Setter = bool (*)(CustomAPIValue * thisApiValue, ExecState* exec, JSObject* slotBase, JSValue receiver, PropertyName propertyName, JSValue value, bool isStrictMode);

	static CustomAPIValue* create(VM& vm, Getter getter = nullptr, Setter setter = nullptr)
	{
		CustomAPIValue* customAPIValue = new (NotNull, allocateCell<CustomAPIValue>(vm.heap)) CustomAPIValue(vm, vm.customAPIValueStructure.get(), getter, setter);
		customAPIValue->finishCreation(vm);
		return customAPIValue;
	}

	static JSC::Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype);

	DECLARE_EXPORT_INFO;

	JSValue get(ExecState* exec, JSObject* slotBase, JSValue receiver, PropertyName propertyName)
	{
		return m_getter.unpoisoned()(this, exec, slotBase, receiver, propertyName);
	}

	bool set(ExecState* exec, JSObject* slotBase, JSValue receiver, PropertyName propertyName, JSValue value, bool isStrictMode)
	{
		return m_setter.unpoisoned()(this, exec, slotBase, receiver, propertyName, value, isStrictMode);
	}

protected:
	CustomAPIValue(VM& vm, Structure* structure, Getter getter, Setter setter)
		: JSCell(vm, structure)
		, m_getter(getter ? getter : defaultGetter)
		, m_setter(setter ? setter : defaultSetter)
	{
	}

private:
	static JSValue defaultGetter(CustomAPIValue * thisApiValue, ExecState* exec, JSObject * slotBase, JSValue receiver, PropertyName propertyName)
	{
		UNUSED_PARAM(thisApiValue);
		UNUSED_PARAM(exec);
		UNUSED_PARAM(slotBase);
		UNUSED_PARAM(receiver);
		UNUSED_PARAM(propertyName);

		return jsUndefined();
	}

	static bool defaultSetter(CustomAPIValue * thisApiValue, ExecState* exec, JSObject * slotBase, JSValue receiver, PropertyName propertyName, JSValue value, bool isStrictMode)
	{
		UNUSED_PARAM(thisApiValue);
		UNUSED_PARAM(exec);
		UNUSED_PARAM(slotBase);
		UNUSED_PARAM(receiver);
		UNUSED_PARAM(propertyName);
		UNUSED_PARAM(value);
		UNUSED_PARAM(isStrictMode);

		return true;
	}

	Poisoned<NativeCodePoison, Getter> m_getter;
	Poisoned<NativeCodePoison, Setter> m_setter;
};

} // namespace JSC
