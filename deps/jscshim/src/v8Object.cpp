/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/Object.h"
#include "shim/ObjectWithInterceptors.h"
#include "shim/Function.h"
#include "shim/APIAccessor.h"
#include "shim/helpers.h"

#include <JavaScriptCore/ObjectConstructor.h>
#include <JavaScriptCore/GetterSetter.h>
#include <JavaScriptCore/StructureChain.h>
#include <JavaScriptCore/JSPromise.h>
#include <JavaScriptCore/JSWeakMap.h>
#include <JavaScriptCore/JSCInlines.h>

/* For it's PromiseWrap\promise hooks, node uses an internal field on promises, configured at build time 
 * (through v8's v8_promise_internal_field_count gyp variable). In order to support it in a generic way,
 * we could use WeakMap, which will map promises to a JSC cell version of EmbeddedFieldsContainer, enter 
 * return it in GetObjectInternalFields. Since node currently uses only one internal field, allocating a new
 * JSC::JSCell for every promise just for that seems like a waste. Thus, for now we'll just support one
 * internal field for promises, and store it's value (which is always a value and not a pointer in node)
 * directly in a WeakMap. */
#ifdef JSCSHIM_PROMISE_INTERNAL_FIELD_COUNT
static_assert(JSCSHIM_PROMISE_INTERNAL_FIELD_COUNT == 1, "jscshim currently supports one internal field for promises");
#endif

namespace
{

v8::jscshim::EmbeddedFieldsContainer<false> * GetObjectInternalFields(v8::Object * object)
{
	JSC::ExecState * exec = v8::jscshim::GetCurrentExecState();
	JSC::VM& vm = exec->vm();

	JSC::JSObject * jscObject = v8::jscshim::GetValue(object).getObject();
	const JSC::ClassInfo * classInfo = jscObject->classInfo(vm);
	
	// Unwrap JSProxy objects (relevant to global objects, which get proxied)
	if (classInfo->isSubClassOf(JSC::JSProxy::info())) {
		jscObject = static_cast<JSC::JSProxy *>(jscObject)->target();
		classInfo = jscObject->classInfo(vm);
	}

	/* Only jscshim::Objects, created by our api, global objects and promises may have 
	 * internal fields. Currently, promise internal fields will be handled by our callers */

	if (classInfo->isSubClassOf(v8::jscshim::Object::info()))
	{
		return &static_cast<v8::jscshim::Object *>(jscObject)->internalFields();
	}
	
	/* TODO: Since all of our global objects are jschim::GlobalObject instance, and it's not being subclassed anywhere, 
	 * can we just use jscObject->isGlobalObject? */
	if (classInfo->isSubClassOf(v8::jscshim::GlobalObject::info()))
	{
		return &static_cast<v8::jscshim::GlobalObject *>(jscObject)->globalInternalFields();
	}

	return nullptr;
}

JSC::MethodTable::GetOwnPropertySlotFunctionPtr GetNonInterceptedGetOwnPropertySlotFunction(JSC::VM& vm, JSC::JSObject * object)
{
	if (object->inherits(vm, v8::jscshim::ObjectWithInterceptors::info()))
	{
		return JSC::JSObject::getOwnPropertySlot;
	}
	
	return object->methodTable(vm)->getOwnPropertySlot;
}

/* TODO: Calling getOwnPropertySlot for each object on the prototype chain is inefficient, 
 * since it might call parseIndex on each call. We should follow JSObject::getPropertySlot's implementation, 
 * but it will require changes in JSObject, as getOwnNonIndexPropertySlot is private. */
bool getRealPropertySlot(JSC::VM& vm, JSC::ExecState * exec, JSC::JSObject * object, const JSC::PropertyName& propertyName, JSC::PropertySlot& slot)
{
	bool found = false;

	JSC::JSObject * currentObject = object;
	while (true)
	{
		JSC::MethodTable::GetOwnPropertySlotFunctionPtr getOwnPropertySlotFunction = GetNonInterceptedGetOwnPropertySlotFunction(vm, currentObject);
		if (getOwnPropertySlotFunction(currentObject, exec, propertyName, slot))
		{
			found = true;
			break;
		}

		/* TODO: Is it ok to call getPrototypeDirect and no getPrototype? this is what JSObject::getPropertySlot
		 * does (using the structure directly, but it basically the same), but it labels it as "FIXME" and mentions
		 * https://bugs.webkit.org/show_bug.cgi?id=172572. */
		JSC::JSValue prototype = currentObject->getPrototypeDirect(vm);
		if (!prototype.isObject())
		{
			break;
		}

		currentObject = asObject(prototype);
	}

	return found;
}

inline void SetMemberFunctionNameIfNecessary(JSC::VM& vm, const JSC::JSValue& key, const JSC::JSValue& value)
{
	/* If this value is a Function, and we have a string name - update it's name property.
	 * TODO: Handle FunctionTemplate instances?
	 * TODO: Handle symbol names? */
	if (key.isString())
	{
		v8::jscshim::Function * asFunction = JSC::jsDynamicCast<v8::jscshim::Function *>(vm, value);
		if (asFunction)
		{
			asFunction->setName(vm, JSC::jsCast<JSC::JSString *>(key.asCell()));
		}
	}
}

v8::Maybe<bool> SetObjectAccessor(v8::Local<v8::Context>		 context,
								  v8::Object					 * object,
								  JSC::JSValue					 name,
								  v8::AccessorNameGetterCallback getter,
								  v8::AccessorNameSetterCallback setter,
								  JSC::JSValue					 data,
								  v8::AccessControl				 settings,
								  v8::PropertyAttribute			 attribute,
								  bool							 isSpecialDataProperty)
{
	SETUP_JSC_USE_IN_FUNCTION(context);
	JSC::JSObject * jscObj = v8::jscshim::GetValue(object).getObject();

	ASSERT(name.isString() || name.isSymbol());
	JSC::PropertyName accessorName = v8::jscshim::JscValueToPropertyName(exec, name);
	SHIM_RETURN_IF_EXCEPTION(v8::Nothing<bool>());

	JSC::PropertySlot slot(jscObj, JSC::PropertySlot::InternalMethodType::GetOwnProperty);
	if (jscObj->methodTable(vm)->getOwnPropertySlot(jscObj, exec, accessorName, slot))
	{
		if (slot.attributes() & JSC::PropertyAttribute::DontDelete)
		{
			return v8::Just(false);
		}
	}

	v8::jscshim::APIAccessor * accessor = v8::jscshim::APIAccessor::create(vm,
																		   global->shimAPIAccessorStructure(),
																		   name.asCell(), 
																		   getter, 
																		   setter, 
																		   data, 
																		   nullptr,
																		   isSpecialDataProperty);
	jscObj->putDirectCustomAPIValue(vm,
								    accessorName,
								    accessor,
								    v8::jscshim::v8PropertyAttributesToJscAttributes(attribute));
	SHIM_RETURN_IF_EXCEPTION(v8::Nothing<bool>());

	return v8::Just(true);
}

/* Assumes objects have ALL_ARRAY_STORAGE_INDEXING_TYPES indexing types
 * TODO: Should be just expose this functionality from JSC::SparseArrayValueMap? */
void CloneArrayStorageSparseMap(JSC::VM&		  vm, 
								JSC::ArrayStorage * sourceArrayStorage, 
								JSC::ArrayStorage * targetArrayStorage, 
								JSC::JSObject	  * targetObject)
{
	JSC::SparseArrayValueMap * sourceSparseMap = sourceArrayStorage->m_sparseMap.get();
	ASSERT(sourceSparseMap);

	JSC::SparseArrayValueMap * targetSparseMap = JSC::SparseArrayValueMap::create(vm);
	
	// Copy the sparse map's flags
	if (sourceSparseMap->sparseMode()) 
	{ 
		targetSparseMap->setSparseMode();
	}
	if (sourceSparseMap->lengthIsReadOnly())
	{
		targetSparseMap->setLengthIsReadOnly();
	}

	// Copy the sparse map's entries
	auto end = sourceSparseMap->end();
	unsigned int i = 0;
	for (auto it = sourceSparseMap->begin(); it != end; ++it, ++i)
	{
		const JSC::SparseArrayEntry& sourceEntry = it.get()->value;
		JSC::SparseArrayEntry& newTargetEntry = targetSparseMap->add(targetObject, i).iterator->value;

		newTargetEntry.set(vm, targetSparseMap, sourceEntry.getNonSparseMode());
		newTargetEntry.attributes = sourceEntry.attributes;
	}

	targetArrayStorage->m_sparseMap.set(vm, targetObject, targetSparseMap);
}

} // (anonymous) namespace

/* The following functions sits in the JSC namespace because of the ALL_* macros used in switch-cases
 * and use consts from JSC without the namespace prefix. */
namespace JSC { namespace jscshim 
{
/* Note that we won't handle sparse map here, since we don't have an owning object yet.
 * Butterfly allocation is based on JSC's Butterfly::createOrGrowPropertyStorage. */
JSC::Butterfly * CreateButterflyForClonedObjectWithoutArraySparseMap(JSC::VM& vm, JSC::JSObject * sourceObject, JSC::Structure * structure)
{
	JSC::Butterfly * originalButterfly = sourceObject->butterfly();
	if (nullptr == originalButterfly)
	{
		return nullptr;
	}

	// Calculate the butterfly's size
	size_t preCapacity = originalButterfly->indexingHeader()->preCapacity(structure);
	size_t indexingPayloadSizeInBytes = originalButterfly->indexingHeader()->indexingPayloadSizeInBytes(structure);
	bool hasIndexingHeader = structure->hasIndexingHeader(sourceObject);
	unsigned int propertyCapacity = structure->outOfLineCapacity();

	size_t butterflyTotalSize = JSC::Butterfly::totalSize(preCapacity, propertyCapacity, hasIndexingHeader, indexingPayloadSizeInBytes);

	// Allocate the new butterfly
	void * base = vm.jsValueGigacageAuxiliarySpace.allocateNonVirtual(vm, butterflyTotalSize, nullptr, JSC::AllocationFailureMode::Assert);
	JSC::Butterfly * newObjectButterfly = JSC::Butterfly::fromBase(base, preCapacity, propertyCapacity);

	if (hasIndexingHeader)
	{
		// Copy the object's indexing header
		*newObjectButterfly->indexingHeader() = *originalButterfly->indexingHeader();

		// Initialize the new ArrayStorage if we have one (without handling the sparse map)
		switch (structure->indexingType())
		{
		case ALL_ARRAY_STORAGE_INDEXING_TYPES:
			JSC::ArrayStorage * sourceArrayStorage = originalButterfly->arrayStorage();
			JSC::ArrayStorage * targetArrayStorage = newObjectButterfly->arrayStorage();

			sourceArrayStorage->m_numValuesInVector = targetArrayStorage->m_numValuesInVector;
			sourceArrayStorage->m_indexBias = targetArrayStorage->m_indexBias;
			break;
		}
	}

	return newObjectButterfly;
}

/* This is based on JSC's JSObject::heapSnapshot.
 * TODO: Is this the most efficient way to do this? */
inline void CopyObjectPropertyValues(VM&	   vm, 
									 Structure * structure, 
									 JSObject  * sourceObject, 
									 JSObject  * targetObject)
{

	for (auto& entry : structure->getPropertiesConcurrently()) {
		targetObject->putDirect(vm, entry.offset, sourceObject->getDirect(entry.offset));
	}

	JSC::Butterfly * sourceButterfly = sourceObject->butterfly();
	if (!sourceButterfly)
	{
		return;
	}

	JSC::Butterfly * targetButterfly = sourceObject->butterfly();
	ASSERT(targetButterfly);

	JSC::WriteBarrier<JSC::Unknown>* sourceData = nullptr;
	JSC::WriteBarrier<JSC::Unknown>* targetData = nullptr;
	uint32_t count = 0;

	switch (sourceObject->indexingType()) {
	case ALL_CONTIGUOUS_INDEXING_TYPES:
		sourceData = sourceButterfly->contiguous().data();
		targetData = targetButterfly->contiguous().data();
		count = sourceButterfly->publicLength();
		for (uint32_t i = 0; i < count; ++i)
		{
			targetData[i].set(vm, targetObject, sourceData[i].get());
		}

		break;
	case ALL_ARRAY_STORAGE_INDEXING_TYPES:
		if (sourceButterfly->arrayStorage()->m_sparseMap)
		{
			JSC::ArrayStorage * sourceArrayStorage = sourceButterfly->arrayStorage();
			JSC::ArrayStorage * targetArrayStorage = targetButterfly->arrayStorage();
			CloneArrayStorageSparseMap(vm, sourceArrayStorage, targetArrayStorage, targetObject);
		}
		else
		{
			sourceData = sourceButterfly->arrayStorage()->m_vector;
			targetData = targetButterfly->arrayStorage()->m_vector;
			count = sourceButterfly->arrayStorage()->vectorLength();
			for (uint32_t i = 0; i < count; ++i)
			{
				targetData[i].set(vm, targetObject, sourceData[i].get());
			}
		}

		break;
	default:
		// TODO: Assert "never reached"?
		break;
	}
}

}} // JSC::jscshim

namespace v8
{

bool Object::Set(Local<Value> key, Local<Value> value)
{
	return Set(Isolate::GetCurrent()->GetCurrentContext(), key, value).FromMaybe(false);
}

Maybe<bool> Object::Set(Local<Context> context, Local<Value> key, Local<Value> value)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);
	
	JSC::PropertyName jscKey = jscshim::JscValueToPropertyName(exec, key.val_);
	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());

	SetMemberFunctionNameIfNecessary(vm, key.val_, value.val_);

	// TODO: Is InternalMethodType::Get OK?
	JSC::PropertySlot slot(thisObj, JSC::PropertySlot::InternalMethodType::Get);
	
	if (false == thisObj->getPropertySlot(exec, jscKey, slot))
	{
		SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());

		JSC::PropertyDescriptor descriptor(value.val_, 0);
		thisObj->methodTable(vm)->defineOwnProperty(thisObj, exec, jscKey, descriptor, false);
	}
	else if (slot.isAccessor() && slot.getterSetter()->isSetterNull())
	{
		JSC::PropertyDescriptor descriptor(value.val_, slot.attributes() & ~JSC::PropertyAttribute::Accessor);
		thisObj->methodTable(vm)->defineOwnProperty(thisObj, exec, jscKey, descriptor, false);
	}
	else
	{
		JSC::PutPropertySlot putSlot(thisObj);
		thisObj->methodTable()->put(thisObj, exec, jscKey, value.val_, putSlot);
	}

	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());
	return Just(true);
}

bool Object::Set(uint32_t index, Local<Value> value)
{
	return Set(Isolate::GetCurrent()->GetCurrentContext(), index, value).FromMaybe(false);
}

Maybe<bool> Object::Set(Local<Context> context, uint32_t index, Local<Value> value)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	// TODO: Make sure we should throw an exception
	if (false == thisObj->methodTable()->putByIndex(thisObj, exec, index, value.val_, true))
	{
		// It seems that v8 always returns Nothing<bool>() on failure (not Just(false))
		return Nothing<bool>();
	}
	
	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());
	return Just(true);
}

Maybe<bool> Object::CreateDataProperty(Local<Context> context,
									   Local<Name> key,
									   Local<Value> value)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	JSC::PropertyName jscKey = jscshim::JscValueToPropertyName(exec, key.val_);
	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());

	SetMemberFunctionNameIfNecessary(vm, key.val_, value.val_);

	// Note: v8's CreateDataProperty doesn't throw here, so we won't too
	JSC::PropertyDescriptor descriptor(value.val_, static_cast<unsigned int>(JSC::PropertyAttribute::None));
	bool result = thisObj->methodTable(vm)->defineOwnProperty(thisObj, exec, jscKey, descriptor, false);
	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());

	return Just(result);

}

Maybe<bool> Object::CreateDataProperty(Local<Context> context,
									   uint32_t index,
									   Local<Value> value)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	/* TODO: This is inefficient, since defineOwnProperty will parse the identifier string and pass it to
	 * the internal defineOwnIndexedProperty. We should find a better solution. */
	JSC::PropertyName jscKey(JSC::Identifier::from(jscshim::GetExecStateForV8Context(*context), index));

	// Note: v8's CreateDataProperty doesn't throw here, so we won't to
	JSC::PropertyDescriptor descriptor(value.val_, static_cast<unsigned int>(JSC::PropertyAttribute::None));
	thisObj->methodTable(vm)->defineOwnProperty(thisObj, exec, jscKey, descriptor, false);
	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());

	return Just(true);
}

Local<Value> Object::Get(Local<Value> key)
{
	return Get(Isolate::GetCurrent()->GetCurrentContext(), key).FromMaybe(Local<Value>());
}

MaybeLocal<Value> Object::Get(Local<Context> context, Local<Value> key)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	JSC::PropertyName jscKey = jscshim::JscValueToPropertyName(exec, key.val_);
	SHIM_RETURN_IF_EXCEPTION(Local<Value>());

	/* Note: Calling "thisObj->get(exec, jscKey->stack)" fails when we're being called
	 * after an exception was thrown, since internally the macro RETURN_IF_EXCEPTION will always
	 * return an error (since we have a pending exception) */
	JSC::PropertySlot slot(thisObj, JSC::PropertySlot::InternalMethodType::Get);
	bool hasProperty = thisObj->getPropertySlot(exec, jscKey, slot);
	SHIM_RETURN_IF_EXCEPTION(Local<Value>());

	JSC::JSValue jscValue = hasProperty ? slot.getValue(exec, jscKey) : JSC::jsUndefined();
	SHIM_RETURN_IF_EXCEPTION(Local<Value>());

	return Local<Value>::New(jscValue);
}

Local<Value> Object::Get(uint32_t index)
{
	return Get(Isolate::GetCurrent()->GetCurrentContext(), index).FromMaybe(Local<Value>());
}

MaybeLocal<Value> Object::Get(Local<Context> context, uint32_t index)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	// TODO: JSObjectGetPropertyAtIndex uses "get", make sure getIndex is better here
	JSC::JSValue jscValue = thisObj->getIndex(exec, index);
	SHIM_RETURN_IF_EXCEPTION(Local<Value>());

	return Local<Value>::New(jscValue);
}

Maybe<bool> Object::Delete(Local<Context> context, Local<Value> key)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	JSC::PropertyName jscKey = jscshim::JscValueToPropertyName(exec, key.val_);
	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());

	bool result = thisObj->methodTable()->deleteProperty(thisObj, exec, jscKey);
	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());

	return Just(result);
}

Maybe<bool> Object::Delete(Local<Context> context, uint32_t index)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	bool result = thisObj->methodTable()->deletePropertyByIndex(thisObj, exec, index);

	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());
	return Just(result);
}

Local<Object> Object::New(Isolate* isolate)
{
	JSC::JSObject * emptyObject = JSC::constructEmptyObject(jscshim::GetExecStateForV8Isolate(isolate));
	return Local<Object>::New(JSC::JSValue(emptyObject));
}

MaybeLocal<String> Object::ObjectProtoToString(Local<Context> context)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	JSC::JSValue toStringFunction = global->objectProtoToString();

	JSC::CallData callData;
	JSC::CallType callType = JSC::getCallData(toStringFunction, callData);
	if (JSC::CallType::None == callType)
	{
		shimExceptionScope.ThrowExcpetion(JSC::createNotAFunctionError(exec, toStringFunction));
		return Local<String>();
	}

	JSC::MarkedArgumentBuffer argList;
	JSC::JSValue result = profiledCall(exec, JSC::ProfilingReason::API, toStringFunction, callType, callData, thisObj, argList);
	SHIM_RETURN_IF_EXCEPTION(Local<String>());

	return Local<String>::New(result);
}

Local<String> Object::GetConstructorName()
{
	SETUP_OBJECT_USE_IN_MEMBER(Isolate::GetCurrent()->GetCurrentContext());

	JSC::JSValue className(JSC::jsString(exec, JSC::JSObject::calculatedClassName(thisObj)));
	SHIM_RETURN_IF_EXCEPTION(Local<String>());

	return Local<String>(className);
}

Maybe<bool> Object::SetIntegrityLevel(Local<Context> context, IntegrityLevel level)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	if (IntegrityLevel::kFrozen == level)
	{
		JSC::objectConstructorFreeze(exec, thisObj);
	}
	else
	{
		JSC::objectConstructorSeal(exec, thisObj);
	}

	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());
	return Just(true);
}

int Object::InternalFieldCount()
{
	auto * internalFields = GetObjectInternalFields(this);
	if (nullptr == internalFields)
	{
#ifdef JSCSHIM_PROMISE_INTERNAL_FIELD_COUNT
		// Handle promises
		JSC::JSObject * jscObject = v8::jscshim::GetValue(this).getObject();
		if (jscObject->classInfo(*jscObject->vm())->isSubClassOf(JSC::JSPromise::info()))
		{
			return JSCSHIM_PROMISE_INTERNAL_FIELD_COUNT;
		}
#endif

		return 0;
	}

	return internalFields->Size();
}

Maybe<bool> Object::SetPrototype(Local<Context> context, Local<Value> prototype)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	/* Note: Regarding whether we should throw an exception, from v8's Object::SetPrototype: 
	 * "We do not allow exceptions thrown while setting the prototype to propagate outside." */
	if (!thisObj->setPrototype(vm, exec, prototype.val_, false))
	{
		return Nothing<bool>();
	}

	return Just(true);
}

// TODO: ApiChecks when out of bounds (currently checked in internalFields->GetValue)
Local<Value> Object::GetInternalField(int index)
{
	auto * internalFields = GetObjectInternalFields(this);
	if (nullptr == internalFields)
	{
#ifdef JSCSHIM_PROMISE_INTERNAL_FIELD_COUNT
		// Handle promises
		JSC::JSObject * jscObject = v8::jscshim::GetValue(this).getObject();
		JSC::JSPromise * thisAsPromise = JSC::jsDynamicCast<JSC::JSPromise *>(*jscObject->vm(), jscObject);
		
		// Note that we currently support only one internal field per promise
		if (thisAsPromise && (0 == index))
		{
			jscshim::GlobalObject * global = static_cast<jscshim::GlobalObject *>(thisAsPromise->globalObject());
			
			/* This will return undefined if the key doesn't exist, which is ok for us since it's
			 * the default value for internal fields in v8 */
			Local<Value>(global->promisesInternalFields()->get(thisAsPromise));
		}
#endif

		return Local<Value>();
	}

	return Local<Value>(internalFields->GetValue(index));
}

// TODO: ApiChecks when out of bounds (currently checked in internalFields->SetValue)
void Object::SetInternalField(int index, Local<Value> value)
{
	auto * internalFields = GetObjectInternalFields(this);
	if (internalFields)
	{
		internalFields->SetValue(index, value.val_);
	}
#ifdef JSCSHIM_PROMISE_INTERNAL_FIELD_COUNT
	else
	{
		// Handle promises
		JSC::JSObject * jscObject = v8::jscshim::GetValue(this).getObject();
		JSC::JSPromise * thisAsPromise = JSC::jsDynamicCast<JSC::JSPromise *>(*jscObject->vm(), jscObject);

		// Note that we currently support only one internal field per promise
		if (thisAsPromise && (0 == index))
		{
			jscshim::GlobalObject * global = static_cast<jscshim::GlobalObject *>(thisAsPromise->globalObject());
			global->promisesInternalFields()->set(*jscObject->vm(), thisAsPromise, value.val_);
		}
	}
#endif
}

// TODO: ApiChecks when out of bounds (currently checked in internalFields->GetAlignedPointer)
void * Object::GetAlignedPointerFromInternalField(int index)
{
	auto * internalFields = GetObjectInternalFields(this);
	if (nullptr == internalFields)
	{
		return nullptr;
	}

	return internalFields->GetAlignedPointer(index);
}

// TODO: ApiChecks when out of bounds (currently checked in internalFields->SetAlignedPointer)
void Object::SetAlignedPointerInInternalField(int index, void * value)
{
	auto * internalFields = GetObjectInternalFields(this);
	if (internalFields)
	{
		internalFields->SetAlignedPointer(index, value);
	}
}

void Object::SetAlignedPointerInInternalFields(int argc, int indices[], void* values[])
{
	auto * internalFields = GetObjectInternalFields(this);
	if (internalFields)
	{
		for (int i = 0; i < argc; i++)
		{
			internalFields->SetAlignedPointer(indices[i], values[i]);
		}
	}
}

Maybe<bool> Object::DefineOwnProperty(Local<Context> context, Local<Name> key, Local<Value> value, PropertyAttribute attributes)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	JSC::PropertyName jscKey = jscshim::JscValueToPropertyName(exec, key.val_);
	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());

	// TODO: Is this needed here?
	SetMemberFunctionNameIfNecessary(vm, key.val_, value.val_);

	JSC::PropertyDescriptor desc(value.val_, jscshim::v8PropertyAttributesToJscAttributes(attributes));
	bool result = thisObj->methodTable(vm)->defineOwnProperty(thisObj, exec, jscKey, desc, false);
	
	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());
	return Just(result);
}

Local<Value> Object::GetPrototype()
{
	SETUP_OBJECT_USE_IN_MEMBER(Isolate::GetCurrent()->GetCurrentContext());

	JSC::JSValue prototype = thisObj->getPrototype(exec->vm(), exec);

	SHIM_RETURN_IF_EXCEPTION(Local<Value>());
	return Local<Value>::New(prototype);
}

Maybe<bool> Object::HasOwnProperty(Local<Context> context, Local<Name> key)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	JSC::PropertyName jscKey = jscshim::JscValueToPropertyName(exec, key.val_);
	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());

	bool result = thisObj->hasOwnProperty(exec, jscKey);
	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());

	return Just(result);
}

Maybe<bool> Object::HasOwnProperty(Local<Context> context, uint32_t index)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	bool result = thisObj->hasOwnProperty(exec, index);
	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());

	return Just(result);
}

MaybeLocal<Value> Object::GetOwnPropertyDescriptor(Local<Context> context, Local<Name> key)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	JSC::Identifier jscKey = key.val_.toPropertyKey(exec);
	JSC::JSValue descriptor = JSC::objectConstructorGetOwnPropertyDescriptor(exec, thisObj, jscKey);

	return Local<Value>(descriptor);
}

Maybe<PropertyAttribute> Object::GetPropertyAttributes(Local<Context> context, Local<Value> key)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	JSC::PropertyName jscKey = jscshim::JscValueToPropertyName(exec, key.val_);
	SHIM_RETURN_IF_EXCEPTION(Nothing<PropertyAttribute>());

	/* Note: Calling "thisObj->get(exec, jscKey->stack)" fails when we're being called
	 * after an exception was thrown, since internally the macro RETURN_IF_EXCEPTION will always
	 * return an error (since we have a pending exception) */
	JSC::PropertySlot slot(thisObj, JSC::PropertySlot::InternalMethodType::Get);
	bool hasProperty = thisObj->getPropertySlot(exec, jscKey, slot);
	SHIM_RETURN_IF_EXCEPTION(Nothing<PropertyAttribute>());

	PropertyAttribute attributes = hasProperty ? jscshim::JscPropertyAttributesToV8Attributes(slot.attributes()) : PropertyAttribute::None;
	SHIM_RETURN_IF_EXCEPTION(Nothing<PropertyAttribute>());

	return Just(attributes);
}

Maybe<bool> Object::HasRealNamedProperty(Local<Context> context, Local<Name> key)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	JSC::PropertyName jscKey = jscshim::JscValueToPropertyName(exec, key.val_);
	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());

	JSC::MethodTable::GetOwnPropertySlotFunctionPtr getOwnPropertySlotFunction = GetNonInterceptedGetOwnPropertySlotFunction(vm, thisObj);
	JSC::PropertySlot slot(thisObj, JSC::PropertySlot::InternalMethodType::Get);
	bool hasProperty = getOwnPropertySlotFunction(thisObj, exec, jscKey, slot);
	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());
	
	return Just<bool>(hasProperty);
}

Maybe<bool> Object::HasRealIndexedProperty(Local<Context> context, uint32_t index)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	// Get the "non intercepted" GetOwnPropertySlotByIndexFunction
	JSC::MethodTable::GetOwnPropertySlotByIndexFunctionPtr getOwnPropertySlotByIndexFunction = nullptr;
	if (thisObj->inherits(vm, v8::jscshim::ObjectWithInterceptors::info()))
	{
		getOwnPropertySlotByIndexFunction = JSC::JSObject::getOwnPropertySlotByIndex;
	}
	else
	{
		getOwnPropertySlotByIndexFunction = thisObj->methodTable(vm)->getOwnPropertySlotByIndex;
	}

	JSC::PropertySlot slot(thisObj, JSC::PropertySlot::InternalMethodType::Get);
	bool hasProperty = getOwnPropertySlotByIndexFunction(thisObj, exec, index, slot);
	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());

	return Just<bool>(hasProperty);
}

MaybeLocal<Value> Object::GetRealNamedProperty(Local<Context> context, Local<Name> key)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	// TODO: Is VMInquiry right?
	JSC::PropertySlot slot(thisObj, JSC::PropertySlot::InternalMethodType::VMInquiry);
	JSC::PropertyName propertyName(key.val_.toPropertyKey(exec));
	if (!getRealPropertySlot(vm, exec, thisObj, propertyName, slot))
	{
		return Local<Value>();
	}

	JSC::JSValue propertyValue = slot.getValue(exec, propertyName);
	SHIM_RETURN_IF_EXCEPTION(Local<Value>());
	
	return Local<Value>(propertyValue);
}

Maybe<bool> Object::HasRealNamedCallbackProperty(Local<Context> context, Local<Name> key)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	// TODO: Is VMInquiry right?
	JSC::PropertySlot slot(thisObj, JSC::PropertySlot::InternalMethodType::VMInquiry);
	JSC::PropertyName propertyName(key.val_.toPropertyKey(exec));
	if (!getRealPropertySlot(vm, exec, thisObj, propertyName, slot))
	{
		return Nothing<bool>();
	}

	return Just(slot.isCustomAPIValue());
}

MaybeLocal<Value> Object::GetRealNamedPropertyInPrototypeChain(Local<Context> context, Local<Name> key)
{
	// TODO
	return Local<Value>();
}

Maybe<PropertyAttribute> Object::GetRealNamedPropertyAttributes(Local<Context> context, Local<Name> key)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	// TODO: Is VMInquiry right?
	JSC::PropertySlot slot(thisObj, JSC::PropertySlot::InternalMethodType::VMInquiry);
	if (!getRealPropertySlot(vm, exec, thisObj, key.val_.toPropertyKey(exec), slot))
	{
		Just(PropertyAttribute::None);
	}

	return Just<PropertyAttribute>(jscshim::JscPropertyAttributesToV8Attributes(slot.attributes()));
}

Maybe<bool> Object::Has(Local<Context> context, Local<Value> key)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	JSC::PropertyName jscKey = jscshim::JscValueToPropertyName(exec, key.val_);
	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());

	bool result = thisObj->hasProperty(exec, jscKey);
	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());

	return Just(result);
}

Maybe<bool> Object::Has(Local<Context> context, uint32_t index)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	bool result = thisObj->hasProperty(exec, index);
	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());

	return Just(result);
}

Maybe<bool> Object::SetAccessor(Local<Context> context,
								Local<Name> name,
								AccessorNameGetterCallback getter,
								AccessorNameSetterCallback setter,
								MaybeLocal<Value> data,
								AccessControl settings,
								PropertyAttribute attribute)
{
	return SetObjectAccessor(context, this, name.val_, getter, setter, data.val_, settings, attribute, false);
}

Maybe<bool> Object::SetNativeDataProperty(Local<Context> context,
										  Local<Name> name,
										  AccessorNameGetterCallback getter,
										  AccessorNameSetterCallback setter,
										  Local<Value> data,
										  PropertyAttribute attributes)
{
	return SetObjectAccessor(context, this, name.val_, getter, setter, data.val_, DEFAULT, attributes, true);
}

Maybe<bool> Object::DefineProperty(Local<Context> context, Local<Name> key, PropertyDescriptor& descriptor)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	JSC::PropertyName jscKey = jscshim::JscValueToPropertyName(exec, key.val_);
	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());

	// TODO: Need to call SetMemberFunctionNameIfNecessary for data descriptors?

	return Just(thisObj->methodTable(vm)->defineOwnProperty(thisObj, exec, jscKey, *descriptor.get_private(), false));
}

Maybe<bool> Object::ForceSet(Local<Context> context,
							 Local<Value> key,
							 Local<Value> value,
							 PropertyAttribute attribs)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	JSC::PropertyName jscKey = jscshim::JscValueToPropertyName(exec, key.val_);
	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());

	SetMemberFunctionNameIfNecessary(vm, key.val_, value.val_);

	bool result = thisObj->putDirect(vm, jscKey, value.val_);
	SHIM_RETURN_IF_EXCEPTION(Nothing<bool>());

	return Just(result);
}

Maybe<bool> Object::HasPrivate(Local<Context> context, Local<Private> key)
{
	return HasOwnProperty(context, Local<Name>(key.val_));
}

Maybe<bool> Object::DeletePrivate(Local<Context> context, Local<Private> key)
{
	return Delete(context, Local<Value>(key.val_));
}

Maybe<bool> Object::SetPrivate(Local<Context> context, Local<Private> key, Local<Value> value)
{
	return Set(context, Local<Value>(key.val_), value);
}

MaybeLocal<Value> Object::GetPrivate(Local<Context> context, Local<Private> key)
{
	return Get(context, Local<Value>(key.val_));
}

Local<Array> Object::GetPropertyNames()
{
	return GetPropertyNames(Isolate::GetCurrent()->GetCurrentContext()).FromMaybe(Local<Array>());
}

MaybeLocal<Array> Object::GetPropertyNames(Local<Context> context)
{
	// Taken from v8
	return GetPropertyNames(context, 
							KeyCollectionMode::kIncludePrototypes,
							static_cast<PropertyFilter>(ONLY_ENUMERABLE | SKIP_SYMBOLS),
							IndexFilter::kIncludeIndices);
}

inline bool PropertyFilterHasFlag(PropertyFilter filter, PropertyFilter flag)
{
	return static_cast<unsigned int>(filter) & static_cast<unsigned int>(flag);
}

/* This is based on JSC::ownPropertyKeys, but calls the object's getPropertyNames
 * instead of getOwnPropertyNames if KeyCollectionMode is kIncludePrototypes.
 *
 * TODO: It seems that v8 prior to node's https://github.com/nodejs/node/commit/12a1b9b8049462e47181a298120243dc83e81c55#diff-42f385dff7890b3c213d9699bcc350cc
 * commit will return "indexed" properties as int32 values, while we'll return them 
 * as numeric strings. The v8\node commit adds a KeyConversionMode parameter letting the
 * caller control this conversion (default is kKeepNumbers). This might be hard to solve, 
 * since JSC::Identifier always converts numbers to strings (and in JSC, PropertyNameArray 
 * arrays contain JSC::Identifier instances).
 *
 * TODO: Support ONLY_WRITABLE, and ONLY_CONFIGURABLE filters 
 * TODO: Handle IndexFilter
 */
MaybeLocal<Array> Object::GetPropertyNames(Local<Context>    context,
										   KeyCollectionMode mode,
										   PropertyFilter    property_filter,
										   IndexFilter       index_filter)
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	JSC::DontEnumPropertiesMode dontEnumPropertiesMode = PropertyFilterHasFlag(property_filter, ONLY_ENUMERABLE) ? JSC::DontEnumPropertiesMode::Exclude : 
																												   JSC::DontEnumPropertiesMode::Include;

	ASSERT(!PropertyFilterHasFlag(property_filter, SKIP_STRINGS) || !PropertyFilterHasFlag(property_filter, SKIP_SYMBOLS));
	JSC::PropertyNameMode propertyNameMode = propertyNameMode = JSC::PropertyNameMode::StringsAndSymbols;
	if (PropertyFilterHasFlag(property_filter, SKIP_STRINGS))
	{
		propertyNameMode = JSC::PropertyNameMode::Symbols;
	}
	else if (PropertyFilterHasFlag(property_filter, SKIP_SYMBOLS))
	{
		propertyNameMode = JSC::PropertyNameMode::Strings;
	}

	// Fist, get the property names from JSC
	JSC::PropertyNameArray properties(&vm, propertyNameMode, JSC::PrivateSymbolMode::Exclude);
	if (KeyCollectionMode::kOwnOnly == mode)
	{
		thisObj->methodTable(vm)->getOwnPropertyNames(thisObj, exec, properties, JSC::EnumerationMode(dontEnumPropertiesMode));
	}
	else
	{
		thisObj->methodTable(vm)->getPropertyNames(thisObj, exec, properties, JSC::EnumerationMode(dontEnumPropertiesMode));
	}
	SHIM_RETURN_IF_EXCEPTION(Local<Array>());

	/* From here on the code is basically copied from JSC::ownPropertyKeys, with minor modifications 
	 * (replacing RETURN_IF_EXCEPTION with SHIM_RETURN_IF_EXCEPTION, fix the return value, etc.). */

	// https://tc39.github.io/ecma262/#sec-enumerableownproperties
	// If {object} is a Proxy, an explicit and observable [[GetOwnProperty]] op is required to filter out non-enumerable properties.
	// In other cases, filtering has already been performed.
	const bool mustFilterProperty = dontEnumPropertiesMode == JSC::DontEnumPropertiesMode::Exclude && thisObj->type() == JSC::ProxyObjectType;
	auto filterPropertyIfNeeded = [exec, thisObj, mustFilterProperty](const JSC::Identifier& identifier) {
		if (!mustFilterProperty)
			return true;
		JSC::PropertyDescriptor descriptor;
		JSC::PropertyName name(identifier);
		return thisObj->getOwnPropertyDescriptor(exec, name, descriptor) && descriptor.enumerable();
	};

	// If !mustFilterProperty and PropertyNameMode::Strings mode, we do not need to filter out any entries in PropertyNameArray.
	// We can use fast allocation and initialization.
	if (propertyNameMode != JSC::PropertyNameMode::StringsAndSymbols) {
		ASSERT(propertyNameMode == JSC::PropertyNameMode::Strings || propertyNameMode == JSC::PropertyNameMode::Symbols);
		if (!mustFilterProperty && properties.size() < MIN_SPARSE_ARRAY_INDEX) {
			auto* globalObject = exec->lexicalGlobalObject();
			if (LIKELY(!globalObject->isHavingABadTime())) {
				size_t numProperties = properties.size();
				JSC::JSArray* keys = JSC::JSArray::create(vm, globalObject->originalArrayStructureForIndexingType(JSC::ArrayWithContiguous), numProperties);
				JSC::WriteBarrier<JSC::Unknown>* buffer = keys->butterfly()->contiguous().data();
				for (size_t i = 0; i < numProperties; i++) {
					const auto& identifier = properties[i];
					if (propertyNameMode == JSC::PropertyNameMode::Strings) {
						ASSERT(!identifier.isSymbol());
						buffer[i].set(vm, keys, jsOwnedString(&vm, identifier.string()));
					}
					else {
						ASSERT(identifier.isSymbol());
						buffer[i].set(vm, keys, JSC::Symbol::create(vm, static_cast<SymbolImpl&>(*identifier.impl())));
					}
				}
				return Local<Array>(JSC::JSValue(keys));
			}
		}
	}

	JSC::JSArray* keys = constructEmptyArray(exec, nullptr);
	SHIM_RETURN_IF_EXCEPTION(Local<Array>());

	unsigned index = 0;
	auto pushDirect = [&](JSC::ExecState* exec, JSC::JSArray* array, JSC::JSValue value) {
		array->putDirectIndex(exec, index++, value);
	};

	switch (propertyNameMode) {
	case JSC::PropertyNameMode::Strings: {
		size_t numProperties = properties.size();
		for (size_t i = 0; i < numProperties; i++) {
			const auto& identifier = properties[i];
			ASSERT(!identifier.isSymbol());
			bool hasProperty = filterPropertyIfNeeded(identifier);
			EXCEPTION_ASSERT(!shimExceptionScope.HasException() || !hasProperty);
			if (hasProperty)
				pushDirect(exec, keys, jsOwnedString(exec, identifier.string()));
			SHIM_RETURN_IF_EXCEPTION(Local<Array>());
		}
		break;
	}

	case JSC::PropertyNameMode::Symbols: {
		size_t numProperties = properties.size();
		for (size_t i = 0; i < numProperties; i++) {
			const auto& identifier = properties[i];
			ASSERT(identifier.isSymbol());
			ASSERT(!identifier.isPrivateName());
			bool hasProperty = filterPropertyIfNeeded(identifier);
			EXCEPTION_ASSERT(!shimExceptionScope.HasException() || !hasProperty);
			if (hasProperty)
				pushDirect(exec, keys, JSC::Symbol::create(vm, static_cast<SymbolImpl&>(*identifier.impl())));
			SHIM_RETURN_IF_EXCEPTION(Local<Array>());
		}
		break;
	}

	case JSC::PropertyNameMode::StringsAndSymbols: {
		Vector<JSC::Identifier, 16> propertySymbols;
		size_t numProperties = properties.size();
		for (size_t i = 0; i < numProperties; i++) {
			const auto& identifier = properties[i];
			if (identifier.isSymbol()) {
				ASSERT(!identifier.isPrivateName());
				propertySymbols.append(identifier);
				continue;
			}

			bool hasProperty = filterPropertyIfNeeded(identifier);
			EXCEPTION_ASSERT(!shimExceptionScope.HasException() || !hasProperty);
			if (hasProperty)
				pushDirect(exec, keys, jsOwnedString(exec, identifier.string()));
			SHIM_RETURN_IF_EXCEPTION(Local<Array>());
		}

		// To ensure the order defined in the spec (9.1.12), we append symbols at the last elements of keys.
		for (const auto& identifier : propertySymbols) {
			bool hasProperty = filterPropertyIfNeeded(identifier);
			EXCEPTION_ASSERT(!shimExceptionScope.HasException() || !hasProperty);
			if (hasProperty)
				pushDirect(exec, keys, JSC::Symbol::create(vm, static_cast<SymbolImpl&>(*identifier.impl())));
			SHIM_RETURN_IF_EXCEPTION(Local<Array>());
		}

		break;
	}
	}

	return Local<Array>(JSC::JSValue(keys));
}

Local<Array> Object::GetOwnPropertyNames()
{
	return GetOwnPropertyNames(Isolate::GetCurrent()->GetCurrentContext()).FromMaybe(Local<Array>());
}

MaybeLocal<Array> Object::GetOwnPropertyNames(Local<Context> context)
{
	// Taken from v8
	return GetOwnPropertyNames(context, static_cast<PropertyFilter>(ONLY_ENUMERABLE | SKIP_SYMBOLS));
}

MaybeLocal<Array> Object::GetOwnPropertyNames(Local<Context> context, PropertyFilter filter)
{
	// Taken from v8
	return GetPropertyNames(context, KeyCollectionMode::kOwnOnly, filter, IndexFilter::kIncludeIndices);
}

/* We can mimic JSC's Object.assign implementation, but it seems inefficient, since we know 
 * we're only cloning one object. It also goes through the object's method table functions
 * for accessing the properties, which means it will go through interceptors. Looking 
 * at v8's implementation (Heap::CopyJSObject in heap\heap.cc), it seems that v8's 
 * implementation is based on copying the object's underlying memory.
 * 
 * JSC doesn't seem to provide a direct cloning functionality, so we'll implement it ourselves.
 * I hope I'm not missing anything, but roughly speaking, an object in JSC consists information 
 * inherited from JSC::JSCell, "storage areas" (for properties and elements) and a JSC::Structure, 
 * defining the object's property layout (offsets in the relevant "storage areas").
 * It seems that an object can have three "storage areas", the first two being buffers of JSValues (JSC::PropertyStorage):
 * - Inline storage for properties, allocated together with the object iteself.
 * - A JSC::Butterfly\JSC::ArrayStorage, containing additional property ("out of line" properties)\Element areas.
 * - JSC::ArrayStorage can also have a sparse map of array entries which will hold the array entries, 
 *   instead of the JSC::ArrayStorage's vector.
 * 
 * For more information about JSC's object model:
 * - http://phrack.org/papers/attacking_javascript_engines.html has a pretty good explanation,
 *   specifically under "Objects and arrays" and "About structures".
 * - https://webkit.org/blog/7846/concurrent-javascript-it-can-work/, under "JavaScriptCore Object Model".
 * - "Overview of JSArray" in JSC's ArrayConventions.h
 * 
 * So, it seems that in order to clone an object, we need to:
 * 1. If the object has a butterfly, allocate a new butterfly with the same size and properties ("indexing header").
 * 2. Allocate a new object with the same JSC::Structure and inline capacity, and optionally the new butterfly. 
 *    It seems that JSObject::createRawObject does exactly that.
 * 3. Copy the actual property\element values to the new object, maintaining proper "write barriers"
 *    for the new object.
 *
 * TODO:
 * - Locking?
 * - Verify this is actually faster than JSC::objectConstructorAssign's implementation.
 * - Make sure this is OK with JSC's copy-on-write (CoW) array buffers
 */
Local<Object> Object::Clone()
{
	SETUP_OBJECT_USE_IN_MEMBER(Isolate::GetCurrent()->GetCurrentContext());

	JSC::Structure * structure = thisObj->structure(vm);

	JSC::Butterfly * newObjectButterfly = JSC::jscshim::CreateButterflyForClonedObjectWithoutArraySparseMap(vm, thisObj, structure);
	JSC::JSObject * newObject = JSC::JSObject::createRawObject(exec, structure, newObjectButterfly);
	JSC::jscshim::CopyObjectPropertyValues(vm, structure, thisObj, newObject);

	return Local<Object>(JSC::JSValue(newObject));
}

Local<Context> Object::CreationContext()
{
	SETUP_OBJECT_USE_IN_MEMBER(Isolate::GetCurrent()->GetCurrentContext());
	return jscshim::GetV8ContextForObject(thisObj);
}

bool Object::IsCallable()
{
	// The name "isFunction" is a bit misleading, it also checks if the value is callable
	return jscshim::GetValue(this).isFunction(jscshim::GetCurrentVM());
}

bool Object::IsConstructor()
{
	return jscshim::GetValue(this).isConstructor();
}

MaybeLocal<Value> Object::CallAsFunction(Local<Context>	context,
										 Local<Value>	recv,
										 int			argc,
										 Local<Value>	argv[])
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	// Get the object's call data
	JSC::CallData callData;
	JSC::CallType callType = thisObj->methodTable()->getCallData(thisObj, callData);
	if (JSC::CallType::None == callType)
	{
		shimExceptionScope.ThrowExcpetion(JSC::createNotAFunctionError(exec, thisObj));
		return Local<Value>();
	}

	// Build the call's argument list
	JSC::MarkedArgumentBuffer argList;
	for (int i = 0; i < argc; i++)
	{
		argList.append(argv[i].val_);
	}

	/* Call our function, using recv as "this" and our (object's) global as the current context
	 * (not the "context" argument) */
	JSC::JSValue result;
	{
		/* In v8, the function will run in the context it was created with in (thisObj->globalObject() will return
		 * the global of thisObj's structure, which was created by the appropriate global, as all of our structures
		 * are held by a global object instance, for all objects\functions). */
		jscshim::GlobalObject * objectGlobal = static_cast<jscshim::GlobalObject *>(thisObj->globalObject());
		v8::jscshim::Isolate::CurrentContextScope currentContextScope(objectGlobal);

		result = profiledCall(objectGlobal->globalExec(),
							  JSC::ProfilingReason::API,
							  thisObj,
							  callType,
							  callData,
							  recv.val_,
							  argList);
	}
	SHIM_RETURN_IF_EXCEPTION(Local<Value>());

	return Local<Value>::New(result);
}

// Based on JSC's JSObjectCallAsConstructor
MaybeLocal<Value> Object::CallAsConstructor(Local<Context> context, int argc, Local<Value> argv[])
{
	SETUP_OBJECT_USE_IN_MEMBER(context);

	// Get the object's constructor data
	JSC::ConstructData constructData;
	JSC::ConstructType constructType = thisObj->methodTable()->getConstructData(thisObj, constructData);
	if (JSC::ConstructType::None == constructType)
	{
		shimExceptionScope.ThrowExcpetion(JSC::createNotAConstructorError(exec, thisObj));
		return Local<Value>();
	}

	// Build the constructor's argument list
	JSC::MarkedArgumentBuffer argList;
	for (int i = 0; i < argc; i++)
	{
		argList.append(argv[i].val_);
	}

	// Call this function as a constructor, using our (object's) global as the current context (not the "context" argument)
	JSC::JSObject * newObject = nullptr;
	{
		/* In v8, the function will run in the context it was created with in (thisObj->globalObject() will return
		 * the global of thisObj's structure, which was created by the appropriate global, as all of our structures
		 * are held by a global object instance, for all objects\functions). */
		jscshim::GlobalObject * objectGlobal = static_cast<jscshim::GlobalObject *>(thisObj->globalObject());
		v8::jscshim::Isolate::CurrentContextScope currentContextScope(objectGlobal);

		newObject = JSC::profiledConstruct(objectGlobal->globalExec(),
										   JSC::ProfilingReason::API, 
										   thisObj,
										   constructType,
										   constructData,
										   argList);
	}
	SHIM_RETURN_IF_EXCEPTION(Local<Object>());

	return Local<Object>::New(JSC::JSValue(newObject));
}

Isolate * Object::GetIsolate()
{
	SETUP_OBJECT_USE_IN_MEMBER(Isolate::GetCurrent()->GetCurrentContext());
	return jscshim::GetV8ContextForObject(thisObj)->GetIsolate();
}

} // v8