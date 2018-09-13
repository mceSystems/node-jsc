/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"
#include "shim/PromiseResolver.h"

#include <JavaScriptCore/BuiltinNames.h>
#include <JavaScriptCore/JSPromise.h>
#include <JavaScriptCore/JSCInlines.h>

#define GET_JSC_THIS_PROMISE() v8::jscshim::GetJscCellFromV8<JSC::JSPromise>(this)
#define GET_JSC_THIS_PROMISE_RESOLVER() v8::jscshim::GetJscCellFromV8<v8::jscshim::PromiseResolver>(this)

namespace
{

// Copied from JSC's JSInternalPromise::then
JSC::JSValue callPromiseThen(JSC::ExecState * exec, JSC::JSPromise * promise, JSC::JSValue onFulfilled, JSC::JSValue onRejected)
{
	JSC::VM& vm = exec->vm();
	auto scope = DECLARE_THROW_SCOPE(vm);

	JSC::JSObject* function = JSC::jsCast<JSC::JSObject*>(promise->get(exec, vm.propertyNames->builtinNames().thenPublicName()));
	RETURN_IF_EXCEPTION(scope, JSC::JSValue());
	JSC::CallData callData;
	JSC::CallType callType = JSC::getCallData(function, callData);
	ASSERT(callType != JSC::CallType::None);

	JSC::MarkedArgumentBuffer arguments;
	arguments.append(onFulfilled);
	arguments.append(onRejected);
	ASSERT(!arguments.hasOverflowed());

	scope.release();
	return JSC::call(exec, function, callType, callData, promise, arguments);
}

}

namespace v8
{

MaybeLocal<Promise::Resolver> Promise::Resolver::New(Local<Context> context)
{
	JSC::ExecState * exec = jscshim::GetExecStateForV8Context(*context);

	jscshim::PromiseResolver * promiseResolver = jscshim::PromiseResolver::create(exec->vm(), 
																				  jscshim::GetGlobalObject(exec)->shimPromiseResolverStructure(), 
																				  exec);

	return Local<Promise::Resolver>(promiseResolver);
}

Local<Promise> Promise::Resolver::GetPromise()
{
	return Local<Promise>(GET_JSC_THIS_PROMISE_RESOLVER()->promise());
}

Maybe<bool> Promise::Resolver::Resolve(Local<Context> context, Local<Value> value)
{
	JSC::ExecState * exec = jscshim::GetExecStateForV8Context(*context);
	DECLARE_SHIM_EXCEPTION_SCOPE(*context);
	
	return Just(GET_JSC_THIS_PROMISE_RESOLVER()->resolve(exec, value.val_));
}

Maybe<bool> Promise::Resolver::Reject(Local<Context> context, Local<Value> value)
{
	JSC::ExecState * exec = jscshim::GetExecStateForV8Context(*context);
	DECLARE_SHIM_EXCEPTION_SCOPE(*context);

	return Just(GET_JSC_THIS_PROMISE_RESOLVER()->reject(exec, value.val_));
}

MaybeLocal<Promise> Promise::Catch(Local<Context> context, Local<Function> handler)
{
	JSC::ExecState * exec = jscshim::GetExecStateForV8Context(*context);
	DECLARE_SHIM_EXCEPTION_SCOPE(*context);

	return Local<Promise>(callPromiseThen(exec, GET_JSC_THIS_PROMISE(), JSC::jsUndefined(), handler.val_));
}

MaybeLocal<Promise> Promise::Then(Local<Context> context, Local<Function> handler)
{
	JSC::ExecState * exec = jscshim::GetExecStateForV8Context(*context);
	DECLARE_SHIM_EXCEPTION_SCOPE(*context);

	return Local<Promise>(callPromiseThen(exec, GET_JSC_THIS_PROMISE(), handler.val_, JSC::jsUndefined()));
}

bool Promise::HasHandler()
{
	JSC::JSPromise * thisPromise = GET_JSC_THIS_PROMISE();
	JSC::VM& vm = *thisPromise->vm();
	DECLARE_SHIM_EXCEPTION_SCOPE(jscshim::Isolate::GetCurrent());

	return thisPromise->isHandled(vm);
}

Local<Value> Promise::Result()
{
	JSC::JSPromise * thisPromise = GET_JSC_THIS_PROMISE();
	JSC::VM& vm = *thisPromise->vm();
	DECLARE_SHIM_EXCEPTION_SCOPE(jscshim::Isolate::GetCurrent());

	jscshim::ApiCheck(JSC::JSPromise::Status::Pending != thisPromise->status(vm), 
					  "v8_Promise_Result",
					  "Promise is still pending");

	return Local<Value>(thisPromise->result(vm));
}

Promise::PromiseState Promise::State()
{
	JSC::JSPromise * thisPromise = GET_JSC_THIS_PROMISE();
	JSC::VM& vm = *thisPromise->vm();
	DECLARE_SHIM_EXCEPTION_SCOPE(jscshim::Isolate::GetCurrent());

	JSC::JSPromise::Status state = thisPromise->status(vm);

	// TODO: It seems that we can just "return (Promise::PromiseState)((unsigned int)state - 1)"
	switch (state)
	{
	case JSC::JSPromise::Status::Pending:
		return PromiseState::kPending;

	case JSC::JSPromise::Status::Fulfilled:
		return PromiseState::kFulfilled;

	case JSC::JSPromise::Status::Rejected:
		return PromiseState::kRejected;

	default:
		// Shouldn't be here
		RELEASE_ASSERT_NOT_REACHED();
		break;
	}
}

} // v8