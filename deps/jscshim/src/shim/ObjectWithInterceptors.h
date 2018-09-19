/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include <v8.h>
#include "Object.h"
#include "ObjectTemplate.h"
#include "InterceptorInfo.h"

namespace v8 { namespace jscshim
{

class ObjectWithInterceptors : public jscshim::Object {
private:
	/* A helper for creating and passing a local v8::PropertyCallbackInfo instance and a JSValue return value.
	 * Regarding "this" and "holder" values, From v8's docs: 
	 * "Suppose you have `x` and its prototype is `y`, and `y` has an interceptor. Then `info.This()` is `x` and `info.Holder()` is `y`". 
	 * So, for us:
	 * - "object" points to the object that actually have the accessor (JSObject::getPropertySlot\getNonIndexPropertySlot walk up the prototype chain, calling 
	 *   each object's getOwnPropertySlotByIndex\getOwnNonIndexPropertySlot).
	 * - The property slot's "thisValue" points to the object the caller is actually querying, which is "this" value.
	 *   In case we don't have a property slot, we'll use "object" as "this" also (in deleteProperty for example). */
	template <typename ReturnValue = v8::Value>
	struct CallbackCall
	{
		CallbackCall(JSC::ExecState * exec, ObjectWithInterceptors * holder, JSC::PropertySlot& slot, JSC::JSValue callbackData) :
			CallbackCall(holder, slot.thisValue(), callbackData)
		{
			JSC::CodeBlock * codeBlock = exec->codeBlock();
			if (codeBlock)
			{
				m_callbackInfo.shouldThrowOnError_ = codeBlock->isStrictMode();
			}
		}

		CallbackCall(JSC::ExecState * exec, ObjectWithInterceptors * holder, JSC::PutPropertySlot& slot, JSC::JSValue callbackData) :
			CallbackCall(holder, slot.thisValue(), callbackData, slot.isStrictMode())
		{
		}

		CallbackCall(JSC::ExecState * exec, ObjectWithInterceptors * holder, const JSC::JSValue& thisValue, JSC::JSValue callbackData, bool shouldThrow) :
			CallbackCall(holder, thisValue, callbackData, shouldThrow)
		{
		}

		CallbackCall(JSC::ExecState * exec, ObjectWithInterceptors * holder, const JSC::JSValue& thisValue, JSC::JSValue callbackData) :
			CallbackCall(holder, thisValue, callbackData)
		{
			JSC::CodeBlock * codeBlock = exec->codeBlock();
			if (codeBlock)
			{
				m_callbackInfo.shouldThrowOnError_ = codeBlock->isStrictMode();
			}
		}

		JSC::JSValue m_returnValue;
		v8::PropertyCallbackInfo<ReturnValue> m_callbackInfo;

	private:
		CallbackCall(ObjectWithInterceptors * object, const JSC::JSValue& thisValue, JSC::JSValue callbackData, bool shouldThrow = false) :
			m_callbackInfo(Local<v8::Object>(thisValue),
						   Local<v8::Object>(JSC::JSValue(object)),
						   Local<v8::Value>(callbackData),
						   jscshim::GetV8ContextForObject(object)->GetIsolate(),
						   &m_returnValue,
						   shouldThrow)
		{
		}
	};

private:
	NamedInterceptorInfo * m_namedInterceptors;
	IndexedInterceptorInfo * m_indexedInterceptors;

public:
	typedef jscshim::Object Base;

	// TODO: Make sure we really need ProhibitsPropertyCaching
	static const unsigned StructureFlags = Base::StructureFlags | 
										   JSC::OverridesGetOwnPropertySlot | 
										   JSC::InterceptsGetOwnPropertySlotByIndexEvenWhenLengthIsNotZero | 
										   JSC::OverridesGetPropertyNames | 
										   JSC::ProhibitsPropertyCaching;

	static ObjectWithInterceptors* create(JSC::VM&				 vm, 
										  JSC::Structure		 * structure, 
										  ObjectTemplate		 * objectTemplate, 
										  NamedInterceptorInfo	 * namedInterceptors, 
										  IndexedInterceptorInfo * indexedInterceptors)
	{
		ObjectWithInterceptors* cell = new (NotNull, JSC::allocateCell<ObjectWithInterceptors>(vm.heap)) ObjectWithInterceptors(vm, 
																																structure, 
																																objectTemplate->m_internalFieldCount, 
																																namedInterceptors, 
																																indexedInterceptors);
		cell->finishCreation(vm, objectTemplate);
		return cell;
	}

	DECLARE_INFO;

	static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype, bool isCallable)
	{
		unsigned int flags = isCallable ? (StructureFlags | JSC::OverridesGetCallData ) : StructureFlags;
		JSC::Structure * result = JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, flags), info());
		
		/* Disable quick property access for enumeration, like JSC's ProxyObject does. This will ensure 
		 * JSC::propertyNameEnumerator will call our getPropertyNames. */
		result->setIsQuickPropertyAccessAllowedForEnumeration(false);

		return result;
	}

private:
	ObjectWithInterceptors(JSC::VM&				  vm, 
						   JSC::Structure		  * structure, 
						   int					  internalFieldCount, 
						   NamedInterceptorInfo   * namedInterceptors, 
						   IndexedInterceptorInfo * indexedInterceptors) : Base(vm, structure, internalFieldCount),
		m_namedInterceptors(namedInterceptors),
		m_indexedInterceptors(indexedInterceptors)
	{
		// If we don't have any interceptors we shouldn't be used (use jscshim::Object directly)
		ASSERT(namedInterceptors || indexedInterceptors);
	}

	static bool getOwnPropertySlot(JSC::JSObject * object, JSC::ExecState* exec, JSC::PropertyName propertyName, JSC::PropertySlot& slot);
	static bool getOwnPropertySlotByIndex(JSC::JSObject * object, JSC::ExecState* exec, unsigned propertyName, JSC::PropertySlot& slot);

	static bool put(JSC::JSCell* cell, JSC::ExecState* exec, JSC::PropertyName propertyName, JSC::JSValue value, JSC::PutPropertySlot& slot);
	static bool putByIndex(JSC::JSCell* cell, JSC::ExecState* exec, unsigned propertyName, JSC::JSValue value, bool shouldThrow);

	static bool deleteProperty(JSC::JSCell * cell, JSC::ExecState* exec, JSC::PropertyName propertyName);
	static bool deletePropertyByIndex(JSC::JSCell * cell, JSC::ExecState * exec, unsigned propertyName);
	
	static bool defineOwnProperty(JSC::JSObject					 * object,
								  JSC::ExecState				 * exec, 
								  JSC::PropertyName				 propertyName, 
								  const JSC::PropertyDescriptor& descriptor, 
								  bool							 shouldThrow);

	static void getOwnPropertyNames(JSC::JSObject * object, JSC::ExecState * exec, JSC::PropertyNameArray& propertyNames, JSC::EnumerationMode mode);
	static void getPropertyNames(JSC::JSObject * object, JSC::ExecState * exec, JSC::PropertyNameArray& propertyNames, JSC::EnumerationMode mode);
	static void getOwnNonIndexPropertyNames(JSC::JSObject *, JSC::ExecState *, JSC::PropertyNameArray&, JSC::EnumerationMode);

	static uint32_t getEnumerableLength(JSC::ExecState * exec, JSC::JSObject * object);
	
	// Taken from JSC's ProxyObject
	static NO_RETURN_DUE_TO_CRASH void getStructurePropertyNames(JSC::JSObject *, JSC::ExecState *, JSC::PropertyNameArray&, JSC::EnumerationMode);
	static NO_RETURN_DUE_TO_CRASH void getGenericPropertyNames(JSC::JSObject *, JSC::ExecState *, JSC::PropertyNameArray&, JSC::EnumerationMode);

	bool shouldSkipNonIndexedInterceptor(JSC::ExecState * exec, const JSC::PropertyName& propertyName);

	template <typename InterceptorsType, typename JscPropertyNameType, typename v8PropertyNameType, typename DefaultHandlerType>
	bool performGetProperty(JSC::ExecState			   * exec, 
							InterceptorsType		   * interceptors, 
							const JscPropertyNameType& propertyName,
							v8PropertyNameType		   v8PropertyName, 
							JSC::PropertySlot&		   slot,
							DefaultHandlerType		   * defaultHandler);

	template <typename InterceptorsType, typename PropertyNameType>
	bool performGetInterceptor(JSC::ExecState	  * exec, 
							   InterceptorsType   * interceptors, 
							   PropertyNameType   v8PropertyName,
							   JSC::PropertySlot& slot);

	template <typename InterceptorsType, typename PropertyNameType>
	bool performGetOwnPropertyInterceptor(JSC::ExecState     * exec,
										  InterceptorsType   * interceptors, 
										  PropertyNameType   v8PropertyName,
										  JSC::PropertySlot& slot);

	template <typename InterceptorsType, typename PropertyNameType>
	bool performHasProperty(JSC::ExecState	   * exec, 
							InterceptorsType   * interceptors, 
							PropertyNameType   v8PropertyName,
							JSC::PropertySlot& slot);

	template <typename InterceptorsType, typename PropertyNameType>
	unsigned int GetPropertyAttributes(InterceptorsType		    * interceptors, 
									   PropertyNameType		    v8PropertyName, 
									   CallbackCall<v8::Value>& callbackCall, 
									   bool					    alreadySuccessfullyCalledGetter, 
									   bool					    * wasAbsent = nullptr);

	/* Note that here we're using two overloads instead of a template since the "default handler" is a 
	 * memeber function and not a static one */
	bool performDefineOwnProperty(JSC::ExecState				  * exec,
								  const JSC::PropertyName&		  propertyName, 
								  const JSC::PropertyDescriptor&  descriptor,
								  bool							  shouldThrow);
	bool performDefineOwnProperty(JSC::ExecState				  * exec,
								  unsigned						  index, 
								  const JSC::PropertyDescriptor&  descriptor,
								  bool							  shouldThrow);
};

}} // v8::jscshim