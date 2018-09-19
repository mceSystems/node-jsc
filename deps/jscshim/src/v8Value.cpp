/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8Value.h"

// TODO: Add JSLockHolder to all calls?

namespace v8
{

DEFINE_IS_TYPE_FUNCTION(Undefined, Undefined)
DEFINE_IS_TYPE_FUNCTION(Null, Null)
DEFINE_IS_TYPE_FUNCTION(NullOrUndefined, UndefinedOrNull)
DEFINE_IS_TYPE_FUNCTION(True, True)
DEFINE_IS_TYPE_FUNCTION(False, False)
DEFINE_IS_TYPE_FUNCTION(String, String)
DEFINE_IS_TYPE_FUNCTION(Symbol, Symbol)
DEFINE_IS_TYPE_FUNCTION(Object, Object)
DEFINE_IS_TYPE_FUNCTION(Boolean, Boolean)
DEFINE_IS_TYPE_FUNCTION(Number, Number)
DEFINE_IS_TYPE_FUNCTION(Int32, Int32)

DEFINE_IS_OBJECT_TYPE_FUNCTION(Date, DateInstance)
DEFINE_IS_OBJECT_TYPE_FUNCTION(ArrayBuffer, JSArrayBuffer)
DEFINE_IS_OBJECT_TYPE_FUNCTION(ArrayBufferView, JSArrayBufferView)
DEFINE_IS_OBJECT_TYPE_FUNCTION(DataView, JSDataView)

DEFINE_IS_OBJECT_TYPE_FUNCTION(Array, JSArray)
DEFINE_IS_OBJECT_TYPE_FUNCTION(RegExp, RegExpObject)
DEFINE_IS_OBJECT_TYPE_FUNCTION(Promise, JSPromise)
DEFINE_IS_OBJECT_TYPE_FUNCTION(AsyncFunction, JSAsyncFunction)
DEFINE_IS_OBJECT_TYPE_FUNCTION(GeneratorFunction, JSGeneratorFunction)
DEFINE_IS_OBJECT_TYPE_FUNCTION(Set, JSSet)
DEFINE_IS_OBJECT_TYPE_FUNCTION(Map, JSMap)
DEFINE_IS_OBJECT_TYPE_FUNCTION(WeakMap, JSWeakMap)
DEFINE_IS_OBJECT_TYPE_FUNCTION(WeakSet, JSWeakSet)
DEFINE_IS_OBJECT_TYPE_FUNCTION(BigIntObject, BigIntObject)
DEFINE_IS_OBJECT_TYPE_FUNCTION(BooleanObject, BooleanObject)
DEFINE_IS_OBJECT_TYPE_FUNCTION(NumberObject, NumberObject)
DEFINE_IS_OBJECT_TYPE_FUNCTION(StringObject, StringObject)
DEFINE_IS_OBJECT_TYPE_FUNCTION(SymbolObject, SymbolObject)
DEFINE_IS_OBJECT_TYPE_FUNCTION(BigInt, JSBigInt)

/* TODO: According to https://github.com/WebKit/webkit/commit/0cf9fa0aced9a69b83213166f6f75e6c75a7cc68,
 * since map\set iterators were optimized with intrinsic, the iterators are only used by WebCore,
 * "So they are not used in user observable JS.". Since the iterators are not exposed through
 * v8's api, this is currently seems pointless. Should we do something else? */
DEFINE_IS_OBJECT_TYPE_FUNCTION(MapIterator, JSMapIterator)
DEFINE_IS_OBJECT_TYPE_FUNCTION(SetIterator, JSSetIterator)

DEFINE_IS_TYPED_ARRAY_FUNCTION(Uint8)
DEFINE_IS_TYPED_ARRAY_FUNCTION(Uint8Clamped)
DEFINE_IS_TYPED_ARRAY_FUNCTION(Int8)
DEFINE_IS_TYPED_ARRAY_FUNCTION(Uint16)
DEFINE_IS_TYPED_ARRAY_FUNCTION(Int16)
DEFINE_IS_TYPED_ARRAY_FUNCTION(Uint32)
DEFINE_IS_TYPED_ARRAY_FUNCTION(Int32)
DEFINE_IS_TYPED_ARRAY_FUNCTION(Float32)
DEFINE_IS_TYPED_ARRAY_FUNCTION(Float64)

DEFINE_TYPED_VALUE_FUNCTION(Boolean, bool, Boolean, false)
DEFINE_TYPED_VALUE_FUNCTION(Number, double, Number, JSC::pureNAN())
DEFINE_TYPED_VALUE_FUNCTION(Integer, int64_t, Integer, 0)
DEFINE_TYPED_VALUE_FUNCTION(Int32, int32_t, Int32, 0)
DEFINE_TYPED_VALUE_FUNCTION(Uint32, uint32_t, UInt32, 0)

DEFINE_CAST_TO_TYPE_FUNCTION(Boolean, Boolean, Boolean)
DEFINE_CAST_TO_TYPE_FUNCTION(Number, Number, Number)
DEFINE_CAST_TO_TYPE_FUNCTION(Integer, Number, Integer)
DEFINE_CAST_TO_TYPE_FUNCTION(Int32, Number, Int32)
DEFINE_CAST_TO_TYPE_FUNCTION(Uint32, Number, UInt32)

/* Note: It seems that JSC might store unsigned integers as doubles, thus using "isUInt32" might fail.
 * For example, 0x81234567 (in v8's "isNumberType" test) fails when using isUInt32. 
 * Using getUInt32 (which also handles doubles properly, returning true if they're actually 
 * "holding" an integer) is better, but fails for negative zero (returns true, we're supposed to return false
 * according to v8's tests). Thus, we'll implement it here ourselves. */
bool Value::IsUint32() const
{
	JSC::JSValue value = jscshim::GetValue(this);
	if (value.isInt32() && (value.asInt32() >= 0))
	{
		return true;
	}

	if (value.isDouble())
	{
		double doubleValue = value.asDouble();
		return ((false == std::signbit(doubleValue)) &&
			    (static_cast<uint32_t>(doubleValue) == doubleValue));
	}

	return false;
}

bool Value::IsFunction() const
{
	/* The name "isFunction" is a bit misleading, it also checks if the value is callable,
	 * but it seems what v8 does too */
	return jscshim::GetValue(this).isFunction(jscshim::GetCurrentVM());
}

MaybeLocal<String> Value::ToString(Local<Context> context) const
{
	/* We might get called after an exception was caught (to convert it to a string),
	 * so we need to clear the current exception or else obj->ToString will fail,
	 * because of internal use of JSC's RETURN_IF_EXCEPTION macro everywhere (which will
	 * cause JSC::JSValue::toStringOrNull to fail since it will "think" a new exception
	 * was thrown in the process). jscshim::SuspendRestoreShimExceptionScope will clear
	 * the exception for us and restore it when we leave. */
	DECLARE_SHIM_SUSPEND_RESTORE_EXCEPTION_SCOPE(*context);

	JSC::JSString * stringValue = nullptr;
	{
		/* Note that in case this value is an object, JSC::JSValue::toStringOrNull might call 
		 * the object's toString function, which may be overidden by an api created function 
		 * (see the CorrectEnteredContext unit test). Thus, we need to make sure "context"
		 * is the current context. */
		jscshim::GlobalObject * global = jscshim::GetGlobalObjectForV8Context(*context);
		v8::jscshim::Isolate::CurrentContextScope currentContextScope(global);

		stringValue = jscshim::GetValue(this).toStringOrNull(global->v8ContextExec());
	}

	return Local<String>::New(JSC::JSValue(stringValue));
}

MaybeLocal<String> Value::ToDetailString(Local<Context> context) const
{
	// TODO: Follow v8's ToDetailString
	return ToString(context);
}

MaybeLocal<Object> Value::ToObject(Local<Context> context) const
{
	jscshim::GlobalObject * global = jscshim::GetGlobalObjectForV8Context(*context);
	JSC::ExecState * exec = global->v8ContextExec();
	DECLARE_SHIM_EXCEPTION_SCOPE(*context);

	JSC::JSObject * valueObj = jscshim::GetValue(this).toObject(exec, global);
	return Local<Object>::New(JSC::JSValue(valueObj));
}

bool Value::IsExternal() const
{
	JSC::JSValue value = v8::jscshim::GetValue(this);
	return value.inherits(v8::jscshim::GetCurrentVM(), jscshim::External::info());
}

bool Value::IsNativeError() const
{
	JSC::VM& vm = jscshim::GetCurrentVM();

	// In JSC, native errors have a prototype but not an "instance" c++ class
	JSC::ErrorInstance * error = jscshim::GetJscCellFromV8Checked<JSC::ErrorInstance>(vm, this);
	return error && jscshim::IsInstanceOf<JSC::NativeErrorPrototype>(vm, error->getPrototypeDirect(vm));
}

bool Value::IsProxy() const
{
	JSC::JSValue value = jscshim::GetValue(this);
	return (value.isCell() && (JSC::ProxyObjectType == value.asCell()->type()));
}

bool Value::IsWebAssemblyCompiledModule() const
{
	// TODO: IMEPLEMENT when we support Web Assembly
	return false;
}

bool Value::IsModuleNamespaceObject() const
{
	// TODO: IMEPLEMENT
	return false;
}

bool Value::IsName() const
{
	JSC::JSValue value = jscshim::GetValue(this);
	if (false == value.isCell())
	{
		return false;
	}

	JSC::JSCell * valueCell = value.asCell();
	return (valueCell->isString() || valueCell->isSymbol());
}

bool Value::IsArgumentsObject() const
{
	JSC::JSValue value = jscshim::GetValue(this);
	if (false == value.isCell())
	{
		return false;
	}

	// Based on JSObject::canDoFastPutDirectIndex
	JSC::JSType cellType = value.asCell()->type();
	return ((JSC::DirectArgumentsType == cellType) ||
			(JSC::ScopedArgumentsType == cellType) ||
			(JSC::ClonedArgumentsType == cellType));
}

bool Value::IsGeneratorObject() const
{
	jscshim::GlobalObject * global = jscshim::GetCurrentGlobalObject();
	JSC::ExecState * exec = global->v8ContextExec();
	DECLARE_SHIM_EXCEPTION_SCOPE(exec);

	/* In JSC, there is no "generator object" class - they're simple objects with "next" etc. properties
	 * set on it (see BytecodeGenerator::emitPutGeneratorFields), but it's prototype is GeneratorPrototype 
	 * (see JSFunction::getOwnPropertySlot). GeneratorPrototype doesn't implement "hasInstance", so we'll
	 * call the default one manually. */
	return JSC::JSObject::defaultHasInstance(exec, jscshim::GetValue(this), global->generatorPrototype());
}

bool Value::IsBigInt64Array() const
{
	// TODO: IMPLEMENT
	return false;
}

bool Value::IsBigUint64Array() const
{
	// TODO: IMPLEMENT
	return false;
}

bool Value::IsSharedArrayBuffer() const
{
	JSC::JSArrayBuffer * arrayBuffer = jscshim::GetJscCellFromV8Checked<JSC::JSArrayBuffer>(jscshim::GetCurrentVM(), this);
	return arrayBuffer && arrayBuffer->isShared();
}

bool Value::IsTypedArray() const
{
	JSC::TypedArrayType arrayType = JSC::NotTypedArray;
	if (false == GetTypedArrayType(this, &arrayType))
	{
		return false;
	}

	return ((JSC::TypeDataView != arrayType) && (JSC::NotTypedArray != arrayType));
}

MaybeLocal<Uint32> Value::ToArrayIndex(Local<Context> context) const
{
	JSC::ExecState * exec = jscshim::GetExecStateForV8Context(*context);
	DECLARE_SHIM_EXCEPTION_SCOPE(exec);
		
	JSC::JSString * stringValue = jscshim::GetValue(this).toStringOrNull(exec);
	SHIM_RETURN_IF_EXCEPTION(Local<Uint32>());

	/* The const_cast is ugly, but for some reason JSC::parseIndex doesn't take a const StringImpl 
	 * although it won't change it) */
	std::optional<uint32_t> asIndex = JSC::parseIndex(const_cast<WTF::StringImpl&>(*(stringValue->tryGetValueImpl())));
	if (!asIndex)
	{
		return Local<Uint32>();
	}

	return Local<Uint32>(JSC::jsNumber(asIndex.value()));
}

double Value::NumberValue() const
{
	return NumberValue(Isolate::GetCurrent()->GetCurrentContext()).FromMaybe(JSC::pureNaN());
}

/* Note that we use the SuspendRestoreShimExceptionScope in the following functions, 
 * since in v8 unit tests Value::Equals gets called after an exception was called. When comparing
 * strings, this might fail since JSString::equalSlowCase checks for exceptions and will always fail
 * in this case. */

bool Value::Equals(Local<Value> that) const
{
	JSC::ExecState * exec = jscshim::GetCurrentExecState();
	DECLARE_SHIM_SUSPEND_RESTORE_EXCEPTION_SCOPE(exec);

	return JSC::JSValue::equal(exec, jscshim::GetValue(this), that.val_);
}

Maybe<bool> Value::Equals(Local<Context> context, Local<Value> that) const
{
	JSC::ExecState * exec = jscshim::GetExecStateForV8Context(*context);
	DECLARE_SHIM_SUSPEND_RESTORE_EXCEPTION_SCOPE(exec);

	return Just(JSC::JSValue::equal(exec, jscshim::GetValue(this), that.val_));
}

bool Value::StrictEquals(Local<Value> that) const
{
	JSC::ExecState * exec = jscshim::GetCurrentExecState();
	DECLARE_SHIM_SUSPEND_RESTORE_EXCEPTION_SCOPE(exec);

	return JSC::JSValue::strictEqual(exec, jscshim::GetValue(this), that.val_);
}

bool Value::SameValue(Local<Value> that) const
{
	JSC::ExecState * exec = jscshim::GetCurrentExecState();
	DECLARE_SHIM_SUSPEND_RESTORE_EXCEPTION_SCOPE(exec);

	return JSC::sameValue(exec, jscshim::GetValue(this), that.val_);
}

Local<String> Value::TypeOf(Isolate * isolate)
{
	jscshim::GlobalObject * global = jscshim::GetGlobalObjectForV8Isolate(isolate);

	JSC::JSValue typeOf = JSC::jsTypeStringForValue(jscshim::V8IsolateToJscShimIsolate(isolate)->VM(), 
												    global, 
												    jscshim::GetValue(this));
	return Local<String>::New(typeOf);
}

Maybe<bool> Value::InstanceOf(Local<Context> context, Local<Object> object)
{
	JSC::ExecState * exec = jscshim::GetExecStateForV8Context(*context);
	DECLARE_SHIM_EXCEPTION_SCOPE(*context);

	JSC::JSObject * constructor = jscshim::GetJscCellFromV8<JSC::JSObject>(*object);
	bool result = constructor->hasInstance(exec, jscshim::GetValue(this));
	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());

	return Just(result);
}

} // v8