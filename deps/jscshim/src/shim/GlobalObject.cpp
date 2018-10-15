/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"

/* TODO: Including v8.h is not really needed but currently fixes a compilation issue
 *		 regarding the "ENABLIE_JIT" not being properly defined. */
#include "v8.h"
#include "GlobalObject.h"

#include "Script.h"
#include "StackTrace.h"
#include "FunctionTemplate.h"
#include "Function.h"
#include "APIAccessor.h"
#include "PromiseResolver.h"
#include "Object.h"
#include "Message.h"
#include "External.h"
#include "CallSite.h"
#include "JSCStackTrace.h"

#include <JavaScriptCore/LazyPropertyInlines.h>
#include <JavaScriptCore/FunctionPrototype.h>
#include <JavaScriptCore/JSInternalPromise.h>
#include <JavaScriptCore/ErrorConstructor.h>
#include <JavaScriptCore/GCDeferralContext.h>
#include <JavaScriptCore/GCDeferralContextInlines.h>
#include <JavaScriptCore/JSCInlines.h>

#ifdef JSCSHIM_PROMISE_INTERNAL_FIELD_COUNT
#include <JavaScriptCore/JSWeakMap.h>
#endif

namespace
{

/* TODO: This is copied from JSC's JSGlobalObjectFunctions.cpp since JSGlobalObjectFunctions.h is currently not copied to
 * the ForwardingHeaders. Should we add it instead of copying it? */
const char* const ObjectProtoCalledOnNullOrUndefinedError = "Object.prototype.__proto__ called on null or undefined";

// Taken from v8 (see https://github.com/v8/v8/wiki/Stack-Trace-API)
constexpr size_t DEFAULT_ERROR_STACK_TRACE_LIMIT = 10;

}

namespace v8 { namespace jscshim
{

/* Note: Changed class name from "JSCShimGlobalObject" to "Object" to appear like regular objects when converted to a string
 * (using toString for example). */
const JSC::ClassInfo GlobalObject::s_info = { "Object", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(GlobalObject) };

const JSC::GlobalObjectMethodTable GlobalObject::globalObjectMethodTable = {
	&supportsRichSourceInfo,
	&shouldInterruptScript,
	&javaScriptRuntimeFlags,
	nullptr, // queueTaskToEventLoop
	&shouldInterruptScriptBeforeTimeout,
	nullptr, // moduleLoaderImportModule
	nullptr, // moduleLoaderResolve
	nullptr, // moduleLoaderFetch
	nullptr, // moduleLoaderCreateImportMetaProperties
	nullptr, // moduleLoaderEvaluate
	&promiseRejectionTracker, // promiseRejectionTracker
	nullptr, // defaultLanguage
	nullptr, // compileStreaming
	nullptr  // instantiateStreaming
};

GlobalObject * GlobalObject::create(JSC::VM& vm, JSC::Structure * structure, int globalInternalFieldCount)
{
	GlobalObject * global = new (NotNull, JSC::allocateCell<GlobalObject>(vm.heap)) GlobalObject(vm, structure, globalInternalFieldCount);
	global->finishCreation(vm);
	return global;
}

JSC::Structure * GlobalObject::createStructure(JSC::VM& vm, JSC::JSValue prototype)
{
	return JSC::Structure::create(vm, 0, prototype, JSC::TypeInfo(JSC::GlobalObjectType, StructureFlags), info());
}

void GlobalObject::visitChildren(JSC::JSCell* cell, JSC::SlotVisitor& visitor)
{
	Base::visitChildren(cell, visitor);

	GlobalObject * thisObject = JSC::jsCast<GlobalObject *>(cell);
	visitor.append(thisObject->m_shimScriptStructure);
	visitor.append(thisObject->m_shimFunctionTemplateStructure);
	visitor.append(thisObject->m_shimObjectTemplateStructure);
	visitor.append(thisObject->m_shimFunctionStructure);
	visitor.append(thisObject->m_shimAPIAccessorStructure);
	visitor.append(thisObject->m_shimPromiseResolverStructure);
	visitor.append(thisObject->m_shimExternalStructure);
	thisObject->m_shimStackTraceStructure.visit(visitor);
	thisObject->m_shimStackFrameStructure.visit(visitor);
	thisObject->m_callSiteStructure.visit(visitor);
	thisObject->m_callSitePrototype.visit(visitor);

#ifdef JSCSHIM_PROMISE_INTERNAL_FIELD_COUNT
	thisObject->m_promisesInternalFields.visit(visitor);
#endif

	visitor.append(thisObject->m_objectProtoToString);
}

JSC::JSValue GlobalObject::getValueFromEmbedderData(int index)
{
	return m_contextEmbedderData.GetValue(index);
}

void GlobalObject::setValueInEmbedderData(int index, JSC::JSValue value)
{
	m_contextEmbedderData.SetValue(index, value);
}

void * GlobalObject::getAlignedPointerFromEmbedderData(int index)
{
	return m_contextEmbedderData.GetAlignedPointer(index);
}

void GlobalObject::setAlignedPointerInEmbedderData(int index, void * value)
{
	m_contextEmbedderData.SetAlignedPointer(index, value);
}

GlobalObject::GlobalObject(JSC::VM& vm, JSC::Structure * structure, int globalInternalFieldCount) : JSGlobalObject(vm, structure, &globalObjectMethodTable),
	m_contextEmbedderData(EMBEDDER_DATA_INITIAL_SIZE),
	m_globalInternalFields(globalInternalFieldCount)
{
}

void GlobalObject::finishCreation(JSC::VM& vm)
{
	Base::finishCreation(vm);

	m_contextExec = globalExec();

	initShimStructuresAndPrototypes(vm);
	setupObjectProtoAccessor(vm);

	/* Add the captureStackTrace api to the Error prototype.
	 * TODO: stackTraceLimit */
	JSC::ErrorConstructor * errorConstructor = this->errorConstructor();
	errorConstructor->putDirectNativeFunctionWithoutTransition(vm, this, JSC::Identifier::fromString(&vm, "captureStackTrace"), 2, errorConstructorCaptureStackTrace, JSC::NoIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));

	// Cache the the Object prototype's "ToString" function, used by v8::Object::ObjectProtoToString	
	m_objectProtoToString.set(vm, this, objectPrototype()->get(m_contextExec, vm.propertyNames->toString));
	ASSERT(m_objectProtoToString);

#ifdef JSCSHIM_PROMISE_INTERNAL_FIELD_COUNT
	m_promisesInternalFields.initLater([](const Initializer<JSC::JSWeakMap>& init) {
		init.set(JSC::JSWeakMap::create(init.vm, init.owner->weakMapStructure()));
	});
#endif
}

void GlobalObject::initShimStructuresAndPrototypes(JSC::VM& vm)
{
	m_shimScriptStructure.set(vm, this, jscshim::Script::createStructure(vm, this, JSC::jsNull()));

	JSC::FunctionPrototype * functionPrototype = Base::functionPrototype();
	m_shimFunctionTemplateStructure.set(vm, this, jscshim::FunctionTemplate::createStructure(vm, this, functionPrototype));
	m_shimObjectTemplateStructure.set(vm, this, jscshim::ObjectTemplate::createStructure(vm, this, functionPrototype));
	m_shimFunctionStructure.set(vm, this, jscshim::Function::createStructure(vm, this, functionPrototype));
	m_shimPromiseResolverStructure.set(vm, this, jscshim::PromiseResolver::createStructure(vm, this, functionPrototype));
	m_shimAPIAccessorStructure.set(vm, this, jscshim::APIAccessor::createStructure(vm, this, JSC::jsNull()));
	m_shimExternalStructure.set(vm, this, jscshim::External::createStructure(vm, this, JSC::jsNull()));

	m_shimStackFrameStructure.initLater([](const Initializer<JSC::Structure>& init) {
		init.set(StackFrame::createStructure(init.vm, init.owner, JSC::jsNull()));
	});
	m_shimStackTraceStructure.initLater([](const Initializer<JSC::Structure>& init) {
		init.set(StackTrace::createStructure(init.vm, init.owner, JSC::jsNull()));
	});
	m_shimMessageStructure.initLater([](const Initializer<JSC::Structure>& init) {
		init.set(Message::createStructure(init.vm, init.owner, JSC::jsNull()));
	});

	m_callSitePrototype.initLater([](const Initializer<CallSitePrototype>& init) {
		init.set(CallSitePrototype::create(init.vm, CallSitePrototype::createStructure(init.vm, init.owner, JSC::jsNull()), init.owner));
	});
	m_callSiteStructure.initLater([](const Initializer<JSC::Structure>& init) {
		jscshim::GlobalObject * globalObject = reinterpret_cast<jscshim::GlobalObject *>(init.owner);
		init.set(CallSite::createStructure(init.vm, globalObject, globalObject->callSitePrototype()));
	});
}

void GlobalObject::setupObjectProtoAccessor(JSC::VM& vm)
{
	/* In v8, the Object.prototype.__proto__ handler has special hanlding for hidden prototypes:
	 * - Getting the prototype will skip hidden prototypes.
	 * - From v8's unit tests: "Setting the prototype on an object skips hidden prototypes", meaning
	 *   it'll set the prototype of the first object in the (prototype) chain without a hidden template.
	 *
	 * To implement it in jscshim, we must override the Object.prototype.__proto__ accessor. It's not enough
	 * to add __proto__ handler to jscshim::Object's prototype, since a hidden prototype can be set on a 
	 * a "regular" js object. 
	 * In JSC, JSGlobalObject::init (call by JSGlobalObject::finishCreation) will create the __proto__ accessor
	 * (a JSC::GetterSetter object) on the (global) Object prototype. the actual getter\setter (globalFuncProtoGetter\globalFuncProtoSetter)
	 * are defined in JSC's JSGlobalObjectFunctions.cpp, and are not exported (although we could get their addresses from the GetterSetter).
	 * Since it's not possible to change the existing GetterSetter (since in JSC GetterSetter's getter\setter can only be set once, and 
	 * the native callbacks of the getter\setter JSFunction can't be modified), the simplest solution is to just override it with a 
	 * new GetterStter object, with our own getter\setter implementation. This is far from ideal, as we're wasting 3 allocated objects
	 * (the GetterSetter and two JSFunctions) and since calling the original getter\setter is not convenient (although their implementation
	 * is pretty short). For now, we'll use this solution, as it is the simplest one and doesn't require any changes to JSC's code.
	 *
	 * Note that there is another downside for this solution: JSC has a UnderscoreProtoIntrinsic intrinsic, meaning the JIT can
	 * generate the code for getting the prototype directly, without making the call to the __proto__ accessor. But, since it seems to
	 * be a JIT issue only, and we disable the JIT, it's not an issue for us.
	 *
	 * TODO: Find a better solution?
	 */
	const JSC::Identifier& underscoreProto = vm.propertyNames->underscoreProto;
	JSC::ObjectPrototype * objectPrototype = this->objectPrototype();

	JSC::PropertyOffset protoAccessorOffset = objectPrototype->getDirectOffset(vm, underscoreProto);
	ASSERT(JSC::invalidOffset != protoAccessorOffset);
	
	JSC::GetterSetter * newProtoAccessor = JSC::GetterSetter::create(vm,
																	 this,
																	 JSC::JSFunction::create(vm, this, 0, makeString("get ", underscoreProto.string()), objectProtoGetter),
																	 JSC::JSFunction::create(vm, this, 0, makeString("set ", underscoreProto.string()), objectProtoSetter));
	objectPrototype->putDirect(vm, protoAccessorOffset, newProtoAccessor);
}

void GlobalObject::promiseRejectionTracker(JSGlobalObject * jsGlobalObject, JSC::ExecState* exec, JSC::JSPromise* promise, JSC::JSPromiseRejectionOperation operation)
{
	JSC::VM& vm = exec->vm();
	GlobalObject * self = JSC::jsCast<GlobalObject *>(jsGlobalObject);

	v8::PromiseRejectCallback userCallback = GetIsolate(vm)->PromiseRejectCallback();
	if (nullptr == userCallback)
	{
		return;
	}

	// Don't expost InternalPromises to outsiders. This is what JSDOMWindowBase::promiseRejectionTracker does.
	// TODO: Make sure we should do this
	if (JSC::jsDynamicCast<JSC::JSInternalPromise*>(self->m_vm, promise))
	{
		return;
	}

	if (JSC::JSPromiseRejectionOperation::Reject == operation)
	{
		v8::Local<v8::Value> value(promise->result(self->m_vm));
		v8::Local<v8::StackTrace> stackTrace(v8::Exception::GetStackTrace(value));

		userCallback(v8::PromiseRejectMessage(v8::Local<v8::Promise>(promise),
											  v8::kPromiseRejectWithNoHandler,
											  value, 
											  stackTrace));
	}
	else
	{
		userCallback(v8::PromiseRejectMessage(v8::Local<v8::Promise>(promise),
											  v8::kPromiseHandlerAddedAfterReject, 
											  v8::Local<v8::Value>(), 
											  v8::Local<v8::StackTrace>()));
	}
}

/* Based on JSC's globalFuncProtoGetter (JSGlobalObject.cpp), with the addition of
 * skipping hidden prototypes.
 * TODO: Instead of copying the code from's JSC's globalFuncProtoGetter (which is 
 * defined in JSGlobalObject.cpp but is not exported), we should get it's address from the 
 * original getter and use it here. */
JSC::EncodedJSValue JSC_HOST_CALL GlobalObject::objectProtoGetter(JSC::ExecState * exec)
{
	JSC::VM& vm = exec->vm();
	auto scope = DECLARE_THROW_SCOPE(vm);

	JSC::JSValue thisValue = exec->thisValue().toThis(exec, JSC::StrictMode);
	if (thisValue.isUndefinedOrNull())
	{
		return JSC::throwVMError(exec, scope, JSC::createNotAnObjectError(exec, thisValue));
	}

	JSC::JSObject * thisObject = JSC::jsDynamicCast<JSC::JSObject *>(vm, thisValue);
	if (!thisObject)
	{
		JSC::JSObject* prototype = thisValue.synthesizePrototype(exec);
		EXCEPTION_ASSERT(!!scope.exception() == !prototype);
		return JSC::JSValue::encode((LIKELY(prototype)) ? prototype : JSC::JSValue());
	}

	// Skip hidden prototypes
	JSC::JSValue prototype = thisObject->getPrototype(vm, exec);
	jscshim::Object * prototypeAsJscshimObject = JSC::jsDynamicCast<jscshim::Object *>(vm, prototype);
	while (prototypeAsJscshimObject && prototypeAsJscshimObject->shouldBeHiddenAsPrototype())
	{
		prototype = prototypeAsJscshimObject->getPrototype(vm, exec);
		prototypeAsJscshimObject = JSC::jsDynamicCast<jscshim::Object *>(vm, prototype);
	}

	scope.release();
	return JSC::JSValue::encode(prototype);
}

/* Based on JSC's globalFuncProtoSetter (JSGlobalObject.cpp), with the addition of
 * skipping hidden prototypes */
JSC::EncodedJSValue JSC_HOST_CALL GlobalObject::objectProtoSetter(JSC::ExecState * exec)
{
	JSC::VM& vm = exec->vm();
	auto scope = DECLARE_THROW_SCOPE(vm);

	JSC::JSValue thisValue = exec->thisValue().toThis(exec, JSC::StrictMode);
	if (thisValue.isUndefinedOrNull())
	{
		return throwVMTypeError(exec, scope, ObjectProtoCalledOnNullOrUndefinedError);
	}

	JSC::JSValue value = exec->argument(0);

	JSC::JSObject * thisObject = JSC::jsDynamicCast<JSC::JSObject *>(vm, thisValue);

	// Setting __proto__ of a primitive should have no effect.
	if (!thisObject)
	{
		return JSC::JSValue::encode(JSC::jsUndefined());
	}

	// Setting __proto__ to a non-object, non-null value is silently ignored to match Mozilla.
	if (!value.isObject() && !value.isNull())
	{
		return JSC::JSValue::encode(JSC::jsUndefined());
	}
	
	/* From v8's tests: "Setting the prototype on an object skips hidden prototypes."
	 * So, we'll follow v8's JSObject::SetPrototype logic here ("Find the first object 
	 * in the chain whose prototype object is not hidden."). */
	JSC::JSValue currentPrototype = thisObject->getPrototype(vm, exec);
	jscshim::Object * currentPrototypeAsJscshimObject = JSC::jsDynamicCast<jscshim::Object *>(vm, currentPrototype);
	while (currentPrototypeAsJscshimObject && currentPrototypeAsJscshimObject->shouldBeHiddenAsPrototype())
	{
		thisObject = currentPrototypeAsJscshimObject;
		currentPrototype = currentPrototypeAsJscshimObject->getPrototype(vm, exec);
		currentPrototypeAsJscshimObject = JSC::jsDynamicCast<jscshim::Object *>(vm, currentPrototype);
	}

	scope.release();
	bool shouldThrowIfCantSet = true;
	thisObject->setPrototype(vm, exec, value, shouldThrowIfCantSet);
	return JSC::JSValue::encode(JSC::jsUndefined());
}

/* Note: For more information about v8's captureStackTrace and call sites, see:
 * https://github.com/v8/v8/wiki/Stack-Trace-API */
JSC::EncodedJSValue JSC_HOST_CALL GlobalObject::errorConstructorCaptureStackTrace(JSC::ExecState * exec)
{
	JSC::VM& vm = exec->vm();
	auto scope = DECLARE_THROW_SCOPE(vm);
	GlobalObject * globalObject = jscshim::GetGlobalObject(exec);

	JSC::JSValue objectArg = exec->argument(0);
	if (!objectArg.isObject())
	{
		return JSC::JSValue::encode(throwTypeError(exec, scope, "invalid_argument"_s));
	}

	JSC::JSObject * errorObject = objectArg.asCell()->getObject();
	JSC::JSValue caller = exec->argument(1);

	/* Get the current stack trace. Note that there's no need to skip the current (our) frame,
	 * since JSCStackTrace::captureCurrentJSStackTrace will skip native frames anyway.
	 * TODO: Handle caller argument
	 * TODO: Read Error.stackTraceLimit */
	JSCStackTrace stackTrace = JSCStackTrace::captureCurrentJSStackTrace(exec, DEFAULT_ERROR_STACK_TRACE_LIMIT);

	// Create an (uninitialized) array for our "call sites"
	JSC::GCDeferralContext deferralContext(vm.heap);
	JSC::ObjectInitializationScope objectScope(vm);
	JSC::JSArray * callSites = JSC::JSArray::tryCreateUninitializedRestricted(objectScope, 
																			  &deferralContext, 
																			  exec->lexicalGlobalObject()->arrayStructureForIndexingTypeDuringAllocation(JSC::ArrayWithContiguous), 
																			  stackTrace.size());
	RELEASE_ASSERT(callSites);

	// Create the call sites (one per frame)
	createCallSitesFromFrames(exec, objectScope, stackTrace, callSites);

	/* Foramt the stack trace.
	 * Note that v8 won't actually format the stack trace here, but will create a "stack" accessor
	 * on the error object, which will format the stack trace on the first access. For now, since
	 * we're not being used internally by JSC, we can assume callers of Error.captureStackTrace in 
	 * node are interested in the (formatted) stack. */
	 JSC::JSValue formattedStackTrace = globalObject->formatStackTrace(vm, exec, errorObject, callSites);
	 RETURN_IF_EXCEPTION(scope, JSC::JSValue::encode(scope.exception()));

	/* Add a "stack" property to the error object
	 * TODO: v8's ErrorCaptureStackTrace (builtins-error.cc) checks if the error object is frozen and throws
	 * a type error if it is. Should we do it too? */
	JSC::PutPropertySlot stackPutPropertySlot(errorObject, true);
    errorObject->putInline(exec, vm.propertyNames->stack, formattedStackTrace, stackPutPropertySlot);
	RETURN_IF_EXCEPTION(scope, JSC::JSValue::encode(scope.exception()));

	return JSC::JSValue::encode(JSC::jsUndefined()); 
}

void GlobalObject::createCallSitesFromFrames(JSC::ExecState * exec, JSC::ObjectInitializationScope& objectScope, JSCStackTrace& stackTrace, JSC::JSArray * callSites)
{
	/* From v8's "Stack Trace API" (https://github.com/v8/v8/wiki/Stack-Trace-API): 
	 * "To maintain restrictions imposed on strict mode functions, frames that have a 
	 * strict mode function and all frames below (its caller etc.) are not allow to access 
	 * their receiver and function objects. For those frames, getFunction() and getThis() 
	 * will return undefined."." */
	bool encounteredStrictFrame = false;

	JSC::Structure * callSiteStructure = jscshim::GetGlobalObject(exec)->callSiteStructure();
	JSC::IndexingType callSitesIndexingType = callSites->indexingType();
	size_t framesCount = stackTrace.size();
	for (size_t i = 0; i < framesCount; i++)
	{
		/* Note that we're using initializeIndex and not callSites->butterfly()->contiguous().data()
		 * directly, since if we're "having a bad time" (globalObject->isHavingABadTime()), 
		 * the array won't be contiguous, but a "slow put" array. 
		 * See https://github.com/WebKit/webkit/commit/1c4a32c94c1f6c6aa35cf04a2b40c8fe29754b8e for more info
		 * about what's a "bad time". */
		CallSite * callSite = CallSite::create(exec, callSiteStructure, stackTrace.at(i), encounteredStrictFrame);
		callSites->initializeIndex(objectScope, i, callSite, callSitesIndexingType);

		if (!encounteredStrictFrame)
		{
			encounteredStrictFrame = callSite->isStrict();
		}
	}
}

JSC::JSValue GlobalObject::formatStackTrace(JSC::VM& vm, JSC::ExecState * exec, JSC::JSObject * errorObject, JSC::JSArray * callSites)
{
	auto scope = DECLARE_THROW_SCOPE(vm);
	JSC::ErrorConstructor * errorConstructor = this->errorConstructor();

	/* If the user has set a callable Error.prepareStackTrace - use it to format the stack trace.
	 * TODO: Cache the prepareStackTrace string (PropertyName)? */
	JSC::JSValue prepareStackTrace = errorConstructor->get(exec, JSC::Identifier::fromString(&vm, "prepareStackTrace"));
	JSC::CallData prepareStackTraceCallData;
	JSC::CallType prepareStackTraceCallType = JSC::CallType::None;
	if (prepareStackTrace && prepareStackTrace.isCallable(vm, prepareStackTraceCallType, prepareStackTraceCallData))
	{
		JSC::MarkedArgumentBuffer arguments;
		arguments.append(errorObject);
		arguments.append(callSites);
		ASSERT(!arguments.hasOverflowed());

		JSC::JSValue result = profiledCall(exec, 
										   JSC::ProfilingReason::Other,
										   prepareStackTrace,
										   prepareStackTraceCallType,
										   prepareStackTraceCallData,
										   errorConstructor,
										   arguments);
		RETURN_IF_EXCEPTION(scope, JSC::jsUndefined());
		return result;
	}

	// TODO: Provide default formatting, like in v8
	return JSC::JSValue(callSites);
}

}} // v8::jscshim
