/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/Isolate.h"
#include "shim/GlobalObject.h"

#include <JavaScriptCore/JSCInlines.h>

#define TO_JSC_ISOLATE(what) reinterpret_cast<jscshim::Isolate *>(what)
#define TO_V8_ISOLATE(what) reinterpret_cast<v8::Isolate *>(what)

namespace v8
{

Isolate::DisallowJavascriptExecutionScope::DisallowJavascriptExecutionScope(Isolate * isolate, OnFailure on_failure)
{
	/* TODO: IMPLEMENT using JSC::DisallowVMReentry? but it seems to be ASSERT based,
	 * thus only works on debug builds */
}

Isolate::DisallowJavascriptExecutionScope::~DisallowJavascriptExecutionScope()
{
	// TODO: IMPLEMENT
}

Isolate * Isolate::New(const CreateParams& params)
{
	return TO_V8_ISOLATE(jscshim::Isolate::New(params));
}

Isolate * Isolate::GetCurrent()
{
	return TO_V8_ISOLATE(jscshim::Isolate::GetCurrent());
}

void Isolate::SetHostImportModuleDynamicallyCallback(HostImportModuleDynamicallyCallback callback)
{
	// TODO: IMPLEMENT
}

void Isolate::SetHostInitializeImportMetaObjectCallback(HostInitializeImportMetaObjectCallback callback)
{
	// TODO: IMPLEMENT
}

void Isolate::Dispose()
{
	TO_JSC_ISOLATE(this)->Dispose();
}

void Isolate::SetAbortOnUncaughtExceptionCallback(Isolate::AbortOnUncaughtExceptionCallback callback)
{
	TO_JSC_ISOLATE(this)->SetAbortOnUncaughtExceptionCallback(callback);
}

void Isolate::Enter()
{
	TO_JSC_ISOLATE(this)->Enter();
}

void Isolate::Exit()
{
	TO_JSC_ISOLATE(this)->Exit();
}

void Isolate::DiscardThreadSpecificMetadata()
{
	// TODO: IMPLEMENT? Does JSC have a way to do this?
}

void Isolate::SetData(uint32_t slot, void * data)
{
	TO_JSC_ISOLATE(this)->SetData(slot, data);
}

void * Isolate::GetData(uint32_t slot)
{
	return TO_JSC_ISOLATE(this)->GetData(slot);
}

uint32_t Isolate::GetNumberOfDataSlots()
{
	return jscshim::Isolate::GetNumberOfDataSlots();
}

void Isolate::SetIdle(bool is_idle)
{
	// TODO: IMPLEMENT
}

bool Isolate::InContext()
{
	return TO_JSC_ISOLATE(this)->InContext();
}

Local<Context> Isolate::GetCurrentContext()
{
	return Local<Context>::New(JSC::JSValue(TO_JSC_ISOLATE(this)->GetCurrentContext()));
}

Local<Context> Isolate::GetEnteredContext()
{
	return Local<Context>::New(JSC::JSValue(TO_JSC_ISOLATE(this)->GetEnteredContext()));
}

Local<Value> Isolate::ThrowException(Local<Value> exception)
{
	return Local<Value>(TO_JSC_ISOLATE(this)->ThrowException(exception.val_));
}

void Isolate::AddGCPrologueCallback(GCCallbackWithData callback, void* data, GCType gc_type_filter)
{
	// TODO: IMPLEMENT
}

void Isolate::AddGCPrologueCallback(GCCallback callback, GCType gc_type_filter)
{
	// TODO: IMPLEMENT
}

void Isolate::RemoveGCPrologueCallback(GCCallbackWithData, void* data)
{
	// TODO: IMPLEMENT
}

void Isolate::RemoveGCPrologueCallback(GCCallback callback)
{
	// TODO: IMPLEMENT
}

void Isolate::AddGCEpilogueCallback(GCCallbackWithData callback, void* data, GCType gc_type_filter)
{
	// TODO: IMPLEMENT
}
void Isolate::AddGCEpilogueCallback(GCCallback callback, GCType gc_type_filter)
{
	// TODO: IMPLEMENT
}

void Isolate::RemoveGCEpilogueCallback(GCCallbackWithData callback, void* data)
{
	// TODO: IMPLEMENT
}

void Isolate::RemoveGCEpilogueCallback(GCCallback callback)
{
	// TODO: IMPLEMENT
}

int64_t Isolate::AdjustAmountOfExternalAllocatedMemory(int64_t change_in_bytes)
{
	return TO_JSC_ISOLATE(this)->AdjustAmountOfExternalAllocatedMemory(change_in_bytes);
}

void Isolate::GetHeapStatistics(HeapStatistics* heap_statistics)
{
	return TO_JSC_ISOLATE(this)->GetHeapStatistics(heap_statistics);
}

bool Isolate::GetHeapSpaceStatistics(HeapSpaceStatistics* space_statistics, size_t index)
{
	return TO_JSC_ISOLATE(this)->GetHeapSpaceStatistics(space_statistics, index);
}

size_t Isolate::NumberOfHeapSpaces()
{
	return TO_JSC_ISOLATE(this)->NumberOfHeapSpaces();
}

HeapProfiler * Isolate::GetHeapProfiler()
{
	return TO_JSC_ISOLATE(this)->GetHeapProfiler();
}

CpuProfiler * Isolate::GetCpuProfiler()
{
	return TO_JSC_ISOLATE(this)->GetCpuProfiler();
}

void Isolate::SetPromiseHook(PromiseHook hook)
{
	TO_JSC_ISOLATE(this)->SetPromiseHook(hook);
}

void Isolate::SetPromiseRejectCallback(PromiseRejectCallback callback)
{
	TO_JSC_ISOLATE(this)->SetPromiseRejectCallback(callback);
}

void Isolate::RunMicrotasks()
{
	TO_JSC_ISOLATE(this)->RunMicrotasks();
}

void Isolate::EnqueueMicrotask(Local<Function> microtask)
{
	TO_JSC_ISOLATE(this)->EnqueueMicrotask(microtask);
}

void Isolate::EnqueueMicrotask(MicrotaskCallback callback, void* data)
{
	TO_JSC_ISOLATE(this)->EnqueueMicrotask(callback, data);
}

void Isolate::SetMicrotasksPolicy(MicrotasksPolicy policy)
{
	TO_JSC_ISOLATE(this)->SetMicrotasksPolicy(policy);
}

void Isolate::SetAutorunMicrotasks(bool autorun)
{
	SetMicrotasksPolicy(autorun ? MicrotasksPolicy::kAuto : MicrotasksPolicy::kExplicit);
}

void Isolate::TerminateExecution()
{
	TO_JSC_ISOLATE(this)->TerminateExecution();
}

void Isolate::CancelTerminateExecution()
{
	TO_JSC_ISOLATE(this)->CancelTerminateExecution();
}

void Isolate::RequestInterrupt(InterruptCallback callback, void * data)
{
	TO_JSC_ISOLATE(this)->CancelTerminateExecution();
}

bool Isolate::AddMessageListener(MessageCallback that, Local<Value> data)
{
	return TO_JSC_ISOLATE(this)->AddMessageListener(that, data.val_);
}

void Isolate::RemoveMessageListeners(MessageCallback that)
{
	return TO_JSC_ISOLATE(this)->RemoveMessageListeners(that);
}

void Isolate::SetCaptureStackTraceForUncaughtExceptions(bool capture,
														int frame_limit,
														StackTrace::StackTraceOptions options)
{
	return TO_JSC_ISOLATE(this)->SetCaptureStackTraceForUncaughtExceptions(capture, frame_limit, options);
}

void Isolate::SetFatalErrorHandler(FatalErrorCallback that)
{
	TO_JSC_ISOLATE(this)->SetFatalErrorHandler(that);
}

void Isolate::SetAllowWasmCodeGenerationCallback(AllowWasmCodeGenerationCallback callback)
{
	// TODO: IMPLEMENT
}

void Isolate::SetCounterFunction(CounterLookupCallback callback)
{
	// TODO: IMPLEMENT
}

void Isolate::SetCreateHistogramFunction(CreateHistogramCallback callback)
{
	// TODO: IMPLEMENT
}

void Isolate::SetAddHistogramSampleFunction(AddHistogramSampleCallback callback)
{
	// TODO: IMPLEMENT
}

bool Isolate::IdleNotificationDeadline(double deadline_in_seconds)
{
	// TODO: IMPLEMENT
	return true;
}

void Isolate::LowMemoryNotification()
{
	TO_JSC_ISOLATE(this)->LowMemoryNotification();
}

int Isolate::ContextDisposedNotification(bool dependant_context)
{
	// TODO: IMPLEMENT

	/* From v8's documentation: "Returns the number of context disposals - including this one - 
	 * since the last time V8 had a chance to clean up". */
	return 1;
}

} // v8