/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"

#include <JavaScriptCore/CallData.h>
#include <JavaScriptCore/CatchScope.h>
#include <JavaScriptCore/JSFunction.h>
#include <JavaScriptCore/JSBoundFunction.h>
#include <JavaScriptCore/JSCInlines.h>

namespace v8
{

MaybeLocal<Function> Function::New(Local<Context> context,
								   FunctionCallback callback,
								   Local<Value> data,
								   int length,
								   ConstructorBehavior behavior)
{
	Local<FunctionTemplate> functionTemplate = FunctionTemplate::New(context->GetIsolate(), callback, data, Local<Signature>(), length);

	if (ConstructorBehavior::kThrow == behavior)
	{
		functionTemplate->RemovePrototype();
	}

	return functionTemplate->GetFunction(context);
}

Local<Function> Function::New(Isolate * isolate,
							 FunctionCallback callback,
 							 Local<Value> data,
							 int length)
{
	return New(isolate->GetCurrentContext(), callback, data, length, ConstructorBehavior::kAllow)
		.FromMaybe(Local<Function>());
}

MaybeLocal<Object> Function::NewInstance(Local<Context> context, int argc, Local<Value> argv[]) const
{
	return const_cast<Function *>(this)->CallAsConstructor(context, argc, argv);
}

MaybeLocal<Object> Function::NewInstance(Local<Context> context)
{
	return NewInstance(context, 0, nullptr);
}

Local<Value> Function::Call(Local<Value> recv, int argc, Local<Value> argv[])
{
	return Call(Isolate::GetCurrent()->GetCurrentContext(), recv, argc, argv).FromMaybe(Local<Value>());
}

MaybeLocal<Value> Function::Call(Local<Context> context, Local<Value> recv, int argc, Local<Value> argv[])
{
	return CallAsFunction(context, recv, argc, argv);
}

// TODO: Should we call "JSFunction::setFunctionName" for JSFunction instances?
void Function::SetName(Local<String> name)
{
	SETUP_OBJECT_USE_IN_MEMBER(Isolate::GetCurrent()->GetCurrentContext());

	// v8's api ignores the return value
	thisObj->putDirect(vm, vm.propertyNames->name, name.val_);

	// TODO: Clear exception?
}

// TODO: Should we call "JSFunction::name" for JSFunction instances?
Local<Value> Function::GetName() const
{
	SETUP_OBJECT_USE_IN_MEMBER(Isolate::GetCurrent()->GetCurrentContext());

	JSC::JSValue name = thisObj->getDirect(vm, vm.propertyNames->name);
	return Local<Value>(name);
}

Local<Value> Function::GetInferredName() const
{
	SETUP_OBJECT_USE_IN_MEMBER(Isolate::GetCurrent()->GetCurrentContext());

	JSC::JSFunction * thisJSFunc = JSC::jsDynamicCast<JSC::JSFunction *>(vm, thisObj);
	if (thisJSFunc && !thisJSFunc->isHostFunction())
	{
		JSC::JSValue inferredName(JSC::jsString(exec, thisJSFunc->jsExecutable()->inferredName().string()));
		return Local<Value>(inferredName);
	}

	return Local<Value>();
}

Local<Value> Function::GetDebugName() const
{
	SETUP_OBJECT_USE_IN_MEMBER(Isolate::GetCurrent()->GetCurrentContext());

	JSC::JSValue debugName;
	if (thisObj->inherits(vm, JSC::JSFunction::info())) {
		debugName = JSC::jsString(exec, 
								  JSC::jsCast<JSC::JSFunction *>(thisObj)->calculatedDisplayName(vm));
	}
	else if (thisObj->inherits(vm, JSC::InternalFunction::info())) {
		debugName = JSC::jsString(exec, 
								  JSC::jsCast<JSC::InternalFunction *>(thisObj)->calculatedDisplayName(vm));
	}

	return Local<Value>(debugName);
}

Local<Value> Function::GetDisplayName() const
{
	SETUP_OBJECT_USE_IN_MEMBER(Isolate::GetCurrent()->GetCurrentContext());

	JSC::JSValue displayName = thisObj->getDirect(vm, vm.propertyNames->displayName);
	if (displayName && isJSString(displayName))
	{
		return Local<Value>(displayName);
	}

	return Local<Value>(displayName);
}

Local<Value> Function::GetBoundFunction() const
{
	SETUP_OBJECT_USE_IN_MEMBER(Isolate::GetCurrent()->GetCurrentContext());

	JSC::JSBoundFunction * thisBoundFunction = JSC::jsDynamicCast<JSC::JSBoundFunction *>(vm, thisObj);
	if (!thisBoundFunction)
	{
		return Local<Value>(JSC::jsUndefined());
	}

	JSC::JSValue targetFunction(thisBoundFunction->targetFunction());
	return Local<Value>(targetFunction);
}

} // v8