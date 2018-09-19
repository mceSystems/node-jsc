/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include "Isolate.h"
#include "GlobalObject.h"
#include "exceptions.h"

#include <JavaScriptCore/CodeBlock.h>
#include <JavaScriptCore/CallFrame.h>
#include <JavaScriptCore/Protect.h>
#include <JavaScriptCore/JSProxy.h>
#include <wtf/Assertions.h>

namespace v8 { namespace jscshim
{

#define SETUP_JSC_USE_IN_FUNCTION(context) v8::jscshim::GlobalObject * global = v8::jscshim::GetGlobalObjectForV8Context(*context); \
										   JSC::ExecState * exec = global->v8ContextExec(); \
										   JSC::VM& vm = exec->vm(); \
										   v8::jscshim::SuspendRestoreShimExceptionScope shimExceptionScope(global->isolate());\
										   v8::jscshim::Isolate::CurrentContextScope currentContextScope(global);

#define SETUP_OBJECT_USE_IN_MEMBER(context) SETUP_JSC_USE_IN_FUNCTION(context) \
											JSC::JSObject * thisObj = jscshim::GetValue(this).getObject(); \
											ASSERT(thisObj);

// Helpers for retreiving JSC values/objects from v8 objects

// v8 context, value and data pointers are always pointers to JSValue
template <typename v8Type>
inline JSC::JSValue GetValue(const v8Type * v8ptr)
{
	return *reinterpret_cast<const JSC::JSValue *>(v8ptr);
}

template <typename JscCellType, typename v8Type>
inline JscCellType * GetJscCellFromV8(v8Type * v8ptr)
{
	return reinterpret_cast<JscCellType *>(GetValue(v8ptr).asCell());
}

template <typename JscCellType, typename v8Type>
inline JscCellType * GetJscCellFromV8Checked(JSC::VM& vm, v8Type * v8ptr)
{
	return JSC::jsDynamicCast<JscCellType *>(vm, GetValue(v8ptr));
}

inline jscshim::Isolate * V8IsolateToJscShimIsolate(v8::Isolate * isolate)
{
	return reinterpret_cast<jscshim::Isolate *>(isolate);
}

// Helpers for retreiving GlobalObject/JSC::ExecState/JSC::VM from various sources
inline GlobalObject * GetGlobalObjectForV8Isolate(v8::Isolate * isolate)
{
	return reinterpret_cast<jscshim::Isolate *>(isolate)->GetCurrentGlobalOrDefault();
}

inline JSC::ExecState * GetExecStateForV8Isolate(v8::Isolate * isolate)
{
	return GetGlobalObjectForV8Isolate(isolate)->v8ContextExec();
}

inline GlobalObject * GetGlobalObjectForV8Context(v8::Context * context)
{
	// v8 contexts are GlobalObject instances
	return GetJscCellFromV8<GlobalObject>(context);
}

inline JSC::ExecState * GetExecStateForV8Context(v8::Context * context)
{
	return GetGlobalObjectForV8Context(context)->v8ContextExec();
}

inline JSC::VM& GetCurrentVM()
{
	return reinterpret_cast<jscshim::Isolate *>(v8::Isolate::GetCurrent())->VM();
}

inline JSC::ExecState * GetCurrentExecState()
{
	return GetExecStateForV8Isolate(v8::Isolate::GetCurrent());
}

inline GlobalObject * GetCurrentGlobalObject()
{
	return GetGlobalObjectForV8Isolate(v8::Isolate::GetCurrent());
}

inline GlobalObject * GetGlobalObject(JSC::ExecState * exec)
{
	return static_cast<GlobalObject *>(exec->lexicalGlobalObject());
}

inline Local<Context> GetV8ContextForObject(JSC::JSObject * obj)
{
	return Local<Context>::New(JSC::JSValue(obj->globalObject()));
}

// Object related helpers

template<typename Type>
inline bool IsInstanceOf(JSC::VM& vm, JSC::JSValue value)
{
	return value.isCell() && value.asCell()->inherits(vm, Type::info());
}

inline unsigned int v8PropertyAttributesToJscAttributes(v8::PropertyAttribute v8Attributes)
{
	unsigned int jscAttributes = 0;

	if (v8Attributes & v8::ReadOnly)   { jscAttributes |= JSC::PropertyAttribute::ReadOnly; }
	if (v8Attributes & v8::DontEnum)   { jscAttributes |= JSC::PropertyAttribute::DontEnum; }
	if (v8Attributes & v8::DontDelete) { jscAttributes |= JSC::PropertyAttribute::DontDelete; }

	return jscAttributes;
}

inline v8::PropertyAttribute JscPropertyAttributesToV8Attributes(unsigned int jscAttributes)
{
	int v8Attributes = v8::PropertyAttribute::None;

	if (jscAttributes & JSC::PropertyAttribute::ReadOnly) { v8Attributes |= v8::ReadOnly; }
	if (jscAttributes & JSC::PropertyAttribute::DontEnum) { v8Attributes |= v8::DontEnum; }
	if (jscAttributes & JSC::PropertyAttribute::DontDelete) { v8Attributes |= v8::DontDelete; }

	return static_cast<v8::PropertyAttribute>(v8Attributes);
}

inline Local<Name> JscPropertyNameToV8Name(JSC::ExecState * exec, const JSC::PropertyName& propertyName)
{
	JSC::JSCell * v8Name = nullptr;
	if (propertyName.isSymbol())
	{
		/* Create a JSC symbol object. Note that internally, Symbol::create will use the vm's symbolImplToSymbolMap
		 * map to get an existing symbol object. */
		WTF::SymbolImpl * symbolImpl = static_cast<WTF::SymbolImpl *>(propertyName.uid());
		v8Name = JSC::Symbol::create(exec->vm(), *symbolImpl);
	}
	else
	{
		v8Name = JSC::jsString(exec, WTF::String(propertyName.publicName()));
	}

	return Local<Name>::New(JSC::JSValue(v8Name));
}

inline JSC::PropertyName JscValueToPropertyName(JSC::ExecState * exec, JSC::JSValue value)
{
	if (value.isSymbol())
	{
		return JSC::asSymbol(value)->privateName();
	}

	if (value.isString())
	{
		return JSC::asString(value)->toIdentifier(exec);
	}

	JSC::VM& vm = exec->vm();
	auto scope = DECLARE_THROW_SCOPE(vm);

	// Based on JSValue::toPropertyKey
	JSC::JSValue primitive = value.toPrimitive(exec, JSC::PreferString);
	RETURN_IF_EXCEPTION(scope, JSC::PropertyName(vm.propertyNames->emptyIdentifier));
	if (primitive.isSymbol())
	{
		return JSC::asSymbol(primitive)->privateName();
	}
		
	scope.release();
	return primitive.toString(exec)->toIdentifier(exec);
}

// Based on JSC::createListFromArrayLike
inline uint64_t GetArrayLikeObjectLength(JSC::ExecState * exec, JSC::JSValue object)
{
	JSC::VM& vm = exec->vm();
	auto scope = DECLARE_THROW_SCOPE(vm);

	JSC::JSValue lengthProperty = object.get(exec, vm.propertyNames->length);
	RETURN_IF_EXCEPTION(scope, 0);
	double lengthAsDouble = lengthProperty.toLength(exec);
	RETURN_IF_EXCEPTION(scope, 0);
	RELEASE_ASSERT(lengthAsDouble >= 0.0 && lengthAsDouble == std::trunc(lengthAsDouble));

	return static_cast<uint64_t>(lengthAsDouble);
}

inline JSC::JSObject * GetConstructorFromPrototype(JSC::VM& vm, const JSC::JSObject * prototype)
{
	JSC::JSValue constructorProperty = prototype->getDirect(vm, vm.propertyNames->constructor);
	return constructorProperty.getObject();
}

inline JSC::JSObject * GetObjectConstructor(JSC::VM& vm, const JSC::JSObject * object)
{
	JSC::JSValue prototype = object->getPrototypeDirect(vm);
	if (!prototype.isObject())
	{
		return nullptr;
	}

	return GetConstructorFromPrototype(vm, prototype.getObject());
}

inline JSC::ECMAMode GetECMAMode(JSC::ExecState * exec)
{
	// Not that we might get called without a code block (happened on some of v8's UT)
	JSC::CodeBlock * codeBlock = exec->codeBlock();
	return (codeBlock && codeBlock->isStrictMode()) ? JSC::ECMAMode::StrictMode : JSC::ECMAMode::NotStrictMode;
}

inline bool isGlobalObjectOrGlobalProxy(JSC::JSObject * object)
{
	if (object->isGlobalObject())
	{
		return true;
	}

	if (JSC::JSProxy * proxy = JSC::jsDynamicCast<JSC::JSProxy *>(*object->vm(), object))
	{
		return proxy->target()->isGlobalObject();
	}
	
	return false;
}

/* Based on v8's Utils::ReportApiFailure (api.cc)
 * TODO: Should this really be inlined? */
static inline void ReportApiFailure(const char* location, const char* message)
{
	jscshim::Isolate * isolate = jscshim::Isolate::GetCurrent();
	v8::FatalErrorCallback fatalErrorCallback = isolate ? isolate->GetFatalErrorHandler() : nullptr;
	if (fatalErrorCallback)
	{
		fatalErrorCallback(location, message);

		/* v8 doesn't do that, and node's callback will crash the process anyway, but we'll make sure
		 * we're exiting after an "api failure", since all checks count on ApiCheck\ReportApiFailure
		 * to never return */
		CRASH();
	}
	else
	{
		WTFLogAlwaysAndCrash("\n#\n# Fatal error in %s\n# %s\n#\n\n", location, message);
	}
}

// Based on v8's Utils::ApiCheck (api.h)
static inline void ApiCheck(bool condition, const char * location, const char * message)
{
	if (!condition)
	{
		ReportApiFailure(location, message);
	}
}

}} // v8::jscshim