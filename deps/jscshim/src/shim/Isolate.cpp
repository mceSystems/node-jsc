/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "Isolate.h"

#include "GlobalObject.h"
#include "Message.h"
#include "Function.h"
#include "FunctionTemplate.h"
#include "ObjectTemplate.h"
#include "PromiseResolver.h"

#include <JavaScriptCore/InitializeThreading.h>
#include <JavaScriptCore/VM.h>
#include <JavaScriptCore/VMEntryScope.h>
#include <JavaScriptCore/Protect.h>
#include <JavaScriptCore/Microtask.h>
#include <JavaScriptCore/ThrowScope.h>
#include <JavaScriptCore/JSDestructibleObjectHeapCellType.h>
#include <JavaScriptCore/JSCInlines.h>
#include <cassert>

namespace v8 { namespace jscshim
{

thread_local std::stack<Isolate *> Isolate::s_isolateStack;

#ifdef DEBUG
std::atomic<size_t> Isolate::s_nonDisposedIsolates = 0;
#endif

Isolate::Isolate(JSC::VM * vm, v8::ArrayBuffer::Allocator * arrayBufferAllocator) :
	m_vm(vm),
	m_functionSpace ISO_SUBSPACE_INIT(vm->heap, vm->destructibleObjectHeapCellType.get(), jscshim::Function),
	m_functionTemplateSpace ISO_SUBSPACE_INIT(vm->heap, vm->destructibleObjectHeapCellType.get(), jscshim::FunctionTemplate),
	m_objectTemplateSpace ISO_SUBSPACE_INIT(vm->heap, vm->destructibleObjectHeapCellType.get(), jscshim::ObjectTemplate),
	m_promiseResolverSpace ISO_SUBSPACE_INIT(vm->heap, vm->destructibleObjectHeapCellType.get(), jscshim::PromiseResolver),
	m_currentContext(nullptr),
	m_defaultGlobal(nullptr),
	m_embeddedData{ 0 },
	m_apiPrivateSymbolRegistry(true),
	m_arrayBufferAllocator(arrayBufferAllocator),
	m_promiseRejectCallback(nullptr),
	m_topTryCatchHandler(nullptr),
	m_fatalErrorCallback(nullptr),
	m_pendingMessage(nullptr),
	m_shouldCaptureStackTraceForUncaughtExceptions(false), // This is v8's default
	m_uncaughtExceptionsStaclTraceFrameLimit(0),
	m_isHandlingThrownException(false),
	m_shimBaseScopesDepth(0),
	m_microtasksPolicy(v8::MicrotasksPolicy::kAuto)
{
#ifdef DEBUG
	s_nonDisposedIsolates++;
#endif
}

Isolate::~Isolate()
{
	m_disposing = true;
	
	JSC::gcUnprotectNullTolerant(m_currentContext);
	JSC::gcUnprotectNullTolerant(m_defaultGlobal);
	while (!m_enteredContexts.empty())
	{
		LeaveEnteredContext();
	}
	
	JSC::gcUnprotectNullTolerant(m_pendingMessage);

	/* This is hacky, but we need to unlock the vm and lock it again, because:
	 * - Locking and unlocking the vm's api lock has a few side effects (see JSLock::didAcquireLock
	 *   and JSLock::willReleaseLock in JSC's runtime/JSLock.cpp). Thus, in order
	 *   to revert the side effects made by acquiring the lock in our constructor, 
	 *   we must unlock it before releasing the VM. For example, JSLock::didAcquireLock
	 *   will set the current thread's atomic string table to the VM's table, which
	 *   is reverted by JSLock::willReleaseLock. Failing to restore the previous atomic
	 *   string table will leave the thread's atomic string table pointing to the already
	 *   released table (which caused jscshim_tests to crash after running IsolateNewDispose).
	 * - The VM's destructor has an ASSERT(currentThreadIsHoldingAPILock()), so we need to acquire 
	 *   the lock again.
	 *
	 * TODO: Properly lock\unlock the vm's api lock on api calls?
	 */
	m_vm->apiLock().unlock();
	m_vm->apiLock().lock();

	m_vm->deref();
	m_vm = nullptr;

#ifdef DEBUG
	s_nonDisposedIsolates--;
#endif
}

void Isolate::EnterContext(GlobalObject * context)
{
	JSC::gcProtect(context);
	m_enteredContexts.push(context);
}

void Isolate::LeaveEnteredContext()
{
	JSC::gcUnprotect(m_enteredContexts.top());
	m_enteredContexts.pop();
}

/* Based on JSContextGroupCreate
 *
 * TODO: Support custom allocators. 
 * JSC doesn't seem to support custom allocators. In node, a custom allocator is
 * used in order to control whether to zero out the memory or not (calloc or malloc), 
 * which is an optimization used by node's Buffer class when allocating ArrayBuffers.
 * In JSC, the internal ArrayBuffer can control weather to iniialize allocated memory
 * or not using ArrayBufferContents::InitializationPolicy flags. ArrayBufferContents::tryAllocate,
 * in turn, will use WTF::tryFastMalloc\WTF::tryFastCalloc to allocate memory. WTF uses
 * either bmalloc or the system allocators.
 * So, it seems like we have two options:
 * - Modify JSC to allow setting a custom allocator per VM (which corresponds to a v8 Isolate).
 * - Modify node.js (buffer class?) to use a custom ArrayBuffer allocation function\constructor.
 * 	 This will probably require minor changes in JSC's ArrayBuffer to expose the underlying
 * 	 "createUnininitialized". 
 *
 * TODO: solve the locking issues properly (we currently won't release the lock until 
 *       the Isolate is detroyed).
 */
Isolate * Isolate::New(const v8::Isolate::CreateParams& params)
{
	JSC::initializeThreading();

	/* Create a new VM and grab it's api lock.
	 * Grabbing to api lock here is ugly, but it makes our constructor easier to implement, 
	 * since we create new JSC::IsoSubspaces, which require us to hold the lock. */
	JSC::VM * vm = &JSC::VM::createContextGroup().leakRef();
	vm->apiLock().lock();

	return new Isolate(vm, params.array_buffer_allocator);
}

Isolate * Isolate::GetCurrent()
{
	return s_isolateStack.top();
}

void Isolate::Dispose()
{
	delete this;
}

void Isolate::SetAbortOnUncaughtExceptionCallback(v8::Isolate::AbortOnUncaughtExceptionCallback callback)
{
	// TODO: IMPLEMENT
}

void Isolate::Enter()
{
	// TODO: Check thread id when re-entring?
	s_isolateStack.push(this);
}

void Isolate::Exit()
{
	assert(s_isolateStack.size() > 0);
	assert(s_isolateStack.top() == this);

	s_isolateStack.pop();
}

void Isolate::SetData(uint32_t slot, void * data)
{
	// TODO: Crash on release too?
	assert(slot < ISOLATE_DATA_SLOTS);

	if (slot < ISOLATE_DATA_SLOTS)
	{
		m_embeddedData[slot] = data;
	}
}

void * Isolate::GetData(uint32_t slot)
{
	// TODO: Crash on release too?
	assert(slot < ISOLATE_DATA_SLOTS);
	return (slot < ISOLATE_DATA_SLOTS) ? m_embeddedData[slot] : nullptr;
}

bool Isolate::InContext()
{
	return !!m_currentContext;
}

void Isolate::SetCurrentContext(GlobalObject * context, bool savePreviousContext)
{
	RELEASE_ASSERT(context);

	/* Note that we'll save m_currentContext even if it's null, since the caller might
	 * not know if it's null or not, and always call RestoreCurrentContextFromSaved.
	 * Theoretically it should only happen for the first SetCurrentContext call, 
	 * thus RestoreCurrentContextFromSaved could have chekced if there are items
	 * in the saved contexts stack. But, if someone calls SetCurrentContext with
	 * a null context for some reason, while there's already a saved context in the 
	 * saved contexts stack, RestoreCurrentContextFromSaved will pop the wrong context. */
	if (savePreviousContext)
	{
		m_savedContexts.push(m_currentContext);
	}
	else
	{
		JSC::gcUnprotectNullTolerant(m_currentContext);
	}

	/* Note that in the case where context == m_currentContext we still want to gcProtect it
	 * for consistency, since RestoreCurrentContextFromSaved will call gcUnprotect */
	JSC::gcProtect(context);
	m_currentContext = context;
}

jscshim::GlobalObject * Isolate::GetCurrentGlobalOrDefault()
{
	if (m_currentContext)
	{
		return m_currentContext;
	}

	if (!m_enteredContexts.empty())
	{
		return m_enteredContexts.top();
	}

	/* If we don't have a context - create a new "default" global. This is done to support working
	 * with an isolate without a context (for example, for creating an global object template).
	 * This is not ideal, as we're wasting memory for a "temp" global. But since node currently doesn't
	 * do this, this is only here for v8's UT, so it's fine for now. */
	if (nullptr == m_defaultGlobal)
	{
		// Based on JSGlobalContextCreateInGroup
		JSC::initializeThreading();

		JSC::JSLockHolder locker(m_vm);
		m_defaultGlobal = jscshim::GlobalObject::create(*m_vm,
														jscshim::GlobalObject::createStructure(*m_vm, JSC::jsNull()),
														this,
														0);
		JSC::gcProtect(m_defaultGlobal);
	}

	return m_defaultGlobal;
}

JSC::JSValue Isolate::ThrowException(const JSC::JSValue& exception)
{
	// Don't allow new exceptions while we're calling message handlers (as v8 doesn't)
	if (!m_isHandlingThrownException)
	{
		auto throwScope = DECLARE_THROW_SCOPE(*m_vm);

		JSC::JSValue thrownValue = exception.isEmpty() ? JSC::jsUndefined() : exception;

		ASSERT(m_currentContext);
		throwScope.throwException(m_currentContext->globalExec(), thrownValue);

		PropagateThrownExceptionToAPI(throwScope.exception());
	}

	// That's what v8 does
	return JSC::jsUndefined();
}

// Based on v8::internal::Isolate's Throw and PropagatePendingExceptionToExternalTryCatch
void Isolate::PropagateThrownExceptionToAPI(JSC::Exception * exception)
{
	ASSERT(exception);

	/* Prevent recursive calls (If Message::create and\or a message handler use a ShimExceptionScope\SuspendRestoreShimExceptionScope,
	 * we might get called from UnregisterShimExceptionScope during it's destruction) */
	if (m_isHandlingThrownException)
	{
		return;
	}

	jscshim::GlobalObject * global = GetCurrentGlobalOrDefault();
	JSC::ExecState * exec = global->globalExec();

	/* Don't handle a  "terminated execution exception" (From ScriptFunctionCall.cpp: 
	 * "Do not treat a terminated execution exception as having an exception.")*/
	if (JSC::isTerminatedExecutionException(*m_vm, exception))
	{
		return;
	}

	unsigned int currentScopesDepths = m_topTryCatchHandler ? m_topTryCatchHandler->shim_scopes_current_depth_ : m_shimBaseScopesDepth;
	if (currentScopesDepths)
	{
		return;
	}

	if (m_topTryCatchHandler)
	{
		// TryCatch handlers are always stack allocated so no need to gcProtect the exception
		m_topTryCatchHandler->exception_ = exception;

		DECLARE_CATCH_SCOPE(*m_vm).clearException();
	}

	// Reset our pending message, if we have one
	if (m_pendingMessage)
	{
		JSC::gcUnprotect(m_pendingMessage);
		m_pendingMessage = nullptr;
	}
	
	bool requiresMessage = (nullptr == m_topTryCatchHandler) ||
						   m_topTryCatchHandler->is_verbose_ ||
						   m_topTryCatchHandler->capture_message_;
	if (requiresMessage)
	{
		// Prevent recursive calls (see the above note)
		m_isHandlingThrownException = true;

		jscshim::Message * message = jscshim::Message::create(exec, 
															  global->shimMessageStructure(), 
															  exception, 
															  m_shouldCaptureStackTraceForUncaughtExceptions);

		ReportMessageToListenersIfNeeded(message, exception);
		if (m_topTryCatchHandler && m_topTryCatchHandler->capture_message_)
		{
			// Note that TryCatch instances are always stack allocated, so no need to gcProtect them
			m_topTryCatchHandler->message_obj_ = message;
		}

		m_isHandlingThrownException = false;
	}
}

void Isolate::ClearException()
{
	// Clear JSC's exception
	 DECLARE_CATCH_SCOPE(*m_vm).clearException();

	if (m_pendingMessage)
	{
		JSC::gcUnprotect(m_pendingMessage);
		m_pendingMessage = nullptr;
	}
}

jscshim::Message * Isolate::CreateMessage(const JSC::JSValue& thrownValue)
{
	jscshim::GlobalObject * global = GetCurrentGlobalOrDefault();
	JSC::ExecState * exec = global->globalExec();

	JSCStackTrace exceptionStackTrace = JSCStackTrace::getStackTraceForThrownValue(*m_vm, thrownValue);
	if (exceptionStackTrace.isEmpty())
	{
		return jscshim::Message::createWithCurrentStackTrace(exec, 
															 global->shimMessageStructure(), 
															 thrownValue, 
															 m_shouldCaptureStackTraceForUncaughtExceptions, 
															 m_uncaughtExceptionsStaclTraceFrameLimit);	
	}

	// TODO: Handle the stack trace limit here (m_uncaughtExceptionsStaclTraceFrameLimit)
	return jscshim::Message::create(exec, 
									global->shimMessageStructure(), 
									thrownValue, 
									exceptionStackTrace, 
									m_shouldCaptureStackTraceForUncaughtExceptions);

	
}

void Isolate::ReportMessageToListenersIfNeeded(jscshim::Message * message, JSC::Exception * exception)
{
	if (m_messageListeners.isEmpty() || (m_topTryCatchHandler && !m_topTryCatchHandler->is_verbose_))
	{
		return;
	}

	Local<v8::Message> messageHandle(message);
	for (const auto& listener : m_messageListeners)
	{
		/* According to v8's docs, if "data" wasn't passed to AddMessageListener,
		 * the exception object will be passed to the callback instead */
		JSC::JSValue data = listener.data ? listener.data : exception->value();
		listener.callback(messageHandle, Local<Value>(data));
	}
}

void Isolate::RegisterTryCatchHandler(v8::TryCatch * handler)
{
	m_topTryCatchHandler = handler;
	if (m_pendingMessage && handler->capture_message_)
	{
		JSC::gcUnprotect(m_pendingMessage);
		handler->message_obj_ = m_pendingMessage;
	}
}

void Isolate::UnregisterTryCatchHandler(v8::TryCatch * handler)
{
	ASSERT(m_topTryCatchHandler == handler);

	v8::TryCatch * nextHandler = handler->next_;

	if (handler->exception_ && handler->rethrow_)
	{
		unsigned int nextScopesDepths = nextHandler ? nextHandler->shim_scopes_current_depth_ : m_shimBaseScopesDepth;
		if (nextScopesDepths > 0)
		{
			auto throwScope = DECLARE_THROW_SCOPE(*m_vm);
			throwScope.throwException(GetCurrentContext()->globalExec(), reinterpret_cast<JSC::Exception *>(handler->exception_));
		}
		else
		{
			nextHandler->exception_ = handler->exception_;
		}
	}

	bool nextHandlerCapturesMessage = nextHandler && nextHandler->capture_message_;
	if (handler->rethrow_ && handler->message_obj_)
	{
		/* The current handler has a message object and it is rethrowing it,
		 * so we'll either pass it on to the next handler, or store it (until
		 * we could either give it to another handler or dismiss it). */
		if (nextHandlerCapturesMessage)
		{
			nextHandler->message_obj_ = handler->message_obj_;
		}
		else
		{
			m_pendingMessage = reinterpret_cast<jscshim::Message *>(handler->message_obj_);
			JSC::gcProtect(m_pendingMessage);
		}
	}
	// TODO: Is this necessary?
	else if (m_pendingMessage && nextHandlerCapturesMessage)
	{
		/* Since handlers are always stack allocated, we can gcUnprotect messages we're passing to them 
		 * (the GC will see it on the stack) */
		handler->message_obj_ = m_pendingMessage;
		JSC::gcUnprotect(m_pendingMessage);
		m_pendingMessage = nullptr;
	}
	
	m_topTryCatchHandler = nextHandler;
}

// TODO: Lock
bool Isolate::AddMessageListener(MessageCallback callback, JSC::JSValue data)
{
	jscshim::Heap::ProtectValue(data);
	m_messageListeners.append({ callback, data });
	return true;
}

// TODO: Lock
void Isolate::RemoveMessageListeners(MessageCallback callback)
{
	m_messageListeners.removeAllMatching([callback](const MessageListener& listener) -> bool {
		if (callback != listener.callback)
		{
			return false;
		}
		
		jscshim::Heap::UnprotectValue(listener.data);
		return true;
	});
}

// TODO: Handle options
void Isolate::SetCaptureStackTraceForUncaughtExceptions(bool capture,
														int frameLimit,
														v8::StackTrace::StackTraceOptions options)
{
	m_shouldCaptureStackTraceForUncaughtExceptions = capture;
	m_uncaughtExceptionsStaclTraceFrameLimit = static_cast<size_t>(std::max(frameLimit, 0));
}

int64_t Isolate::AdjustAmountOfExternalAllocatedMemory(int64_t change_in_bytes)
{
	// TODO: IMPLEMENT
	return 0;
}

void Isolate::GetHeapStatistics(HeapStatistics* heap_statistics)
{
	// TODO: IMPLEMENT
}

bool Isolate::GetHeapSpaceStatistics(HeapSpaceStatistics* space_statistics, size_t index)
{
	// TODO: IMPLEMENT
	return false;
}

size_t Isolate::NumberOfHeapSpaces()
{
	// TODO: IMPLEMENT
	return 0;
}

HeapProfiler * Isolate::GetHeapProfiler()
{
	// TODO: IMPLEMENT
	return nullptr;
}

CpuProfiler * Isolate::GetCpuProfiler()
{
	// TODO: IMPLEMENT
	return nullptr;
}

void Isolate::SetPromiseHook(PromiseHook hook)
{
	// TODO: IMPLEMENT
}

void Isolate::SetPromiseRejectCallback(v8::PromiseRejectCallback callback)
{
	m_promiseRejectCallback = callback;
}

void Isolate::RunMicrotasks()
{
	m_vm->drainMicrotasks();
}

void Isolate::EnqueueMicrotask(Local<v8::Function> microtask)
{
	// TODO: Use the user provided v8 platform
}

void Isolate::EnqueueMicrotask(MicrotaskCallback microtask, void* data)
{
	// TODO: Use the user provided v8 platform
}

void Isolate::SetMicrotasksPolicy(MicrotasksPolicy policy)
{
	m_microtasksPolicy = policy;
}

void Isolate::TerminateExecution()
{
	// TODO: IMPLEMENT
}

void Isolate::CancelTerminateExecution()
{
	// TODO: IMPLEMENT
}

void Isolate::RequestInterrupt(InterruptCallback callback, void* data)
{
	// TODO: IMPLEMENT
}

void Isolate::SetFatalErrorHandler(FatalErrorCallback that)
{
	// TODO: IMPLEMENT
}

void Isolate::LowMemoryNotification()
{
	// TODO: IMPLEMENT
}

}} // v8::jscshim
