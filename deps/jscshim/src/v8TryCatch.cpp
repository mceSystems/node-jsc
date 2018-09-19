/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/Isolate.h"
#include "shim/Message.h"
#include "shim/helpers.h"

#include <JavaScriptCore/Exception.h>
#include <JavaScriptCore/ExceptionHelpers.h>
#include <JavaScriptCore/CatchScope.h>
#include <JavaScriptCore/JSCInlines.h>

namespace v8
{

TryCatch::TryCatch(Isolate* isolate) :
	isolate_(jscshim::V8IsolateToJscShimIsolate(isolate)),
	next_(isolate_->TopTryCatchHandler()),
	exception_(nullptr),
	shim_scopes_current_depth_(0),
	message_obj_(nullptr),
	is_verbose_(false),
	capture_message_(true),
	rethrow_(false)
{
	isolate_->RegisterTryCatchHandler(this);
}

TryCatch::~TryCatch()
{
	isolate_->UnregisterTryCatchHandler(this);
}

bool TryCatch::CanContinue() const
{
	/* v8's Isolate::PropagatePendingExceptionToExternalTryCatch (isolate.cc) sets this to false
	 * if !is_catchable_by_javascript(exception), while is_catchable_by_javascript is just
	 * "exception != heap()->termination_exception()". 
	 * TODO: Calling HasTerminated is currently OK but might break when we'll support\implement
	 * Isolate::CancelTerminateExecution (which in v8 resets the TryCatch has_terminated_, but not
	 * can_continue_). */ 
	return !HasTerminated();
}

bool TryCatch::HasTerminated() const
{
	// TODO: Needed?
	if (!HasCaught())
	{
		return false;
	}

	JSC::Exception * exception = reinterpret_cast<JSC::Exception *>(this->exception_);
	return (exception && JSC::isTerminatedExecutionException(isolate_->VM(), exception));
}

Local<Value> TryCatch::ReThrow()
{
	rethrow_ = true;
	return this->Exception();
}

Local<Value> TryCatch::Exception() const
{
	if (!HasCaught())
	{
		return Local<Value>();
	}

	JSC::Exception * exception = reinterpret_cast<JSC::Exception *>(this->exception_);
	if ((nullptr == exception) || JSC::isTerminatedExecutionException(isolate_->VM(), exception))
	{
		return Local<Value>();
	}

	return Local<Value>::New(exception->value());
}

MaybeLocal<Value> TryCatch::StackTrace(Local<Context> context) const
{
	if (!HasCaught())
	{
		return Local<Value>();
	}

	JSC::VM& vm = isolate_->VM();

	// Get the thrown exception, if we have one
	JSC::Exception * exception = reinterpret_cast<JSC::Exception *>(this->exception_);
	if ((nullptr == exception) || JSC::isTerminatedExecutionException(vm, exception))
	{
		return Local<Value>();
	}

	// We only handle "Error" objects, which (may) have "stack" property
	JSC::ErrorInstance * error = JSC::jsDynamicCast<JSC::ErrorInstance *>(vm, exception->value());
	if (nullptr == error)
	{
		return Local<Value>();
	}

	JSC::ExecState * exec = jscshim::GetExecStateForV8Context(*context);

	/* Note: Calling "error->get(exec, vm.propertyNames->stack)" fails when we're being called
	 * after an exception was thrown, since internally the macro RETURN_IF_EXCEPTION will always
	 * return an error (since we have a pending exception) */
	JSC::PropertySlot stackSlot(error, JSC::PropertySlot::InternalMethodType::Get);
	if (!error->methodTable(vm)->getOwnPropertySlot(error, exec, vm.propertyNames->stack, stackSlot))
	{
		return Local<Value>();
	}
	
	JSC::JSValue stackTrace = stackSlot.getValue(exec, vm.propertyNames->stack);
	return Local<Value>::New(stackTrace);
}

Local<v8::Message> TryCatch::Message() const
{
	return Local<v8::Message>(JSC::JSValue(message_obj_));
}

void TryCatch::Reset()
{
	isolate_->ClearException();
	exception_ = nullptr;
	message_obj_ = nullptr;
}

} // v8