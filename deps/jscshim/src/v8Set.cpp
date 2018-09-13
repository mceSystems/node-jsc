/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"

#include <JavaScriptCore/JSSet.h>
#include <JavaScriptCore/JSCInlines.h>

#define GET_JSC_THIS() v8::jscshim::GetJscCellFromV8<JSC::JSSet>(this)

namespace v8
{

size_t Set::Size() const
{
	return GET_JSC_THIS()->size();
}

void Set::Clear()
{
	JSC::ExecState * exec = jscshim::GetCurrentExecState();
	DECLARE_SHIM_EXCEPTION_SCOPE(exec);
	GET_JSC_THIS()->clear(exec);
}

MaybeLocal<Set> Set::Add(Local<Context>	context, Local<Value> key)
{
	JSC::JSSet * jscThis = GET_JSC_THIS();
	DECLARE_SHIM_EXCEPTION_SCOPE(*context);

	jscThis->add(jscshim::GetExecStateForV8Context(*context), key.val_);
	return Local<Set>(jscThis);
}

Maybe<bool> Set::Has(Local<Context>	context, Local<Value> key)
{
	DECLARE_SHIM_EXCEPTION_SCOPE(*context);
	return Just(GET_JSC_THIS()->has(jscshim::GetExecStateForV8Context(*context), key.val_));
}

Maybe<bool> Set::Delete(Local<Context> context, Local<Value> key)
{
	DECLARE_SHIM_EXCEPTION_SCOPE(*context);
	return Just(GET_JSC_THIS()->remove(jscshim::GetExecStateForV8Context(*context), key.val_));
}

/* Array allocation and initialization is based on JSC::constructArray (JSArray.h)
 * TODO: Share implementation with v8::Map::AsArray? */
Local<Array> Set::AsArray() const
{
	JSC::ExecState * exec = jscshim::GetCurrentExecState();
	JSC::VM& vm = exec->vm();
	JSC::JSSet * jscThis = GET_JSC_THIS();
	DECLARE_SHIM_EXCEPTION_SCOPE(exec);

	/* Allocate an uninitialized array with the needed size */
	JSC::ObjectInitializationScope objectScope(vm);
	JSC::JSArray* array = JSC::JSArray::tryCreateUninitializedRestricted(objectScope,
																		 exec->lexicalGlobalObject()->arrayStructureForIndexingTypeDuringAllocation(JSC::ArrayWithContiguous),
																		 jscThis->size());
	RELEASE_ASSERT(array);

	/* Copy the map's keys and values to the new array.
	 * Note: Iteration logic is copied from JSC::HashMapImpl::checkConsistency. */
	unsigned int arrayIndex = 0;
	auto * iter = jscThis->head()->next();
	auto * end = jscThis->tail();
	JSC::IndexingType arrayIndexingType = array->indexingType();
	while (iter != end)
	{
		/* Note that we're using initializeIndex and not array->butterfly()->contiguous().data()
		 * directly, since if we're "having a bad time" (globalObject->isHavingABadTime()), 
		 * the array won't be contiguous, but a "slow put" array. 
		 * See https://github.com/WebKit/webkit/commit/1c4a32c94c1f6c6aa35cf04a2b40c8fe29754b8e for more info
		 * about what's a "bad time". */
		array->initializeIndex(objectScope, arrayIndex++, iter->key(), arrayIndexingType);
		iter = iter->next();
	}
	RELEASE_ASSERT(jscThis->size() == arrayIndex);

	return Local<Array>(array);
}

Local<Set> Set::New(Isolate* isolate)
{
	JSC::ExecState * exec = jscshim::GetExecStateForV8Isolate(isolate);
	
	JSC::JSValue newSet(JSC::JSSet::create(exec, 
										   exec->vm(), 
										   exec->lexicalGlobalObject()->setStructure()));
	return Local<Set>(newSet);
}

} // v8