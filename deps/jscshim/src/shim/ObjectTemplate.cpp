/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "ObjectTemplate.h"
#include "Object.h"
#include "ObjectWithInterceptors.h"
#include "FunctionTemplate.h"
#include "Function.h"

#include <JavaScriptCore/Error.h>
#include <JavaScriptCore/JSCInlines.h>

namespace v8 { namespace jscshim
{

const JSC::ClassInfo ObjectTemplate::s_info = { "JSCShimObjectTemplate", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(ObjectTemplate) };

void ObjectTemplate::finishCreation(JSC::VM& vm, FunctionTemplate * constructor)
{
	Base::finishCreation(vm, this->classInfo()->className);

	m_constructor.setMayBeNull(vm, this, constructor);
}

void ObjectTemplate::visitChildren(JSC::JSCell* cell, JSC::SlotVisitor& visitor)
{
	Base::visitChildren(cell, visitor);

	ObjectTemplate * thisTemplate = JSC::jsCast<ObjectTemplate *>(cell);
	visitor.append(thisTemplate->m_objectStructure);
}

void ObjectTemplate::destroy(JSC::JSCell* cell)
{
	static_cast<ObjectTemplate*>(cell)->~ObjectTemplate();
}

// TODO: Handle hidden prototypes
Object * ObjectTemplate::makeNewObjectInstance(JSC::ExecState * exec, JSC::JSValue newTarget, Function * constructorInstance)
{
	bool haveInterceptors = m_namedInterceptors || m_indexedInterceptors;

	// If we have a FunctionTemplate (owner) but not a Function instance - create one to be used a constructor
	if ((nullptr == constructorInstance) && m_constructor)
	{
		constructorInstance = m_constructor->makeNewFunctionInstance(exec);
	}

	// Create the new instance object, with or without interceptors
	Object * newInstance = nullptr;
	JSC::Structure * newObjectStructure = getStructrueForNewInstance(exec, newTarget, haveInterceptors, constructorInstance);
	if (haveInterceptors)
	{
		newInstance = jscshim::ObjectWithInterceptors::create(exec->vm(),
															  newObjectStructure, 
															  this,
															  m_namedInterceptors.get(),
															  m_indexedInterceptors.get());
	}
	else
	{
		newInstance = jscshim::Object::create(exec->vm(), newObjectStructure, this);
	}

	// Apply our (and our parents) properties to the new instance
	applyPropertiesIncludingParentsToInstance(exec, newInstance);

	return newInstance;
}

int ObjectTemplate::internalFieldCount() const
{
	return m_internalFieldCount;
}

void ObjectTemplate::setInternalFieldCount(int newCount)
{
	// v8 doens't verify the template wasn't Instantiated here, but we'll check
	ensureNotInstantiated("v8::ObjectTemplate::SetInternalFieldCount");

	if (newCount > 0)
	{
		m_internalFieldCount = newCount;
	}
}

void ObjectTemplate::setInterceptors(const NamedPropertyHandlerConfiguration& configuration)
{
	ensureNotInstantiated("v8::ObjectTemplate::SetHandler");
	m_namedInterceptors.reset(new NamedInterceptorInfo(configuration));
}

void ObjectTemplate::setInterceptors(const IndexedPropertyHandlerConfiguration& configuration)
{
	ensureNotInstantiated("v8::ObjectTemplate::SetHandler");
	m_indexedInterceptors.reset(new IndexedInterceptorInfo(configuration));
}

void ObjectTemplate::setCallAsFunctionHandler(v8::FunctionCallback callback, JSC::JSValue data)
{
	ensureNotInstantiated("v8::ObjectTemplate::SetCallAsFunctionHandler");

	m_callAsFunctionCallback = callback;
	m_callAsFunctionCallbackData.set(*vm(), this, data);
}

JSC_HOST_CALL JSC::EncodedJSValue ObjectTemplate::DummyCallback(JSC::ExecState * exec)
{
	auto scope = DECLARE_THROW_SCOPE(exec->vm());
	return JSC::throwVMError(exec, scope, "v8::ObjectTemplate called as a function\\constructor");
}

JSC::EncodedJSValue JSC_HOST_CALL ObjectTemplate::instanceFunctionCall(JSC::ExecState * exec)
{
	return handleInstanceCall(exec, false);
}

JSC::EncodedJSValue JSC_HOST_CALL ObjectTemplate::instanceConstructorCall(JSC::ExecState * exec)
{
	return handleInstanceCall(exec, true);
}

inline JSC::EncodedJSValue JSC_HOST_CALL ObjectTemplate::handleInstanceCall(JSC::ExecState * exec, bool isConstructor)
{
	Object * instance = JSC::jsDynamicCast<Object *>(exec->vm(), exec->jsCallee());
	ASSERT(instance);

	ObjectTemplate * functionTemplate = instance->objectTemplate();
	ASSERT(functionTemplate->m_callAsFunctionCallback);

	JSC::JSValue thisValue(instance);
	JSC::JSValue returnValue = functionTemplate->forwardCallToCallback(exec, thisValue, thisValue, isConstructor);

	return JSC::JSValue::encode(returnValue ? returnValue : JSC::jsUndefined());
}

JSC::Structure * ObjectTemplate::createObjectStructure(jscshim::GlobalObject * global, bool withInterceptors, JSC::JSValue prototype)
{
	if (withInterceptors)
	{
		return jscshim::ObjectWithInterceptors::createStructure(global->vm(), global, prototype, !!m_callAsFunctionCallback);
	}

	return jscshim::Object::createStructure(global->vm(), global, prototype, !!m_callAsFunctionCallback);
}

JSC::Structure * ObjectTemplate::getDefaultObjectStructure(jscshim::GlobalObject * global, bool withInterceptors)
{
	if (!m_objectStructure)
	{
		// Note that we need "global->objectPrototype" for the "__proto__" property accessor
		m_objectStructure.set(global->vm(), 
							  this, 
							  createObjectStructure(global, withInterceptors, global->objectPrototype()));
	}

	return m_objectStructure.get();
}

JSC::Structure * ObjectTemplate::getStructrueForNewInstance(JSC::ExecState	* exec, 
															JSC::JSValue	newTarget, 
															bool			haveInterceptors,
															Function		* constructorInstance)
{
	jscshim::GlobalObject * global = GetGlobalObject(exec);

	JSC::Structure * baseStructure = nullptr;
	if (constructorInstance)
	{
		baseStructure = constructorInstance->structureForObjects();
		if (nullptr == baseStructure)
		{
			JSC::VM& vm = exec->vm();

			/* This is the first instance created with this function instance, so we need to create a 
			 * new structure */
			JSC::JSValue prototype = constructorInstance->get(exec, vm.propertyNames->prototype);
			if (prototype.isObject())
			{
				baseStructure = createObjectStructure(global, haveInterceptors, prototype);
			}
			else
			{
				baseStructure = getDefaultObjectStructure(global, haveInterceptors);
			}

			// Cache the new structure for the next instances
			constructorInstance->setStructureForObjects(vm, baseStructure);
		}
	}
	else
	{
		baseStructure = getDefaultObjectStructure(global, haveInterceptors);
	}

	// Finally, create the new instance object (with or without interceptors)
	return createSubclassStructure(exec, newTarget, baseStructure);
}

/* According the v8's (FunctionTemplate) docs: "An instance of the Child function 
 * has all properties on Parent's instance templates". 
 * Note that because a template can override a parent property, we'll apply the properties
 * in reverse order (parents first, which means from the top most parent to our template). */
void ObjectTemplate::applyPropertiesIncludingParentsToInstance(JSC::ExecState * exec, JSC::JSObject * instance)
{
	if (m_constructor)
	{
		FunctionTemplate * currentParentConstructor = m_constructor->parentTemplate();
		if (currentParentConstructor)
		{
			currentParentConstructor->getInstnaceTemplate(exec)->applyPropertiesIncludingParentsToInstance(exec, instance);
		}
	}
	
	// Apply our properties to the new instance
	applyPropertiesToInstance(exec, instance);
}

}} // v8::jscshim