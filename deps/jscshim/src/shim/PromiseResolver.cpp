/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "PromiseResolver.h"

#include <JavaScriptCore/JSCInlines.h>

namespace v8 { namespace jscshim
{

const JSC::ClassInfo PromiseResolver::s_info = { "JSCShimPromiseResolver", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(PromiseResolver) };

bool PromiseResolver::resolve(JSC::ExecState * exec, JSC::JSValue value)
{
	return resolveOrReject(exec, value, true);
}

bool PromiseResolver::reject(JSC::ExecState * exec, JSC::JSValue value)
{
	return resolveOrReject(exec, value, false);
}

void PromiseResolver::finishCreation(JSC::VM& vm, JSC::ExecState * exec)
{
	Base::finishCreation(vm, this->classInfo()->className);
	putDirect(vm, vm.propertyNames->length, JSC::jsNumber(2), ReadOnly | DontEnum);

	/* Create a new promise and set our selves as the new promise's executor, 
	 * so we'll get the resolve\reject callbacks */
	JSC::JSGlobalObject * globalObject = exec->lexicalGlobalObject();
	JSC::JSPromise * newPromise = JSC::JSPromise::create(vm, globalObject->promiseStructure());
	newPromise->initialize(exec, globalObject, this);
	ASSERT(m_resolver);
	ASSERT(m_rejector);

	m_promise.set(vm, this, newPromise);
}

void PromiseResolver::visitChildren(JSC::JSCell* cell, JSC::SlotVisitor& visitor)
{
	Base::visitChildren(cell, visitor);

	PromiseResolver * thisObject = JSC::jsCast<PromiseResolver *>(cell);
	visitor.appendUnbarriered(thisObject->m_promise.get());
	visitor.append(thisObject->m_resolver);
	visitor.append(thisObject->m_rejector);
}

// Note that our function will be called from our own finishCreation
JSC::EncodedJSValue JSC_HOST_CALL PromiseResolver::functionCall(JSC::ExecState * exec)
{
	JSC::VM& vm = exec->vm();

	PromiseResolver * instance = JSC::jsDynamicCast<PromiseResolver *>(vm, exec->jsCallee());
	ASSERT(instance);

	/* As the executor of our promise, we should be could by the promise's initialization function 
	 * (initializePromise in JSC's PromiseOperations.js) */
	JSC::JSFunction * resolve = JSC::jsDynamicCast<JSC::JSFunction *>(vm, exec->argument(0));
	JSC::JSFunction * reject = JSC::jsDynamicCast<JSC::JSFunction *>(vm, exec->argument(1));
	ASSERT(resolve);
	ASSERT(reject);

	instance->m_resolver.set(vm, instance, resolve);
	instance->m_rejector.set(vm, instance, reject);

	// The return value will be ignored
	return JSC::JSValue::encode(JSC::jsUndefined());
}

bool PromiseResolver::resolveOrReject(JSC::ExecState * exec, JSC::JSValue value, bool resolve)
{
	JSC::VM& vm = exec->vm();
	auto scope = DECLARE_THROW_SCOPE(vm);
	JSC::JSFunction * promiseFunction = resolve ? m_resolver.get() : m_rejector.get();

	JSC::CallData callData;
	auto callType = JSC::getCallData(vm, promiseFunction, callData);
	ASSERT(callType != JSC::CallType::None);

	JSC::MarkedArgumentBuffer arguments;
	arguments.append(value);
	ASSERT(!arguments.hasOverflowed());

	JSC::call(exec, promiseFunction, callType, callData, JSC::jsUndefined(), arguments);
	RETURN_IF_EXCEPTION(scope, false);

	return true;
}

}} // v8::jscshim