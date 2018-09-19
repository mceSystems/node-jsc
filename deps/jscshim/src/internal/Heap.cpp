/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "../shim/helpers.h"

#include <JavaScriptCore/JSCInlines.h>

namespace v8 { namespace jscshim
{ 
void Heap::CollectGarbage(v8::Isolate * isolate)
{
	JSC::Heap * heap = &jscshim::V8IsolateToJscShimIsolate(isolate)->VM().heap;
	heap->collectNow(JSC::Sync, JSC::CollectionScope::Full);
}

void Heap::ProtectValue(const JSC::JSValue& value)
{
	if (!value || !value.isCell())
	{
		return;
	}

	JSC::JSCell * cell = value.asCell();
	JSC::VM& vm = *cell->vm();
	JSC::JSLockHolder locker(vm);

	JSC::JSGlobalObject * asGlobalObject = JSC::jsDynamicCast<JSC::JSGlobalObject *>(vm, cell);
	if (asGlobalObject)
	{
		JSC::ExecState * exec = asGlobalObject->globalExec();
		//JSC::JSLockHolder locker(exec);

		gcProtect(exec->vmEntryGlobalObject());
	}
	else
	{
		gcProtect(value.asCell());
	}
}

void Heap::UnprotectValue(const JSC::JSValue& value)
{
	if (value && value.isCell())
	{
		//JSC::JSCell * asCell = value.asCell();
		//JSC::JSLockHolder locker(asCell->vm());
		gcUnprotect(value.asCell());
	}
}

}} // v8::jscshim