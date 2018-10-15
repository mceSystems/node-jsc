/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include "v8.h"
#include "GlobalObject.h"

#include <JavaScriptCore/VM.h>
#include <JavaScriptCore/JSBase.h>
#include <wtf/text/SymbolRegistry.h>

#include <stdint.h>
#include <stack>

namespace v8 { namespace jscshim
{
class GlobalObject;

class Isolate : public JSC::VM::ClientData {
public:
	class CurrentContextScope
	{
	private:
		jscshim::Isolate * m_isolate;
		bool m_shouldRestoreContext;

	public:
		CurrentContextScope(GlobalObject * context)
		{
			m_isolate = context->isolate();

			if (m_isolate->GetCurrentContext() != context)
			{
				m_isolate->SetCurrentContext(context, true);
				m_shouldRestoreContext = true;
			}
			else
			{
				m_shouldRestoreContext = false;
			}
		}

		~CurrentContextScope()
		{
			if (m_shouldRestoreContext)
			{
				m_isolate->RestoreCurrentContextFromSaved();
			}
		}
	};

private:
	// Based on v8's Internals::kNumIsolateDataSlots
	static constexpr uint32_t ISOLATE_DATA_SLOTS = 4;

private:
	RefPtr<JSC::VM> m_vm;
	JSC::IsoSubspace m_functionSpace;
	JSC::IsoSubspace m_functionTemplateSpace;
	JSC::IsoSubspace m_objectTemplateSpace;
	JSC::IsoSubspace m_promiseResolverSpace;

	// TODO: Should this be thread_local?
	std::stack<GlobalObject *> m_enteredContexts;
	std::stack<GlobalObject *> m_savedContexts;
	GlobalObject * m_currentContext;
	GlobalObject * m_defaultGlobal;

	void * m_embeddedData[ISOLATE_DATA_SLOTS];

	static thread_local std::stack<Isolate *> s_isolateStack;

	WTF::SymbolRegistry m_apiSymbolRegistry;
	WTF::SymbolRegistry m_apiPrivateSymbolRegistry;

	v8::ArrayBuffer::Allocator * m_arrayBufferAllocator;

	v8::PromiseRejectCallback m_promiseRejectCallback;

	// TODO: This should be per thread
	v8::TryCatch * m_topTryCatchHandler;

	// TODO: Message level
	struct MessageListener
	{
		MessageCallback callback;
		JSC::JSValue data;
	};
	WTF::Vector<MessageListener> m_messageListeners;

	FatalErrorCallback m_fatalErrorCallback;
	jscshim::Message * m_pendingMessage;
	bool m_shouldCaptureStackTraceForUncaughtExceptions;
	size_t m_uncaughtExceptionsStaclTraceFrameLimit;
	bool m_isHandlingThrownException;
	unsigned int m_shimBaseScopesDepth;

	MicrotasksPolicy m_microtasksPolicy;

	bool m_disposing;

#ifdef DEBUG
	// Used for testing
	static std::atomic<size_t> s_nonDisposedIsolates;
#endif

public:
	// JSC related
	JSC::VM& VM() const { return *m_vm; }
	void EnterContext(GlobalObject * context);
	void LeaveEnteredContext();

	inline JSC::IsoSubspace * FunctionSpace() { return &m_functionSpace; }
	inline JSC::IsoSubspace * FunctionTemplateSpace() { return &m_functionTemplateSpace; }
	inline JSC::IsoSubspace * ObjectTemplateSpace() { return &m_objectTemplateSpace; }
	inline JSC::IsoSubspace * PromiseResolverSpace() { return &m_promiseResolverSpace; }

	v8::ArrayBuffer::Allocator * ArrayBufferAllocator() const { return m_arrayBufferAllocator; }

	// v8 interface
	static Isolate * New(const v8::Isolate::CreateParams& params);

	static Isolate * GetCurrent();

	void Dispose();

	inline bool IsDisposing() const { return m_disposing; }

	void SetAbortOnUncaughtExceptionCallback(v8::Isolate::AbortOnUncaughtExceptionCallback callback);

	void Enter();

	void Exit();

	void SetData(uint32_t slot, void * data);

	void * GetData(uint32_t slot);

	static uint32_t GetNumberOfDataSlots() { return ISOLATE_DATA_SLOTS; }

	bool InContext();

	inline GlobalObject * GetEnteredContext() { return m_enteredContexts.empty() ? nullptr : m_enteredContexts.top(); }

	inline GlobalObject * GetCurrentContext() { return m_currentContext; }

	void SetCurrentContext(GlobalObject * context, bool savePreviousContext = true);

	void RestoreCurrentContextFromSaved()
	{
		RELEASE_ASSERT(!m_savedContexts.empty());
		
		JSC::gcUnprotect(m_currentContext);

		m_currentContext = m_savedContexts.top();
		m_savedContexts.pop();
	}

	/* Similar to GetCurrentContext, but will create a new global object if we don't have any contexts.
	 * This is here to support creating contexts with a global template, which are currently not used by node. */
	GlobalObject * GetCurrentGlobalOrDefault();

	JSC::JSValue ThrowException(const JSC::JSValue& exception);
	void PropagateThrownExceptionToAPI(JSC::Exception * currentException);
	void ClearException();
	jscshim::Message * CreateMessage(const JSC::JSValue& thrownValue);

	void RegisterTryCatchHandler(v8::TryCatch * handler);
	void UnregisterTryCatchHandler(v8::TryCatch * handler);
	v8::TryCatch * TopTryCatchHandler() const { return m_topTryCatchHandler; }

	bool AddMessageListener(MessageCallback callback, JSC::JSValue data);
	void RemoveMessageListeners(MessageCallback callback);

	void SetCaptureStackTraceForUncaughtExceptions(bool capture,
												   int frameLimit,
												   v8::StackTrace::StackTraceOptions options);

	void SetFatalErrorHandler(FatalErrorCallback that);
	FatalErrorCallback GetFatalErrorHandler() const { return m_fatalErrorCallback; }

	int64_t AdjustAmountOfExternalAllocatedMemory(int64_t change_in_bytes);

	void GetHeapStatistics(HeapStatistics* heap_statistics);

	bool GetHeapSpaceStatistics(HeapSpaceStatistics* space_statistics, size_t index);

	size_t NumberOfHeapSpaces();

	HeapProfiler * GetHeapProfiler();

	V8_DEPRECATE_SOON("CpuProfiler should be created with CpuProfiler::New call.",
					  CpuProfiler * GetCpuProfiler());

	void SetPromiseHook(PromiseHook hook);

	void SetPromiseRejectCallback(PromiseRejectCallback callback);

	void RunMicrotasks();

	void EnqueueMicrotask(Local<v8::Function> microtask);

	void EnqueueMicrotask(MicrotaskCallback microtask, void* data);

	void SetMicrotasksPolicy(MicrotasksPolicy policy);

	MicrotasksPolicy GetMicrotasksPolicy() const { return m_microtasksPolicy; }

	void TerminateExecution();

	void CancelTerminateExecution();

	void RequestInterrupt(InterruptCallback callback, void * data);

	void LowMemoryNotification();

	explicit operator v8::Isolate*() { return reinterpret_cast<v8::Isolate *>(this); }

	WTF::SymbolRegistry& ApiPrivateSymbolRegistry() { return m_apiPrivateSymbolRegistry; }
	WTF::SymbolRegistry& ApiSymbolRegistry() { return m_apiSymbolRegistry; }

	v8::PromiseRejectCallback PromiseRejectCallback() const { return m_promiseRejectCallback; }

#ifdef DEBUG
	// Used for testing
	static size_t NonDisposedIsolates()
	{
		return s_nonDisposedIsolates;
	}
	
#endif

private:
	friend class ShimExceptionScope;

	// Construction/Destruction should go through New/Dispose
	explicit Isolate(RefPtr<JSC::VM>&& vm, v8::ArrayBuffer::Allocator * arrayBufferAllocator);
	~Isolate();

	void ReportMessageToListenersIfNeeded(jscshim::Message * message, JSC::Exception * exception);

	// Should be used only by ShimExceptionScope
	inline void RegisterShimExceptionScope()
	{
		if (m_topTryCatchHandler)
		{
			m_topTryCatchHandler->shim_scopes_current_depth_++;
		}
		else
		{
			m_shimBaseScopesDepth++;
		}
	}
	
	inline void UnregisterShimExceptionScope(JSC::Exception * currentException)
	{
		if (m_topTryCatchHandler)
		{
			m_topTryCatchHandler->shim_scopes_current_depth_--;
		}
		else
		{
			m_shimBaseScopesDepth--;
		}

		if (currentException)
		{
			PropagateThrownExceptionToAPI(currentException);
		}
	}
};

}} // v8::jscshim
