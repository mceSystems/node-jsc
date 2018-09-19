/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"

#include <JavaScriptCore/RegExpObject.h>
#include <JavaScriptCore/JSCInlines.h>

#define GET_JSC_THIS() v8::jscshim::GetJscCellFromV8<JSC::RegExpObject>(this)

/* Make sure (at compile time) that v8's and JSC's regexp flags have the same 
 * values, so we could easily cast between them */
#define MATCH_REGEXP_FLAG(flag) (static_cast<unsigned int>(v8::RegExp::k##flag) == static_cast<unsigned int>(JSC::RegExpFlags::Flag##flag))
static_assert((static_cast<unsigned int>(v8::RegExp::kNone) == static_cast<unsigned int>(JSC::RegExpFlags::NoFlags)) && 
			  MATCH_REGEXP_FLAG(Global) &&
			  MATCH_REGEXP_FLAG(IgnoreCase) &&
			  MATCH_REGEXP_FLAG(Multiline) &&
			  MATCH_REGEXP_FLAG(Sticky) &&
			  MATCH_REGEXP_FLAG(Unicode) &&
			  MATCH_REGEXP_FLAG(IgnoreCase) &&
			  MATCH_REGEXP_FLAG(DotAll), 
			  "v8\\JSC RegExp flags mismatch");
#undef MATCH_REGEXP_FLAG

namespace v8
{

Local<RegExp> RegExp::New(Local<String> pattern, Flags flags)
{
	return New(Isolate::GetCurrent()->GetCurrentContext(), pattern, flags).ToLocalChecked();
}

MaybeLocal<RegExp> RegExp::New(Local<Context> context,
							   Local<String> pattern,
							   Flags flags)
{
	JSC::ExecState * exec = jscshim::GetExecStateForV8Context(*context);
	JSC::VM& vm = exec->vm();
	DECLARE_SHIM_EXCEPTION_SCOPE(*context);

	const WTF::String& jscPattern = JSC::asString(pattern.val_)->tryGetValue();
	JSC::RegExp* regExp = JSC::RegExp::create(vm, jscPattern, static_cast<JSC::RegExpFlags>(flags));
	if (!regExp->isValid())
	{
		shimExceptionScope.ThrowExcpetion(JSC::createSyntaxError(exec, regExp->errorMessage()));
		return Local<RegExp>();
	}

	JSC::Structure * structure = exec->lexicalGlobalObject()->regExpStructure();
	return Local<RegExp>(JSC::RegExpObject::create(vm, structure, regExp));
}

Local<String> RegExp::GetSource() const
{
	JSC::RegExpObject * thisRegExp = GET_JSC_THIS();

	// TODO: Cache the JSString instance?
	return Local<String>(JSC::jsString(thisRegExp->vm(), thisRegExp->regExp()->pattern()));
}

RegExp::Flags RegExp::GetFlags() const
{
	// TODO: Are the InvalidFlags and DeletedValueFlags checks really needed?
	JSC::RegExpFlags flags = GET_JSC_THIS()->regExp()->key().flagsValue;
	if ((JSC::InvalidFlags == flags) || (JSC::DeletedValueFlags == flags))
	{
		return RegExp::kNone;
	}

	return static_cast<RegExp::Flags>(flags);
}

} // v8