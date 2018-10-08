/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include "EmbeddedFieldsContainer.h"
#include "CallSitePrototype.h"

#include <JavaScriptCore/JSCJSValue.h>
#include <JavaScriptCore/JSGlobalObject.h>

namespace v8 { namespace jscshim
{
class Isolate;
class JSCStackTrace;

class GlobalObject final : public JSC::JSGlobalObject {
private:
	// v8 reserves the first slot to itself, and node uses another one (in node_contextify.cc)
	static int constexpr EMBEDDER_DATA_INITIAL_SIZE = 2;

	template<typename T> using Initializer = typename JSC::LazyProperty<GlobalObject, T>::Initializer;

	Isolate * m_isolate;

	/* In jscshim, a v8::Context is actually just the global object. But in v8 they're different
	 * entities. They also have different internal\embedded fields: Contexts have auto-growing 
	 * "embedder" data, while global objects might have internal fields (if they were created
	 * from an object template with internal fields). Thus, we'll hold the fields for both Contexts
	 * and our global object here. */
	EmbeddedFieldsContainer<true> m_contextEmbedderData;
	EmbeddedFieldsContainer<false> m_globalInternalFields;

	// Cache the globalExec for repeated use
	JSC::ExecState * m_contextExec;

	JSC::WriteBarrier<JSC::Structure> m_shimScriptStructure;
	JSC::WriteBarrier<JSC::Structure> m_shimFunctionTemplateStructure;
	JSC::WriteBarrier<JSC::Structure> m_shimObjectTemplateStructure;
	JSC::WriteBarrier<JSC::Structure> m_shimFunctionStructure;
	JSC::WriteBarrier<JSC::Structure> m_shimAPIAccessorStructure;
	JSC::WriteBarrier<JSC::Structure> m_shimPromiseResolverStructure;
	JSC::WriteBarrier<JSC::Structure> m_shimExternalStructure;
	JSC::LazyProperty<GlobalObject, JSC::Structure> m_shimStackTraceStructure;
	JSC::LazyProperty<GlobalObject, JSC::Structure> m_shimStackFrameStructure;
	JSC::LazyProperty<GlobalObject, JSC::Structure> m_shimMessageStructure;
	JSC::LazyProperty<GlobalObject, JSC::Structure> m_callSiteStructure;
	JSC::LazyProperty<GlobalObject, CallSitePrototype> m_callSitePrototype;

#ifdef JSCSHIM_PROMISE_INTERNAL_FIELD_COUNT
	JSC::LazyProperty<GlobalObject, JSC::JSWeakMap> m_promisesInternalFields;
#endif
	
	/* Used by v8::Object::ObjectProtoToString, which accroding to v8 must always call the original Object.prototype.toString,
	 * even if the user has overridden it. Thus, We'll store it right after the creation of our global .*/
	JSC::WriteBarrier<JSC::Unknown> m_objectProtoToString;
	
public:
	typedef JSC::JSGlobalObject Base;

	static GlobalObject * create(JSC::VM& vm, JSC::Structure * structure, Isolate * isolate, int globalInternalFieldCount);

	static const bool needsDestruction = true;

	DECLARE_INFO;

	static JSC::Structure * createStructure(JSC::VM& vm, JSC::JSValue prototype);

	static const JSC::GlobalObjectMethodTable globalObjectMethodTable;

	static void visitChildren(JSC::JSCell * cell, JSC::SlotVisitor& visitor);

	JSC::Structure * shimScriptStructure() const { return m_shimScriptStructure.get(); }
	JSC::Structure * shimFunctionStructure() const { return m_shimFunctionStructure.get(); }
	JSC::Structure * shimFunctionTemplateStructure() const { return m_shimFunctionTemplateStructure.get(); }
	JSC::Structure * shimObjectTemplateStructure() const { return m_shimObjectTemplateStructure.get(); }
	JSC::Structure * shimAPIAccessorStructure() const { return m_shimAPIAccessorStructure.get(); }
	JSC::Structure * shimPromiseResolverStructure() const { return m_shimPromiseResolverStructure.get(); }
	JSC::Structure * shimExternalStructure() const { return m_shimExternalStructure.get(); }
	JSC::Structure * shimStackTraceStructure() const { return m_shimStackTraceStructure.get(this); }
	JSC::Structure * shimStackFrameStructure() const { return m_shimStackFrameStructure.get(this); }
	JSC::Structure * shimMessageStructure() const { return m_shimMessageStructure.get(this); }
	JSC::Structure * callSiteStructure() const { return m_callSiteStructure.get(this); }
	CallSitePrototype * callSitePrototype() const { return m_callSitePrototype.get(this); }

#ifdef JSCSHIM_PROMISE_INTERNAL_FIELD_COUNT
	JSC::JSWeakMap * promisesInternalFields() const { return m_promisesInternalFields.get(this); }
#endif

	JSC::JSValue objectProtoToString() const { return m_objectProtoToString.get(); }
	
	Isolate * isolate() const { return m_isolate; }
	JSC::ExecState * v8ContextExec() const { return m_contextExec; }

	EmbeddedFieldsContainer<false>& globalInternalFields() { return m_globalInternalFields; }
	
	size_t contextEmbedderDataSize() const { return m_contextEmbedderData.Size(); }

	JSC::JSValue getValueFromEmbedderData(int index);
	void setValueInEmbedderData(int index, JSC::JSValue value);

	void * getAlignedPointerFromEmbedderData(int index);
	void setAlignedPointerInEmbedderData(int index, void * value);

private:
	GlobalObject(JSC::VM& vm, JSC::Structure * structure, Isolate * isolate, int globalInternalFieldCount);

	~GlobalObject() {}

	static void destroy(JSC::JSCell* cell)
	{
		static_cast<GlobalObject *>(cell)->GlobalObject::~GlobalObject();
	}

	void finishCreation(JSC::VM& vm);

	void initShimStructuresAndPrototypes(JSC::VM& vm);
	void setupObjectProtoAccessor(JSC::VM& vm);

	static void promiseRejectionTracker(JSGlobalObject * jsGlobalObject, JSC::ExecState* exec, JSC::JSPromise* promise, JSC::JSPromiseRejectionOperation operation);

	static JSC::EncodedJSValue JSC_HOST_CALL objectProtoGetter(JSC::ExecState* exec);
	static JSC::EncodedJSValue JSC_HOST_CALL objectProtoSetter(JSC::ExecState* exec);

	static JSC::EncodedJSValue JSC_HOST_CALL errorConstructorCaptureStackTrace(JSC::ExecState* exec);
	static void createCallSitesFromFrames(JSC::ExecState * exec, JSC::ObjectInitializationScope& objectScope, JSCStackTrace& stackTrace, JSC::JSArray * callSites);
	JSC::JSValue formatStackTrace(JSC::VM& vm, JSC::ExecState * exec, JSC::JSObject * errorObject, JSC::JSArray * callSites);
};

}} // v8::jscshim