/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"

#include <JavaScriptCore/JSMap.h>
#include <JavaScriptCore/JSCInlines.h>

#define GET_JSC_THIS() v8::jscshim::GetJscCellFromV8<JSC::JSMap>(this)

namespace v8
{

size_t Map::Size() const
{
	return GET_JSC_THIS()->size();
}

void Map::Clear()
{
	JSC::ExecState * exec = jscshim::GetCurrentExecState();
	DECLARE_SHIM_EXCEPTION_SCOPE(exec);
	GET_JSC_THIS()->clear(exec);
}

MaybeLocal<Value> Map::Get(Local<Context> context, Local<Value> key)
{
	DECLARE_SHIM_EXCEPTION_SCOPE(*context);

	auto ** bucket = GET_JSC_THIS()->findBucket(jscshim::GetExecStateForV8Context(*context), key.val_);
	if (!bucket)
	{
		return Local<Value>(JSC::jsUndefined());
	}

	return Local<Value>((*bucket)->value());
	
}
MaybeLocal<Map> Map::Set(Local<Context>	context,
						 Local<Value>	key,
						 Local<Value>	value)
{
	JSC::JSMap * jscThis = GET_JSC_THIS();
	DECLARE_SHIM_EXCEPTION_SCOPE(*context);

	jscThis->set(jscshim::GetExecStateForV8Context(*context), key.val_, value.val_);
	return Local<Map>(jscThis);

}

Maybe<bool> Map::Has(Local<Context>	context, Local<Value> key)
{
	DECLARE_SHIM_EXCEPTION_SCOPE(*context);
	return Just(GET_JSC_THIS()->has(jscshim::GetExecStateForV8Context(*context), key.val_));
}

Maybe<bool> Map::Delete(Local<Context> context, Local<Value> key)
{
	DECLARE_SHIM_EXCEPTION_SCOPE(*context);
	return Just(GET_JSC_THIS()->remove(jscshim::GetExecStateForV8Context(*context), key.val_));
}

/* Array allocation and initialization is based on JSC::constructArray (JSArray.h)
 * TODO: Share implementation with v8::Set::AsArray? note that iter->value() is not available for JSSet (std::enable_if) */
Local<Array> Map::AsArray() const
{
	JSC::ExecState * exec = jscshim::GetCurrentExecState();
	JSC::VM& vm = exec->vm();
	JSC::JSMap * jscThis = GET_JSC_THIS();
	DECLARE_SHIM_EXCEPTION_SCOPE(exec);

	// Allocate an uninitialized array with the needed size
	JSC::ObjectInitializationScope objectScope(vm);
	JSC::JSArray* array = JSC::JSArray::tryCreateUninitializedRestricted(objectScope,
																		 exec->lexicalGlobalObject()->arrayStructureForIndexingTypeDuringAllocation(JSC::ArrayWithContiguous),
																		 jscThis->size() * 2);
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
		array->initializeIndex(objectScope, arrayIndex++, iter->value(), arrayIndexingType);

		iter = iter->next();
	}
	RELEASE_ASSERT(array->length() == arrayIndex);

	return Local<Array>(array);
}

Local<Map> Map::New(Isolate* isolate)
{
	JSC::ExecState * exec = jscshim::GetExecStateForV8Isolate(isolate);
	
	JSC::JSValue newMap(JSC::JSMap::create(exec, 
										   exec->vm(), 
										   exec->lexicalGlobalObject()->mapStructure()));
	return Local<Map>(newMap);
}

} // v8