/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"
#include "shim/helpers.h"

#include <JavaScriptCore/JSExportMacros.h>
#include <JavaScriptCore/InitializeThreading.h>
#include <JavaScriptCore/Options.h>
#include <JavaScriptCore/JSCInlines.h>

#include <wtf/Assertions.h>

// TODO: Needed?
#include <wtf/MainThread.h>

namespace v8 
{

const char * V8::GetVersion()
{
	/* This is just the version that ships with node 8.11.1.
	 * TODO: Return the actual version we're compatible with. */
	return "6.2.414.50";
}

bool V8::Initialize()
{
	// TODO: Needed?
	WTF::initializeMainThread();

	JSC::initializeThreading();
	
#if defined(JSCSHIM_DISABLE_JIT) && JSCSHIM_DISABLE_JIT
	JSC::Options::useJIT() = false;
#endif

	return true;
}

bool V8::Dispose() 
{
	// TODO
	return true;
}

void V8::InitializePlatform(Platform * platform)
{
	// TODO
}

void V8::ShutdownPlatform()
{
	// TODO
}

void V8::SetFlagsFromString(const char * str, int length) {
	// TODO
}

void V8::SetFlagsFromCommandLine(int * argc, char ** argv, bool remove_flags)
{
	// TODO
}

void V8::SetEntropySource(EntropySource source)
{
	// TODO
}

void V8::FromJustIsNothing()
{
	jscshim::ReportApiFailure("v8::FromJust", "Maybe value is Nothing.");
}

void V8::ToLocalEmpty()
{
	jscshim::ReportApiFailure("v8::ToLocalChecked", "Empty MaybeLocal.");
}

void V8::InternalFieldOutOfBounds(int index)
{
	jscshim::ApiCheck(0 <= index && index < kInternalFieldsInWeakCallback, 
					  "WeakCallbackInfo::GetInternalField", 
					  "Internal field out of bounds.");
}

} // v8