/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include "v8.h"

#include "shim/Isolate.h"
#include "shim/External.h"
#include "shim/helpers.h"

#include <JavaScriptCore/JSCJSValue.h>
#include <JavaScriptCore/JSCell.h>
#include <JavaScriptCore/CatchScope.h>

#include <JavaScriptCore/JSPromise.h>
#include <JavaScriptCore/DateInstance.h>
#include <JavaScriptCore/RegExpObject.h>
#include <JavaScriptCore/NativeErrorPrototype.h>

#include <JavaScriptCore/JSAsyncFunction.h>
#include <JavaScriptCore/JSGeneratorFunction.h>
#include <JavaScriptCore/GeneratorPrototype.h>

#include <JavaScriptCore/JSSet.h>
#include <JavaScriptCore/JSSetIterator.h>
#include <JavaScriptCore/JSMap.h>
#include <JavaScriptCore/JSMapIterator.h>
#include <JavaScriptCore/JSWeakMap.h>
#include <JavaScriptCore/JSWeakSet.h>

#include <JavaScriptCore/JSArrayBuffer.h>
#include <JavaScriptCore/JSArrayBufferView.h>
#include <JavaScriptCore/JSDataView.h>

#include <JavaScriptCore/BigIntObject.h>
#include <JavaScriptCore/BooleanObject.h>
#include <JavaScriptCore/NumberObject.h>
#include <JavaScriptCore/StringObject.h>
#include <JavaScriptCore/SymbolObject.h>

#include <JavaScriptCore/Operations.h>
#include <JavaScriptCore/JSCInlines.h>

namespace
{

bool GetTypedArrayType(const v8::Value * v8Value, JSC::TypedArrayType * arrayType)
{
	JSC::VM& vm = v8::jscshim::GetCurrentVM();
	
	JSC::JSArrayBufferView * array = v8::jscshim::GetJscCellFromV8Checked<JSC::JSArrayBufferView>(vm, v8Value);
	if (nullptr == array)
	{
		return false;
	}
	
	*arrayType = array->classInfo(vm)->typedArrayStorageType;
	return true;
}

}

#define DEFINE_IS_TYPE_FUNCTION(v8Type, jscType) \
	bool Value::Is##v8Type() const \
	{ \
		return v8::jscshim::GetValue(this).is##jscType();\
	}

#define DEFINE_IS_OBJECT_TYPE_FUNCTION(v8Type, jscType) \
	bool Value::Is##v8Type() const \
	{ \
		JSC::JSValue value = v8::jscshim::GetValue(this); \
		return value.inherits(v8::jscshim::GetCurrentVM(), JSC::jscType::info()); \
	}

#define DEFINE_IS_TYPED_ARRAY_FUNCTION(v8ArrayType) \
	bool Value::Is##v8ArrayType##Array() const \
	{ \
		JSC::TypedArrayType arrayType = JSC::NotTypedArray;\
		if (false == GetTypedArrayType(this, &arrayType))\
		{\
			return false;\
		}\
		return (JSC::Type##v8ArrayType == arrayType);\
	}

/* Note: In the following two macros, we use the jscshim::SuspendRestoreShimExceptionScope.
 * Consider the following example from v8's unit tests (CatchZero test):
 *		v8::TryCatch try_catch(context->GetIsolate());
 *		CompileRun("throw 10");
 *		try_catch.Exception()->Int32Value(context.local()).FromJust();
 *
 * Int32Value is called when there's already an exception caught, thus we
 * can't just "regular" exception scope to check for conversion
 * exceptions. SuspendRestoreShimExceptionScope will clear the current exception
 * upon entry and restore it on exit.
 */
#define DEFINE_TYPED_VALUE_FUNCTION(v8Type, nativeType, jscCast, defaultValueForMaybe) \
	Maybe<nativeType> Value::v8Type##Value(Local<Context> context) const \
	{\
		JSC::ExecState * exec = jscshim::GetExecStateForV8Context(*context); \
		JSC::JSValue value = jscshim::GetValue(this); \
		DECLARE_SHIM_SUSPEND_RESTORE_EXCEPTION_SCOPE(*context); \
		\
		nativeType convertedValue = value.to##jscCast(exec); \
		SHIM_RETURN_IF_EXCEPTION(Nothing<nativeType>()); \
		\
		return Just<nativeType>(convertedValue); \
	}

#define DEFINE_CAST_TO_TYPE_FUNCTION(v8Type, jscFactory, jscCast) \
	MaybeLocal<v8Type> Value::To##v8Type(Local<Context> context) const \
	{ \
		JSC::ExecState * exec = jscshim::GetExecStateForV8Context(*context); \
		DECLARE_SHIM_SUSPEND_RESTORE_EXCEPTION_SCOPE(*context); \
		JSC::JSValue value = jscshim::GetValue(this); \
		\
		JSC::JSValue convertedValue = JSC::js##jscFactory(value.to##jscCast(exec)); \
		SHIM_RETURN_IF_EXCEPTION(Local<v8Type>()); \
		\
		return Local<v8Type>::New(convertedValue); \
	}