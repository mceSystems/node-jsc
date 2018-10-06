/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include "v8.h"
#include "Isolate.h"

#include <JavaScriptCore/JSDestructibleObject.h>
#include <JavaScriptCore/JSFunction.h>
#include <JavaScriptCore/JSPromise.h>

namespace v8 { namespace jscshim
{

// PromiseResolver is a shim for v8::PromiseResolver, which also acts the promise's executor (to get the resolve\reject callbacks)
class PromiseResolver final : public JSC::InternalFunction
{
private:
	JSC::WriteBarrier<JSC::JSPromise> m_promise;
	JSC::WriteBarrier<JSC::JSFunction> m_resolver;
	JSC::WriteBarrier<JSC::JSFunction> m_rejector;

public:
	typedef InternalFunction Base;

	template<typename CellType>
	static JSC::IsoSubspace * subspaceFor(JSC::VM& vm)
	{
		jscshim::Isolate * currentIsolate = jscshim::Isolate::GetCurrent();
		RELEASE_ASSERT(&currentIsolate->VM() == &vm);

		return currentIsolate->PromiseResolverSpace();
	}

	static PromiseResolver* create(JSC::VM& vm, JSC::Structure* structure, JSC::ExecState * exec)
	{
		PromiseResolver * promiseResolver = new (NotNull, JSC::allocateCell<PromiseResolver>(vm.heap)) PromiseResolver(vm, structure);
		promiseResolver->finishCreation(vm, exec);
		return promiseResolver;
	}

	DECLARE_INFO;

	static JSC::Structure * createStructure(JSC::VM& vm, JSC::JSGlobalObject * globalObject, JSC::JSValue prototype)
	{
		return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::InternalFunctionType, StructureFlags), info());
	}

	bool resolve(JSC::ExecState * exec, JSC::JSValue value);
	bool reject(JSC::ExecState * exec, JSC::JSValue value);

	JSC::JSPromise * promise() const { return m_promise.get(); }

private:
	PromiseResolver(JSC::VM& vm, JSC::Structure* structure) : Base(vm, structure, functionCall, JSC::callHostFunctionAsConstructor)
	{
	}

	void finishCreation(JSC::VM& vm, JSC::ExecState * exec);

	static void visitChildren(JSC::JSCell*, JSC::SlotVisitor&);

	static JSC::EncodedJSValue JSC_HOST_CALL functionCall(JSC::ExecState * exec);

	bool resolveOrReject(JSC::ExecState * exec,  JSC::JSValue value, bool resolve);
};

}} // v8::jscshim