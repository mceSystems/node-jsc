/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"
#include "shim/Script.h"

#include <JavaScriptCore/JSObject.h>
#include <JavaScriptCore/Completion.h>
#include <JavaScriptCore/SourceCode.h>
#include <JavaScriptCore/JSCInlines.h>
#include <wtf/text/OrdinalNumber.h>

#define GET_JSC_THIS() v8::jscshim::GetJscCellFromV8<jscshim::Script>(this)

namespace
{
	ALWAYS_INLINE int GetNumberValue(JSC::JSValue value)
	{
		return value.isInt32() ? value.asInt32() : 0;
	}
}

namespace v8
{
Local<Script> UnboundScript::BindToCurrentContext()
{
	jscshim::Script * unboundScript = GET_JSC_THIS();
	jscshim::GlobalObject * global = jscshim::Isolate::GetCurrent()->GetCurrentContext();

	/* Note that we assume that both the unbound script and the current context were created
	 * under the same VM, so it's ok to share JSStrings between them. */
	jscshim::Script * boundScript = jscshim::Script::create(global->vm(),
															global->shimScriptStructure(),
															unboundScript->source(),
															unboundScript->resourceName(),
															unboundScript->sourceMapUrl(),
															unboundScript->resourceLine(),
															unboundScript->resourceColumn());

	return Local<Script>::New(JSC::JSValue(boundScript));
}

// TODO: Actually compile/parse here. UnlinkedProgramCodeBlock\prepareForRepeatCall?
MaybeLocal<Script> Script::Compile(Local<Context> context, Local<String> source, ScriptOrigin* origin)
{
	jscshim::GlobalObject * global = jscshim::GetGlobalObjectForV8Context(*context);
	JSC::ExecState * exec = global->v8ContextExec();

	// Create the script in our global, which "binds" it to our context
	jscshim::Script * script = nullptr;
	if (origin)
	{
		script = jscshim::Script::create(exec->vm(),
										 global->shimScriptStructure(),
										 jscshim::GetJscCellFromV8<JSC::JSString>(*source),
										 jscshim::GetJscCellFromV8<JSC::JSString>(*origin->ResourceName()),
										 jscshim::GetJscCellFromV8<JSC::JSString>(*origin->SourceMapUrl()),
										 GetNumberValue(origin->ResourceLineOffset().val_),
										 GetNumberValue(origin->ResourceColumnOffset().val_));
	}
	else
	{
		script = jscshim::Script::create(exec->vm(),
										 global->shimScriptStructure(),
										 jscshim::GetJscCellFromV8<JSC::JSString>(*source),
										 JSC::jsEmptyString(exec),
										 JSC::jsEmptyString(exec),
										 0,
										 0);
	}

	return Local<Script>::New(JSC::JSValue(script));
}

MaybeLocal<Value> Script::Run(Local<Context> context)
{
	SETUP_JSC_USE_IN_FUNCTION(context);
	jscshim::Script * thisScript = GET_JSC_THIS();

	// TODO: Should we have a different property for the url?
	JSC::JSString * resourceName = thisScript->resourceName();
	const WTF::String& fileName = resourceName ? resourceName->value(exec) : exec->vm().smallStrings.emptyString()->value(exec);
	JSC::JSString * sourceMapUrl = thisScript->sourceMapUrl();
	const TextPosition sourceTextPosition(OrdinalNumber::fromZeroBasedInt(thisScript->resourceLine()), OrdinalNumber::fromZeroBasedInt(thisScript->resourceColumn()));
	JSC::SourceCode source = JSC::makeSource(thisScript->source()->value(exec),
											 JSC::SourceOrigin{ fileName },
											 fileName,
											 sourceTextPosition);
	if (sourceMapUrl)
	{
		source.provider()->setSourceURLDirective(sourceMapUrl->tryGetValue());
	}

	
	NakedPtr<JSC::Exception> evaluationException;
	JSC::JSValue returnValue;
	{
		/* In v8, the script will run in the context it was compiled in (thisScript->globalObject() will return
		 * the global of thisScript's structure, which in turn is the global the script is bound to (as we use
		 * the structure from the global passed to Script::Compile or the current on in UnboundScript::BindToCurrentContext). */
		jscshim::GlobalObject * scriptGlobal = static_cast<jscshim::GlobalObject *>(thisScript->globalObject());
		v8::jscshim::Isolate::CurrentContextScope currentContextScope(scriptGlobal);

		returnValue = JSC::profiledEvaluate(scriptGlobal->globalExec(),
											JSC::ProfilingReason::API, 
											source, 
											scriptGlobal->globalThis(), 
											evaluationException);
	}
	if (evaluationException)
	{
		// TODO: Should we still run the microtasks here, even though we have an exception
		shimExceptionScope.ThrowExcpetion(reinterpret_cast<JSC::JSObject *>(evaluationException.get()));
		return Local<Value>();
	}

	/* TODO: This shouldn't really be just here, but on most api calls (See v8's CallDepthScope usage in api.cc, which calls 
	 * Isolate::FireCallCompletedCallback, which in turn will run microtasks). But, since node calls Isolate::SetAutorunMicrotasks
	 * with false, it's only relevant to the unit tests. For now, doing it here will be enough. */
	jscshim::Isolate * isolate = jscshim::V8IsolateToJscShimIsolate(context->GetIsolate());
	if (v8::MicrotasksPolicy::kAuto == isolate->GetMicrotasksPolicy())
	{
		isolate->RunMicrotasks();
	}

	return Local<Value>::New(returnValue);
}

Local<Value> Script::Run()
{
	return Run(jscshim::GetV8ContextForObject(GET_JSC_THIS())).FromMaybe(Local<Value>());
}

/* TODO: Actually compile/parse here. UnlinkedProgramCodeBlock\prepareForRepeatCall?
 * TODO: no_cache_reason? */
MaybeLocal<UnboundScript> ScriptCompiler::CompileUnboundScript(Isolate* isolate, Source* source, CompileOptions options, NoCacheReason no_cache_reason)
{
	JSC::ExecState * exec = jscshim::GetExecStateForV8Isolate(isolate);

	jscshim::Script * script = jscshim::Script::create(exec->vm(),
													   jscshim::GetGlobalObject(exec)->shimScriptStructure(),
												  	   jscshim::GetJscCellFromV8<JSC::JSString>(*source->source_string),
													   jscshim::GetJscCellFromV8<JSC::JSString>(*source->resource_name),
													   jscshim::GetJscCellFromV8<JSC::JSString>(*source->source_map_url),
													   GetNumberValue(source->resource_line_offset.val_),
													   GetNumberValue(source->resource_column_offset.val_));

	return Local<UnboundScript>::New(JSC::JSValue(script));
}

/* TODO: Actually compile/parse here. UnlinkedProgramCodeBlock\prepareForRepeatCall?
 * TODO: no_cache_reason? */
MaybeLocal<Script> ScriptCompiler::Compile(Local<Context> context, Source* source, CompileOptions options, NoCacheReason no_cache_reason)
{
	jscshim::GlobalObject * global = jscshim::GetGlobalObjectForV8Context(*context);
	JSC::ExecState * exec = global->v8ContextExec();

	// Create the script in our global, whichi "binds" it to our context
	jscshim::Script * script = jscshim::Script::create(exec->vm(),
													   global->shimScriptStructure(),
													   jscshim::GetJscCellFromV8<JSC::JSString>(*source->source_string),
													   jscshim::GetJscCellFromV8<JSC::JSString>(*source->resource_name),
													   jscshim::GetJscCellFromV8<JSC::JSString>(*source->source_map_url),
													   GetNumberValue(source->resource_line_offset.val_),
													   GetNumberValue(source->resource_column_offset.val_));
	
	return Local<Script>::New(JSC::JSValue(script));
}

uint32_t ScriptCompiler::CachedDataVersionTag()
{
	// TODO: IMPLEMENT?
	return 0;
}

MaybeLocal<Module> ScriptCompiler::CompileModule(Isolate* isolate, Source* source)
{
	// TODO: IMPLEMENT
	return Local<Module>();
}

ScriptCompiler::CachedData * ScriptCompiler::CreateCodeCache(Local<UnboundScript> unbound_script)
{
	// TODO: IMPLEMENT
	return nullptr;
}

ScriptCompiler::CachedData * ScriptCompiler::CreateCodeCacheForFunction(Local<Function> function,
																		Local<String> source)
{
	// TODO: IMPLEMENT
	return nullptr;
}

ScriptCompiler::CachedData * ScriptCompiler::CreateCodeCacheForFunction(Local<Function> function)
{
	// TODO: IMPLEMENT
	return nullptr;
}

} // v8