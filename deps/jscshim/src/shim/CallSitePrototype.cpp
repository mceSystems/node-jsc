/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "CallSitePrototype.h"
#include "CallSite.h"
#include "helpers.h"

#include <JavaScriptCore/JSGlobalObject.h>
#include <JavaScriptCore/Error.h>
#include <JavaScriptCore/CodeBlock.h>
#include <JavaScriptCore/Operations.h>
#include <JavaScriptCore/JSCInlines.h>

// TODO: This needs testing, as test-api won't cover it

namespace v8 { namespace jscshim
{

ALWAYS_INLINE static CallSite * getCallSite(JSC::ExecState * exec, JSC::JSValue thisValue)
{
	JSC::VM& vm = exec->vm();
	auto scope = DECLARE_THROW_SCOPE(vm);

	if (UNLIKELY(!thisValue.isCell())) {
		JSC::throwVMError(exec, scope, createNotAnObjectError(exec, thisValue));
		return nullptr;
	}

	if (LIKELY(thisValue.asCell()->inherits(vm, CallSite::info()))) {
		return JSC::jsCast<CallSite *>(thisValue);
	}
		
	throwTypeError(exec, scope, "CallSite operation called on non-CallSite object"_s);
	return nullptr;
}

#define ENTER_PROTO_FUNC() JSC::VM& vm = exec->vm(); \
						   auto scope = DECLARE_THROW_SCOPE(vm); \
						   \
						   CallSite * callSite = getCallSite(exec, exec->thisValue()); \
						   if (!callSite) \
						   { \
								return JSC::JSValue::encode(JSC::jsUndefined()); \
						   }

static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncGetThis(JSC::ExecState * exec);
static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncGetTypeName(JSC::ExecState * exec);
static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncGetFunction(JSC::ExecState * exec);
static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncGetFunctionName(JSC::ExecState * exec);
static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncGetMethodName(JSC::ExecState * exec);
static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncGetFileName(JSC::ExecState * exec);
static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncGetLineNumber(JSC::ExecState * exec);
static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncGetColumnNumber(JSC::ExecState * exec);
static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncGetEvalOrigin(JSC::ExecState * exec);
static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncIsToplevel(JSC::ExecState * exec);
static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncIsEval(JSC::ExecState * exec);
static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncIsNative(JSC::ExecState * exec);
static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncIsConstructor(JSC::ExecState * exec);

const JSC::ClassInfo CallSitePrototype::s_info = { "CallSite", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(CallSitePrototype) };

// TODO: Implement toString
void CallSitePrototype::finishCreation(JSC::VM& vm, JSC::JSGlobalObject * globalObject)
{
	Base::finishCreation(vm);
	ASSERT(inherits(vm, info()));
	didBecomePrototype();

	putDirectWithoutTransition(vm, vm.propertyNames->toStringTagSymbol, jsString(&vm, "CallSite"), PropertyAttribute::DontEnum | PropertyAttribute::ReadOnly);

	JSC_NATIVE_INTRINSIC_FUNCTION_WITHOUT_TRANSITION("getThis", callSiteProtoFuncGetThis, static_cast<unsigned>(PropertyAttribute::DontEnum), 0, JSC::NoIntrinsic);
	JSC_NATIVE_INTRINSIC_FUNCTION_WITHOUT_TRANSITION("getTypeName", callSiteProtoFuncGetTypeName, static_cast<unsigned>(PropertyAttribute::DontEnum), 0, JSC::NoIntrinsic);
	JSC_NATIVE_INTRINSIC_FUNCTION_WITHOUT_TRANSITION("getFunction", callSiteProtoFuncGetFunction, static_cast<unsigned>(PropertyAttribute::DontEnum), 0, JSC::NoIntrinsic);
	JSC_NATIVE_INTRINSIC_FUNCTION_WITHOUT_TRANSITION("getFunctionName", callSiteProtoFuncGetFunctionName, static_cast<unsigned>(PropertyAttribute::DontEnum), 0, JSC::NoIntrinsic);
	JSC_NATIVE_INTRINSIC_FUNCTION_WITHOUT_TRANSITION("getMethodName", callSiteProtoFuncGetMethodName, static_cast<unsigned>(PropertyAttribute::DontEnum), 0, JSC::NoIntrinsic);
	JSC_NATIVE_INTRINSIC_FUNCTION_WITHOUT_TRANSITION("getFileName", callSiteProtoFuncGetFileName, static_cast<unsigned>(PropertyAttribute::DontEnum), 0, JSC::NoIntrinsic);
	JSC_NATIVE_INTRINSIC_FUNCTION_WITHOUT_TRANSITION("getLineNumber", callSiteProtoFuncGetLineNumber, static_cast<unsigned>(PropertyAttribute::DontEnum), 0, JSC::NoIntrinsic);
	JSC_NATIVE_INTRINSIC_FUNCTION_WITHOUT_TRANSITION("getColumnNumber", callSiteProtoFuncGetColumnNumber, static_cast<unsigned>(PropertyAttribute::DontEnum), 0, JSC::NoIntrinsic);
	JSC_NATIVE_INTRINSIC_FUNCTION_WITHOUT_TRANSITION("getEvalOrigin", callSiteProtoFuncGetEvalOrigin, static_cast<unsigned>(PropertyAttribute::DontEnum), 0, JSC::NoIntrinsic);
	JSC_NATIVE_INTRINSIC_FUNCTION_WITHOUT_TRANSITION("isToplevel", callSiteProtoFuncIsToplevel, static_cast<unsigned>(PropertyAttribute::DontEnum), 0, JSC::NoIntrinsic);
	JSC_NATIVE_INTRINSIC_FUNCTION_WITHOUT_TRANSITION("isEval", callSiteProtoFuncIsEval, static_cast<unsigned>(PropertyAttribute::DontEnum), 0, JSC::NoIntrinsic);
	JSC_NATIVE_INTRINSIC_FUNCTION_WITHOUT_TRANSITION("isNative", callSiteProtoFuncIsNative, static_cast<unsigned>(PropertyAttribute::DontEnum), 0, JSC::NoIntrinsic);
	JSC_NATIVE_INTRINSIC_FUNCTION_WITHOUT_TRANSITION("isConstructor", callSiteProtoFuncIsConstructor, static_cast<unsigned>(PropertyAttribute::DontEnum), 0, JSC::NoIntrinsic);
}

static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncGetThis(JSC::ExecState * exec)
{
	ENTER_PROTO_FUNC();
	return JSC::JSValue::encode(callSite->thisValue());
}

static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncGetTypeName(JSC::ExecState * exec)
{
	ENTER_PROTO_FUNC();
	return JSC::JSValue::encode(JSC::jsTypeStringForValue(exec, callSite->thisValue()));
}

static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncGetFunction(JSC::ExecState * exec)
{
	ENTER_PROTO_FUNC();
	return JSC::JSValue::encode(callSite->function());
}

static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncGetFunctionName(JSC::ExecState * exec)
{
	ENTER_PROTO_FUNC();
	return JSC::JSValue::encode(callSite->functionName());
}

// TODO
static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncGetMethodName(JSC::ExecState * exec)
{
	return callSiteProtoFuncGetFunctionName(exec);
}

static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncGetFileName(JSC::ExecState * exec)
{
	ENTER_PROTO_FUNC();
	return JSC::JSValue::encode(callSite->sourceURL());
}

static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncGetLineNumber(JSC::ExecState * exec)
{
	ENTER_PROTO_FUNC();
	return JSC::JSValue::encode(callSite->lineNumber());
}

static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncGetColumnNumber(JSC::ExecState * exec)
{
	ENTER_PROTO_FUNC();
	return JSC::JSValue::encode(callSite->columnNumber());
}

// TODO
static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncGetEvalOrigin(JSC::ExecState * exec)
{
	return JSC::JSValue::encode(JSC::jsUndefined());
}

static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncIsToplevel(JSC::ExecState * exec)
{
	ENTER_PROTO_FUNC();

	JSC::JSValue thisValue = callSite->thisValue();

	// This is what v8 does (JSStackFrame::IsToplevel in messages.cc):

	if (thisValue.isUndefinedOrNull())
	{
		return JSC::JSValue::encode(JSC::jsBoolean(true));
	}

	JSC::JSObject * thisObject = thisValue.getObject();
	if (thisObject && jscshim::isGlobalObjectOrGlobalProxy(thisObject))
	{
		return JSC::JSValue::encode(JSC::jsBoolean(true));
	}

	return JSC::JSValue::encode(JSC::jsBoolean(false));
}

static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncIsEval(JSC::ExecState * exec)
{
	ENTER_PROTO_FUNC();

	bool isEval = callSite->isEval();
	return JSC::JSValue::encode(JSC::jsBoolean(isEval));
}

static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncIsNative(JSC::ExecState * exec)
{
	ENTER_PROTO_FUNC();

	// TODO
	return JSC::JSValue::encode(JSC::jsBoolean(false));
}

static JSC::EncodedJSValue JSC_HOST_CALL callSiteProtoFuncIsConstructor(JSC::ExecState * exec)
{
	ENTER_PROTO_FUNC();

	bool isConstructor = callSite->isConstructor();
	return JSC::JSValue::encode(JSC::jsBoolean(isConstructor));
}

}} // v8::jscshim