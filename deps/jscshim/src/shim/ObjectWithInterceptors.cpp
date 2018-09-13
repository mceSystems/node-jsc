/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "ObjectWithInterceptors.h"
#include "FunctionTemplate.h"
#include "helpers.h"

#include <JavaScriptCore/Identifier.h>
#include <JavaScriptCore/Symbol.h>
#include <JavaScriptCore/CodeBlock.h>
#include <JavaScriptCore/ObjectConstructor.h>
#include <JavaScriptCore/CatchScope.h>
#include <JavaScriptCore/ArrayStorage.h>
#include <JavaScriptCore/JSCInlines.h>

namespace JSC { namespace jscshim
{

/* Copied from JSC's JSObject::getOwnPropertyNames. See ObjectWithInterceptors::getOwnPropertyNames' documentation
 * below for more information. 
 * Note that this sits in the JSC namespace because of the ALL_* macros used in the switch-case and use consts
 * from JSC wihtout the namespace prefix. */
void getObjectOwnIndexPropertyNames(v8::jscshim::ObjectWithInterceptors	* object,
									ExecState							* exec,
									PropertyNameArray&					propertyNames,
									EnumerationMode						mode)
{
	switch (object->indexingType()) {
		case ALL_BLANK_INDEXING_TYPES:
		case ALL_UNDECIDED_INDEXING_TYPES:
			break;
			
		case ALL_INT32_INDEXING_TYPES:
		case ALL_CONTIGUOUS_INDEXING_TYPES: {
			Butterfly* butterfly = object->butterfly();
			unsigned usedLength = butterfly->publicLength();
			for (unsigned i = 0; i < usedLength; ++i) {
				if (!butterfly->contiguous().at(object, i))
					continue;
				propertyNames.add(i);
			}
			break;
		}
			
		case ALL_DOUBLE_INDEXING_TYPES: {
			Butterfly* butterfly = object->butterfly();
			unsigned usedLength = butterfly->publicLength();
			for (unsigned i = 0; i < usedLength; ++i) {
				double value = butterfly->contiguousDouble().at(object, i);
				if (value != value)
					continue;
				propertyNames.add(i);
			}
			break;
		}
			
		case ALL_ARRAY_STORAGE_INDEXING_TYPES: {
			ArrayStorage* storage = object->butterfly()->arrayStorage();
			
			unsigned usedVectorLength = std::min(storage->length(), storage->vectorLength());
			for (unsigned i = 0; i < usedVectorLength; ++i) {
				if (storage->m_vector[i])
					propertyNames.add(i);
			}
			
			if (SparseArrayValueMap* map = storage->m_sparseMap.get()) {
				Vector<unsigned, 0, UnsafeVectorOverflow> keys;
				keys.reserveInitialCapacity(map->size());
				
				SparseArrayValueMap::const_iterator end = map->end();
				for (SparseArrayValueMap::const_iterator it = map->begin(); it != end; ++it) {
					if (mode.includeDontEnumProperties() || !(it->value.attributes & PropertyAttribute::DontEnum))
						keys.uncheckedAppend(static_cast<unsigned>(it->key));
				}
				
				std::sort(keys.begin(), keys.end());
				for (unsigned i = 0; i < keys.size(); ++i)
					propertyNames.add(keys[i]);
			}
			break;
		}
			
		default:
			RELEASE_ASSERT_NOT_REACHED();
	}
}

}} // JSC::jscshim

namespace v8 { namespace jscshim
{
	
const JSC::ClassInfo ObjectWithInterceptors::s_info = { "JSCShimObjectWithInterceptors", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(ObjectWithInterceptors) };

bool ObjectWithInterceptors::getOwnPropertySlot(JSC::JSObject * object, 
												JSC::ExecState * exec, 
												JSC::PropertyName propertyName, 
												JSC::PropertySlot& slot)
{
	ObjectWithInterceptors * thisObject = JSC::jsCast<ObjectWithInterceptors *>(object);

	// Handle index based property access
	std::optional<uint32_t> index = parseIndex(propertyName);
	if (index)
	{
		return thisObject->performGetProperty(exec, 
											  thisObject->m_indexedInterceptors, 
											  index.value(),
											  index.value(), 
											  slot, 
											  Base::getOwnPropertySlotByIndex);
	}

	// Skip named property interception if this is a symbol property and kOnlyInterceptStrings flag is set
	if (thisObject->shouldSkipNonIndexedInterceptor(exec, propertyName))
	{
		return Base::getOwnPropertySlot(object, exec, propertyName, slot);
	}

	/* Named property access.
	 * Note tht internally, JSObject::getOwnPropertySlot calls thisObject->getOwnNonIndexPropertySlot without
	 * calling parseIndex first (only asserts), so parseIndex won't be called twice (on realease builds). */
	return thisObject->performGetProperty(exec, 
										  thisObject->m_namedInterceptors,
										  propertyName,
										  Local<Name>(JscPropertyNameToV8Name(exec, propertyName)), 
										  slot,
										  Base::getOwnPropertySlot);
}

bool ObjectWithInterceptors::getOwnPropertySlotByIndex(JSC::JSObject* object, JSC::ExecState* exec, unsigned propertyName, JSC::PropertySlot& slot)
{
	ObjectWithInterceptors * thisObject = JSC::jsCast<ObjectWithInterceptors *>(object);
	return thisObject->performGetProperty(exec, 
										  thisObject->m_indexedInterceptors, 
										  propertyName, 
										  propertyName,
										  slot, 
										  Base::getOwnPropertySlotByIndex);
}

/* TODO: When falling back to JSObject::put, internally it will call parseIndex again, but currently there's no way
 * to let it know it's not an index (there is no "putNonIndexed"). Should we change that? */
bool ObjectWithInterceptors::put(JSC::JSCell* cell, JSC::ExecState* exec, JSC::PropertyName propertyName, JSC::JSValue value, JSC::PutPropertySlot& slot)
{
	// Handle index based property
	std::optional<uint32_t> index = parseIndex(propertyName);
	if (index)
	{
		return putByIndex(cell, exec, index.value(), value, slot.isStrictMode());
	}

	// Handle name based property, skipping property interception if this is a symbol property and kOnlyInterceptStrings flag is set
	ObjectWithInterceptors * thisObject = JSC::jsCast<ObjectWithInterceptors *>(cell);
	NamedInterceptorInfo * interceptors = thisObject->m_namedInterceptors;
	if ((nullptr == interceptors) || (nullptr == interceptors->setter) || thisObject->shouldSkipNonIndexedInterceptor(exec, propertyName))
	{
		return Base::put(cell, exec, propertyName, value, slot);
	}

	CallbackCall<v8::Value> callbackCall(exec, thisObject, slot, interceptors->data);
	interceptors->setter(Local<Name>(JscPropertyNameToV8Name(exec, propertyName)),
						 Local<Value>(value),
						 callbackCall.m_callbackInfo);

	// In v8, setters should return the value to indicate they've intercepted the request
	if (callbackCall.m_returnValue == value)
	{
		return true;
	}

	// Setter didn't intercept the call
	return Base::put(cell, exec, propertyName, value, slot);
}

bool ObjectWithInterceptors::putByIndex(JSC::JSCell* cell, JSC::ExecState* exec, unsigned propertyName, JSC::JSValue value, bool shouldThrow)
{
	ObjectWithInterceptors * thisObject = JSC::jsCast<ObjectWithInterceptors *>(cell);

	IndexedInterceptorInfo * interceptors = thisObject->m_indexedInterceptors;
	if ((nullptr == interceptors) || (nullptr == interceptors->setter))
	{
		return Base::putByIndex(cell, exec, propertyName, value, shouldThrow);
	}

	CallbackCall<v8::Value> callbackCall(exec, thisObject, JSC::JSValue(thisObject), interceptors->data, shouldThrow);
	interceptors->setter(propertyName, Local<Value>(value), callbackCall.m_callbackInfo);

	// In v8, setters should return the value to indicate they've intercepted the request
	if (callbackCall.m_returnValue == value)
	{
		return true;
	}

	// Setter didn't intercept the call
	return Base::putByIndex(cell, exec, propertyName, value, shouldThrow);
}

/* TODO: When falling back to JSObject::deleteProperty, internally will call parseIndex again, but currently there's no way
 * to let it know it's not an index (there is no "putNonIndexed"). Should we change that? */
bool ObjectWithInterceptors::deleteProperty(JSC::JSCell * cell, JSC::ExecState* exec, JSC::PropertyName propertyName)
{
	// Handle index based property
	std::optional<uint32_t> index = parseIndex(propertyName);
	if (index)
	{
		return deletePropertyByIndex(cell, exec, index.value());
	}

	// Handle name based property, skipping property interception if this is a symbol property and kOnlyInterceptStrings flag is set
	ObjectWithInterceptors * thisObject = JSC::jsCast<ObjectWithInterceptors *>(cell);
	NamedInterceptorInfo * interceptors = thisObject->m_namedInterceptors;
	if ((nullptr == interceptors) || (nullptr == interceptors->deleter) || thisObject->shouldSkipNonIndexedInterceptor(exec, propertyName))
	{
		return Base::deleteProperty(cell, exec, propertyName);
	}

	// Call the "deleter" interceptor
	CallbackCall<v8::Boolean> callbackCall(exec, thisObject, JSC::JSValue(thisObject), interceptors->data);
	interceptors->deleter(Local<Name>(JscPropertyNameToV8Name(exec, propertyName)), callbackCall.m_callbackInfo);

	/* In v8, deleters should return a boolean value when they intercept the
	* the call. That should be used as the return value of "delete". */
	if (callbackCall.m_returnValue.isBoolean())
	{
		return callbackCall.m_returnValue.asBoolean();
	}

	// The deleter didn't intercept the call
	return Base::deleteProperty(cell, exec, propertyName);
}

bool ObjectWithInterceptors::deletePropertyByIndex(JSC::JSCell * cell, JSC::ExecState * exec, unsigned propertyName)
{
	ObjectWithInterceptors * thisObject = JSC::jsCast<ObjectWithInterceptors *>(cell);

	IndexedInterceptorInfo * interceptors = thisObject->m_indexedInterceptors;
	if ((nullptr == interceptors) || (nullptr == interceptors->deleter))
	{
		return Base::deletePropertyByIndex(cell, exec, propertyName);
	}

	// Call the "deleter" interceptor
	CallbackCall<v8::Boolean> callbackCall(exec, thisObject, JSC::JSValue(thisObject), interceptors->data);
	interceptors->deleter(propertyName, callbackCall.m_callbackInfo);

	/* In v8, deleters should return a boolean value when they intercept the
	 * the call. That should be used as the return value of "delete". */
	if (callbackCall.m_returnValue.isBoolean())
	{
		return callbackCall.m_returnValue.asBoolean();
	}

	// The deleter didn't intercept the call
	return Base::deletePropertyByIndex(cell, exec, propertyName);
}

bool ObjectWithInterceptors::defineOwnProperty(JSObject						  * object,
											   JSC::ExecState				  * exec, 
											   JSC::PropertyName			  propertyName, 
											   const JSC::PropertyDescriptor& descriptor,
											   bool							  shouldThrow)
{
	ObjectWithInterceptors * thisObject = JSC::jsCast<ObjectWithInterceptors *>(object);

	// Handle index based property
	std::optional<uint32_t> index = parseIndex(propertyName);
	if (index)
	{
		return thisObject->performDefineOwnProperty(exec,
													index.value(), 
													descriptor,
													shouldThrow);
	}

	// Named based property
	return thisObject->performDefineOwnProperty(exec,
												propertyName,
												descriptor,
												shouldThrow);
}

bool ObjectWithInterceptors::shouldSkipNonIndexedInterceptor(JSC::ExecState * exec, const JSC::PropertyName& propertyName)
{
	if (nullptr == m_namedInterceptors)
	{
		return true;
	}

	int interceptorflags = static_cast<int>(m_namedInterceptors->flags);
	
	// Skip if this is a symbol and we're only interested in string properties
	if (interceptorflags & static_cast<int>(v8::PropertyHandlerFlags::kOnlyInterceptStrings))
	{
		if (propertyName.isSymbol())
		{
			return true;
		}
	}

	// If this is a non-masking interceptor, we'll skip it if the property exists
	if (interceptorflags & static_cast<int>(v8::PropertyHandlerFlags::kNonMasking))
	{
		JSC::PropertySlot slot(this, JSC::PropertySlot::InternalMethodType::GetOwnProperty);
		return Base::getOwnPropertySlot(this, exec, propertyName, slot);
	}

	return false;
}

void ObjectWithInterceptors::getStructurePropertyNames(JSC::JSObject *, JSC::ExecState *, JSC::PropertyNameArray&, JSC::EnumerationMode)
{
	// Like JSC's ProxyObject, we don't need getStructurePropertyNames (which is only used by JSC::propertyNameEnumerator)
	RELEASE_ASSERT_NOT_REACHED();
}

void ObjectWithInterceptors::getGenericPropertyNames(JSC::JSObject *, JSC::ExecState *, JSC::PropertyNameArray&, JSC::EnumerationMode)
{
	// Like JSC's ProxyObject, we don't need getGenericPropertyNames (which is only used by JSC::propertyNameEnumerator)
	RELEASE_ASSERT_NOT_REACHED();
}

/* In JSC, there is no clear seperation between enumeration of named\symbol and numeric properties in JSObject:
 * - JSObject::getOwnPropertyNames seem to handle numeric properties itself, while it calls 
 *   getOwnNonIndexPropertyNames for the named\symbol ones. 
 * - In JSObject, getOwnNonIndexPropertyNames will get properties from the class and structure of the object.
 * - Different classes override different functions. JSArray, for example, overrides getOwnNonIndexPropertyNames,
 *   but ProxyObject seems to handle everything in ProxyObject::performGetOwnPropertyNames (it's getOwnNonIndexPropertyNames
 *   just asserts). 
 *
 * It seems that when enumerating keys, both JSC and v8 will return numeric keys first, followed by
 * named\symbol properties. In v8, following the flow of KeyAccumulator::CollectOwnKeys in keys.cc, 
 * it seems to organize to build the list of keys from 4 parts, in the following order:
 * 1. Numeric keys from the object.
 * 2. Numeric keys from the interceptor, if present.
 * 3. Non-indexed keys from the object
 * 4. Non-indexed keys from the interceptor, if present.
 *
 * If we want to follow v8's order, we have a problem with JSObject::getOwnPropertyNames, since we want to
 * insert the numeric interceptor's keys between the object's numeric and non-indexed keys. This is not 
 * possible in the current JSObject::getOwnPropertyNames, which handles numeric keys itself. For now,
 * it seems that the simplest solution, without modifying JSC is to just copy the numeric keys handling from 
 * JSObject::getOwnPropertyNames. It's pretty straight forward, but still an ugly solution.
 * 
 * TODO: Find a better solution, or commit getOwnIndexPropertyNames to JSC
 * TODO: Should we sort?
 */
void ObjectWithInterceptors::getOwnPropertyNames(JSC::JSObject			 * object, 
												 JSC::ExecState			 * exec, 
												 JSC::PropertyNameArray& propertyNames, 
												 JSC::EnumerationMode	 mode)
{
	auto scope = DECLARE_CATCH_SCOPE(exec->vm());
	ObjectWithInterceptors * thisObject = JSC::jsCast<ObjectWithInterceptors *>(object);

	// First, start with numeric properties
	JSC::jscshim::getObjectOwnIndexPropertyNames(thisObject, exec, propertyNames, mode);

	// If we have an "indexed" enumerator - get it's indices
	auto enumerator = thisObject->m_indexedInterceptors ? thisObject->m_indexedInterceptors->enumerator : nullptr;
	if (enumerator)
	{
		// Call the interceptor
		CallbackCall<v8::Array> callbackCall(exec, thisObject, JSC::JSValue(thisObject), thisObject->m_indexedInterceptors->data);
		enumerator(callbackCall.m_callbackInfo);

		/* Add the indices we've got from the interceptor. The interceptor retuns an "array like" object containing
		 * indices.
		 * Based on JSC::createListFromArrayLike (JSObjectInlines.h) and JSC::ProxyObject::performGetOwnPropertyNames. */

		uint64_t length = GetArrayLikeObjectLength(exec, callbackCall.m_returnValue);
		if (0 == length)
		{
			return;
		}

		for (uint64_t index = 0; index < length; index++)
		{
			JSC::JSValue currentKey = callbackCall.m_returnValue.get(exec, index);

			// Cast the key to an unsigned integer
			uint32_t currentKeyIndexValue = currentKey.toIndex(exec, "interceptor value");
			RETURN_IF_EXCEPTION(scope, void());
			
			// Add the (key) index to the properties array
			propertyNames.add(currentKeyIndexValue);
			RETURN_IF_EXCEPTION(scope, void());
		}
	}

	// Get non-indexed (string\symbol) properties from the object and interceptor
	ObjectWithInterceptors::getOwnNonIndexPropertyNames(object, exec, propertyNames, mode);
}

/* Like in v8's KeyAccumulator::CollectOwnPropertyNames (keys.cc), we'll collect the object's
 * keys, then get additional keys from the interceptor. Note that in v8, CollectOwnPropertyNames
 * will add strings first, than symbols, which we currently won't do in this order. */
void ObjectWithInterceptors::getOwnNonIndexPropertyNames(JSC::JSObject			 * object, 
														 JSC::ExecState			 * exec, 
														 JSC::PropertyNameArray& propertyNames, 
														 JSC::EnumerationMode	 mode)
{
	auto scope = DECLARE_CATCH_SCOPE(exec->vm());
	
	// Add the object's properties
	Base::getOwnNonIndexPropertyNames(object, exec, propertyNames, mode);

	// If we have a named enumerator - get it's keys
	ObjectWithInterceptors * thisObject = JSC::jsCast<ObjectWithInterceptors *>(object);
	auto enumerator = thisObject->m_namedInterceptors->enumerator;
	if (nullptr == enumerator)
	{
		// Nothing more to do here
		return;
	}

	// Call the interceptor
	CallbackCall<v8::Array> callbackCall(exec, thisObject, JSC::JSValue(thisObject), thisObject->m_namedInterceptors->data);
	enumerator(callbackCall.m_callbackInfo);

	/* Add the keys we've got from the interceptor. The interceptor retuns an "array like" object containing 
	 * keys (symbols/strings).
	 * Based on JSC::createListFromArrayLike (JSObjectInlines.h) and JSC::ProxyObject::performGetOwnPropertyNames. */

	uint64_t length = GetArrayLikeObjectLength(exec, callbackCall.m_returnValue);
	if (0 == length)
	{
		return;
	}

	bool includeSymbols = propertyNames.includeSymbolProperties();
	bool includeStrings = propertyNames.includeStringProperties();
	for (uint64_t index = 0; index < length; index++)
	{
		JSC::JSValue currentKey = callbackCall.m_returnValue.get(exec, index);

		if ((currentKey.isSymbol() && includeSymbols) ||
			(currentKey.isString() && includeStrings))
		{
			JSC::Identifier keyIdentifier = currentKey.toPropertyKey(exec);
			RETURN_IF_EXCEPTION(scope, void());
			propertyNames.addUnchecked(keyIdentifier.impl());
		}
	}
}

// TODO: Do we really have to override it? (JSC's ProxyObject does)
void ObjectWithInterceptors::getPropertyNames(JSC::JSObject			  * object, 
											  JSC::ExecState		  * exec, 
											  JSC::PropertyNameArray& propertyNameArray, 
											  JSC::EnumerationMode    enumerationMode)
{
	Base::getPropertyNames(object, exec, propertyNameArray, enumerationMode);
}

// TODO: Do we need to handle it and call our numeric interceptor? JSC::ProxyObject doesn't even override this
uint32_t ObjectWithInterceptors::getEnumerableLength(JSC::ExecState * exec, JSC::JSObject * object)
{
	return Base::getEnumerableLength(exec, object);
}

template <typename InterceptorsType, typename JscPropertyNameType, typename v8PropertyNameType, typename DefaultHandlerType>
bool ObjectWithInterceptors::performGetProperty(JSC::ExecState			   * exec,
												InterceptorsType		   * interceptors,
												const JscPropertyNameType& propertyName,
												v8PropertyNameType		   v8PropertyName, 
												JSC::PropertySlot&		   slot,
												DefaultHandlerType		   * defaultHandler)
{
	if (nullptr == interceptors)
	{
		return defaultHandler(this, exec, propertyName, slot);
	}

	/* JSC's getOwnPropertySlot\getOwnPropertySlotByIndex doesn't exactly match
	 * v8's interceptors api. Different functions in JSC call them with different
	 * slot "internalMethodType", but looking at JSC::ProxyObject it seem to:
	 * - Pass InternalMethodType::Get to the proxy's getter.
	 * - Pass InternalMethodType::GetOwnProperty to the proxy's getOwnPropertyDescriptor.
	 * So we'll follow the same general behavior. 
	 * Regarding InternalMethodType::HasProperty, see performHasProperty's documentation.
	 *
	 * TODO: Should we handle InternalMethodType::VMInquiry? */
	bool intercepted = false;
	switch (slot.internalMethodType())
	{
	case JSC::PropertySlot::InternalMethodType::Get:
		intercepted = performGetInterceptor(exec, interceptors, v8PropertyName, slot);
		break;

	case JSC::PropertySlot::InternalMethodType::GetOwnProperty:
		intercepted = performGetOwnPropertyInterceptor(exec, interceptors, v8PropertyName, slot);
		break;

	case JSC::PropertySlot::InternalMethodType::HasProperty:
		intercepted = performHasProperty(exec, interceptors, v8PropertyName, slot);
		break;
	}

	// TODO: Should we always disable caching?
	if (intercepted)
	{
		// Disable caching of our this property. This is what JSC's ProxyObject::getOwnPropertySlotCommon does.
		slot.disableCaching();
		return true;
	}

	return defaultHandler(this, exec, propertyName, slot);
}

template <typename InterceptorsType, typename PropertyNameType>
bool ObjectWithInterceptors::performGetInterceptor(JSC::ExecState	  * exec, 
												   InterceptorsType   * interceptors, 
												   PropertyNameType   v8PropertyName,
												   JSC::PropertySlot& slot)
{
	/* Note the we shouldn't throw errors on "get" operations (follows v8 GetPropertyWithInterceptorInternal, 
	 * in objects.cc) */
	CallbackCall<v8::Value> callbackCall(exec, this, slot.thisValue(), interceptors->data, false);
	
	// Invoke the getter interceptor
	interceptors->getter(v8PropertyName, callbackCall.m_callbackInfo);

	if (!callbackCall.m_returnValue)
	{
		return false;
	}

	// TODO: make sure it's ok not to pass the attributes here (this is what JSC's ProxyObject::performGet does)
	slot.setValue(this, 0, callbackCall.m_returnValue);
	return true;
}

/* This was "inspired" by JSC's ProxyObject::performInternalMethodGetOwnProperty. 
 * Note that there is one major difference between JSC in v8 here: JSC::JSObject::hasOwnProperty 
 * (the variants without the slot argument) will pass InternalMethodType::GetOwnProperty to 
 * getOwnPropertySlot, which means we can't always tell the difference between "get" and "has"
 * operations. This means two things:
 * - We might call different user callbacks than v8 on these situtations. This should be OK, 
 *   as we're free to call the user callbacks when we need them, but might affect situations
 *	 were the getter and query interceptors have different behavior. We'll follow v8's logic
 *   as much as we can.
 * - We might get here when there's no getter interceptor, only a query interceptor. This happens
 *   in v8's unit tests, and is legal. 
 */
template <typename InterceptorsType, typename PropertyNameType>
bool ObjectWithInterceptors::performGetOwnPropertyInterceptor(JSC::ExecState	 * exec,
															  InterceptorsType   * interceptors, 
															  PropertyNameType   v8PropertyName,
															  JSC::PropertySlot& slot)
{
	auto scope = DECLARE_CATCH_SCOPE(exec->vm());
	CallbackCall<v8::Value> callbackCall(exec, this, slot, interceptors->data);
	
	// If we have a getOwnPropertyDescriptor interceptor - use it
	if (interceptors->descriptor)
	{
		interceptors->descriptor(v8PropertyName, callbackCall.m_callbackInfo);
		if (!callbackCall.m_returnValue)
		{
			return false;
		}

		/* The getOwnPropertyDescriptor interceptor should return an object that can be
		 * converted to a PropertyDescriptor */
		JSC::PropertyDescriptor propertyDescriptor;
		JSC::toPropertyDescriptor(exec, callbackCall.m_returnValue, propertyDescriptor);
		RETURN_IF_EXCEPTION(scope, false);

		if (propertyDescriptor.isAccessorDescriptor())
		{
			JSC::GetterSetter* getterSetter = propertyDescriptor.slowGetterSetter(exec);
			RETURN_IF_EXCEPTION(scope, false);

			slot.setGetterSlot(this, propertyDescriptor.attributes(), getterSetter);
		}
		else if (propertyDescriptor.isDataDescriptor() && !propertyDescriptor.value().isEmpty())
		{
			slot.setValue(this, propertyDescriptor.attributes(), propertyDescriptor.value());
		}
		else
		{
			// From JSC: We use undefined because it's the default value in object properties.
			slot.setValue(this, propertyDescriptor.attributes(), JSC::jsUndefined());
		}

		return true;
	}

	/* We don't have a getOwnPropertyDescriptor interceptor, so we'll need to use the 
	 * getter\query interceptors.
	 * Note that according to v8's HasOwnProperty unit test, query interceptor "wins" on disagreement
	 * with the getter interceptor, meaning that if the query interceptor didn't intercept the call
	 * we won't continue to the getter. */
	if (interceptors->query)
	{
		bool wasAbsent = false;
		unsigned int propertyAttributes = GetPropertyAttributes(interceptors, v8PropertyName, callbackCall, false, &wasAbsent);
		if (wasAbsent)
		{
			return false;
		}

		callbackCall.m_returnValue = JSC::jsUndefined();
		if (interceptors->getter)
		{
			interceptors->getter(v8PropertyName, callbackCall.m_callbackInfo);
		}

		slot.setValue(this, propertyAttributes, callbackCall.m_returnValue);
		return true;
	}
	
	if (interceptors->getter)
	{
		interceptors->getter(v8PropertyName, callbackCall.m_callbackInfo);

		if (!callbackCall.m_returnValue)
		{
			return false;
		}

		// See ObjectWithInterceptors::GetPropertyAttributes's documentation below for why we're using DontEnum
		slot.setValue(this, static_cast<unsigned int>(JSC::PropertyAttribute::DontEnum), callbackCall.m_returnValue);
		return true;
	}

	// We don't have any relevant interceptor
	return false;
}

/* When using interceptors, v8's JSReceiver::HasProperty uses JSObject::GetPropertyAttributesWithInterceptor,
 * which GetPropertyAttributes mimics */
template <typename InterceptorsType, typename PropertyNameType>
bool ObjectWithInterceptors::performHasProperty(JSC::ExecState	   * exec,
												InterceptorsType   * interceptors, 
												PropertyNameType   v8PropertyName,
												JSC::PropertySlot& slot)
{
	CallbackCall<v8::Value> callbackCall(exec, this, slot, interceptors->data);

	bool wasAbsent = false;
	GetPropertyAttributes(interceptors, v8PropertyName, callbackCall, false, &wasAbsent);

	return (false == wasAbsent);
}

/* Looking at v8's GetPropertyAttributesWithInterceptorInternal in objects.cc, we'll follow the same flow:
 * - If we have a "query" interceptor - use it.
 * - If not and we have a getter interceptor, if the getter returns a value - return "DontEnum" */
template <typename InterceptorsType, typename PropertyNameType>
unsigned int ObjectWithInterceptors::GetPropertyAttributes(InterceptorsType		    * interceptors, 
														   PropertyNameType		    v8PropertyName, 
														   CallbackCall<v8::Value>& callbackCall,
														   bool					    alreadySuccessfullyCalledGetter,
														   bool					    * wasAbsent)
{
	if (wasAbsent)
	{
		*wasAbsent = true;
	}

	// Try the "query" interceptor first
	v8::PropertyAttribute propertyAttributes = v8::PropertyAttribute::None;
	if (interceptors->query)
	{
		/* Note that casting v8::PropertyCallbackInfo<v8::Integer> to v8::PropertyCallbackInfo<v8::Value>
		 * is legal since the return value is always a JSValue for us. */
		callbackCall.m_returnValue = JSC::JSValue();
		interceptors->query(v8PropertyName, *reinterpret_cast<v8::PropertyCallbackInfo<v8::Integer> *>(&callbackCall.m_callbackInfo));
		if (callbackCall.m_returnValue.isInt32())
		{
			propertyAttributes = static_cast<v8::PropertyAttribute>(callbackCall.m_returnValue.asInt32());
			if (wasAbsent)
			{
				*wasAbsent = false;
			}
			
		}
	}
	// Try to fallback to the getter
	else if (interceptors->getter)
	{
		// Only call the getter if the caller had't already done that
		if (false == alreadySuccessfullyCalledGetter)
		{
			callbackCall.m_returnValue = JSC::JSValue();
			interceptors->getter(v8PropertyName, callbackCall.m_callbackInfo);
			alreadySuccessfullyCalledGetter = !callbackCall.m_returnValue.isEmpty();
		}

		if (alreadySuccessfullyCalledGetter)
		{
			propertyAttributes = static_cast<v8::PropertyAttribute>(static_cast<int>(propertyAttributes) | static_cast<int>(v8::PropertyAttribute::DontEnum));
			if (wasAbsent)
			{
				*wasAbsent = false;
			}
		}
	}

	return jscshim::v8PropertyAttributesToJscAttributes(propertyAttributes);
}

bool ObjectWithInterceptors::performDefineOwnProperty(JSC::ExecState				  * exec,
													  const JSC::PropertyName&		  propertyName,
													  const JSC::PropertyDescriptor&  descriptor,
													  bool							  shouldThrow)
{
	if ((nullptr == m_namedInterceptors) || (nullptr == m_namedInterceptors->definer))
	{
		return defineOwnNonIndexProperty(exec, propertyName, descriptor, shouldThrow);
	}

	// Call the "definer" interceptor
	// Note: The const_cast is ugly, but we pass v8Descriptor as a const reference anyway
	v8::PropertyDescriptor v8Descriptor(const_cast<JSC::PropertyDescriptor *>(&descriptor));
	CallbackCall<v8::Value> callbackCall(exec, this, JSC::JSValue(this), m_namedInterceptors->data, shouldThrow);
	m_namedInterceptors->definer(JscPropertyNameToV8Name(exec, propertyName),
								 v8Descriptor,
								 callbackCall.m_callbackInfo);

	// In v8, definers should return a value to indicate they've intercepted the request
	if (callbackCall.m_returnValue)
	{
		return true;
	}

	// The definer didn't intercept the call
	return defineOwnNonIndexProperty(exec, propertyName, descriptor, shouldThrow);
}

bool ObjectWithInterceptors::performDefineOwnProperty(JSC::ExecState				  * exec,
													  unsigned						  index, 
													  const JSC::PropertyDescriptor&  descriptor,
													  bool							  shouldThrow)
{
	if ((nullptr == m_indexedInterceptors) || (nullptr == m_indexedInterceptors->definer))
	{
		return defineOwnIndexedProperty(exec, index, descriptor, shouldThrow);
	}

	// Call the "definer" interceptor
	// Note: The const_cast is ugly, but we pass v8Descriptor as a const reference anyway
	CallbackCall<v8::Value> callbackCall(exec, this, JSC::JSValue(this), m_indexedInterceptors->data, shouldThrow);
	v8::PropertyDescriptor v8Descriptor(const_cast<JSC::PropertyDescriptor *>(&descriptor));
	m_indexedInterceptors->definer(index,
								   v8Descriptor,
								   callbackCall.m_callbackInfo);

	// In v8, definers should return a value to indicate they've intercepted the request
	if (callbackCall.m_returnValue)
	{
		return true;
	}

	// The definer didn't intercept the call
	return defineOwnIndexedProperty(exec, index, descriptor, shouldThrow);
}

}} // v8::jscshim