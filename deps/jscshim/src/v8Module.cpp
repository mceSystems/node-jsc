/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include <JavaScriptCore/JSCInlines.h>

namespace v8
{

Module::Status Module::GetStatus() const
{
	// TODO: Implement
	return kUninstantiated;
}

Local<Value> Module::GetException() const
{
	// TODO: Implement
	return Local<Value>();
}

int Module::GetModuleRequestsLength() const
{
	// TODO: Implement
	return 0;
}

Local<String> Module::GetModuleRequest(int i) const
{
	// TODO: Implement
	return Local<String>();
}

Location Module::GetModuleRequestLocation(int i) const
{
	return { 0, 0 };
}

int Module::GetIdentityHash() const
{
	// TODO: Implement
	return 0;
}

bool Module::Instantiate(Local<Context> context, ResolveCallback callback)
{
	// TODO: Implement
	return false;
}
Maybe<bool> Module::InstantiateModule(Local<Context> context, ResolveCallback callback)
{
	return Just(false);
}

MaybeLocal<Value> Module::Evaluate(Local<Context> context)
{
	// TODO: Implement
	return Local<Value>();
}

Local<Value> Module::GetModuleNamespace()
{
	// TODO: Implement
	return Local<Value>();
}

} // v8