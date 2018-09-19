/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include "Isolate.h"
#include "GlobalObject.h"

#include <JavaScriptCore/ThrowScope.h>
#include <JavaScriptCore/CatchScope.h>

namespace v8 { namespace jscshim
{

class ShimExceptionScope
{
protected:
	/* Note that we're using a ThrowScope even though we're not really using it to throw an exception. 
	 * This is because we're never clearing the exception on destruction, which users of CatchScope should
	 * do according to it's docs (and is verified when JSC's EXCEPTION_SCOPE_VERIFICATION is enabled). */
	JSC::ThrowScope m_throwScope;
	jscshim::Isolate * m_isolate;

public:
	ShimExceptionScope(jscshim::Isolate * isolate) :
#if ENABLE(EXCEPTION_SCOPE_VERIFICATION)
		m_throwScope(isolate->VM(), JSC::ExceptionEventLocation(__FUNCTION__, __FILE__, __LINE__)),
#else
		m_throwScope(isolate->VM()),
#endif
		m_isolate(isolate)
	{
		isolate->RegisterShimExceptionScope();
	}

	ShimExceptionScope(v8::Isolate * isolate) : ShimExceptionScope(reinterpret_cast<jscshim::Isolate *>(isolate))
	{
	}

	ShimExceptionScope(v8::Context * context) : ShimExceptionScope(reinterpret_cast<jscshim::Isolate *>(context->GetIsolate()))
	{
	}

	ShimExceptionScope(JSC::ExecState * exec) : ShimExceptionScope(static_cast<GlobalObject *>(exec->lexicalGlobalObject())->isolate())
	{
	}

	ShimExceptionScope() : ShimExceptionScope(jscshim::Isolate::GetCurrent()) 
	{
	}

	~ShimExceptionScope()
	{
		m_isolate->UnregisterShimExceptionScope(m_throwScope.exception());
		m_throwScope.release();
	}

	bool HasException() { return !!m_throwScope.exception(); }

	void ThrowExcpetion(const JSC::JSValue& value) { m_isolate->ThrowException(value); }
};

/* Note: Because exceptions are objects, v8::Object member functions could be called on exceptions, thus will
 * be called when there's already an exception caught. This will cause JSC's RESTURN_IF_EXCEPTION to always return.
 * This will cause JSC functions to fail even if they actually succeeded (since they use this macros internally), 
 * and won't allow us to use this macro.
 * To handle this situation, we'll mimic JSC's SuspendExceptionScope (from JSC's FrameTracers.h) and clear 
 * the exception on enter and "restore" it when leaving the scope. 
 * Note that this is done through the Isolate rather than directly here to preserve the "message" (if we have one). */
class SuspendRestoreShimExceptionScope : public ShimExceptionScope
{
private:
	JSC::Exception * m_suspendedException;

public:
	SuspendRestoreShimExceptionScope(jscshim::Isolate * isolate) : ShimExceptionScope(isolate)
	{
		m_suspendedException = m_throwScope.exception();
		if (m_suspendedException)
		{
			DECLARE_CATCH_SCOPE(m_isolate->VM()).clearException();
		}
	}

	SuspendRestoreShimExceptionScope(v8::Isolate * isolate) : SuspendRestoreShimExceptionScope(reinterpret_cast<jscshim::Isolate *>(isolate))
	{
	}

	SuspendRestoreShimExceptionScope(v8::Context * context) : SuspendRestoreShimExceptionScope(reinterpret_cast<jscshim::Isolate *>(context->GetIsolate()))
	{
	}

	SuspendRestoreShimExceptionScope(JSC::ExecState * exec) : SuspendRestoreShimExceptionScope(static_cast<GlobalObject *>(exec->lexicalGlobalObject())->isolate())
	{
	}

	SuspendRestoreShimExceptionScope() : SuspendRestoreShimExceptionScope(jscshim::Isolate::GetCurrent())
	{
	}

	~SuspendRestoreShimExceptionScope()
	{
		if (m_suspendedException)
		{
			m_isolate->VM().restorePreviousException(m_suspendedException);
		}
	}
};

}} // v8::jscshim

#define DECLARE_SHIM_EXCEPTION_SCOPE(arg) v8::jscshim::ShimExceptionScope shimExceptionScope(arg)
#define DECLARE_SHIM_SUSPEND_RESTORE_EXCEPTION_SCOPE(arg) v8::jscshim::SuspendRestoreShimExceptionScope shimExceptionScope(arg)
#define SHIM_RETURN_IF_EXCEPTION(value) if (UNLIKELY(shimExceptionScope.HasException())) { return value; }
#define SHIM_THROW_EXCEPTION(exception) shimExceptionScope.ThrowExcpetion(exception)

