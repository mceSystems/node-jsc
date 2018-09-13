/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"
#include "shim/Object.h"

#include <JavaScriptCore/Weak.h>
#include <JavaScriptCore/Protect.h>
#include <JavaScriptCore/JSCInlines.h>

namespace
{

class PersistentBaseWeakHandleOwner : public JSC::WeakHandleOwner
{
public:
	PersistentBaseWeakHandleOwner() {}

	void finalize(JSC::Handle<JSC::Unknown> handle, void * context) override
	{
		JSC::HandleSlot slot = handle.slot();
		const v8::jscshim::WeakWrapper * wrapper = reinterpret_cast<v8::jscshim::WeakWrapper *>(context);
        
        /* If the isolate is being disposed, we're being called through the VM's destructor.
         * node's "weak callbacks" seem to rely on being called earlier (they access node's Environment object,
         * Isolate, etc.), which might not be legal in our case (acessing already freed object\memory, etc.).
         * So, when the VM is being destroyed we won't call the user supplied callback. This will not
         * protect all cases, but seem to fix the crashes during Isolate->Dispose which were caused by
         * a callback accessing already freed memory\object. */
        if (v8::jscshim::V8IsolateToJscShimIsolate(wrapper->Isolate())->IsDisposing())
        {
            // Deallocate JSC's weak handle
            JSC::WeakSet::deallocate(JSC::WeakImpl::asWeakImpl(slot));
            return;
        }

		// If needed, fill the (future) WeakCallbackInfo's embedder fields
		void * embedderFields[v8::kEmbedderFieldsInWeakCallback]{nullptr};
		if (slot->isCell())
		{
			FillEmbedderFields(wrapper, slot->asCell(), embedderFields);
		}

		/* The user callback might call the PersistentBase's Reset function, which will delete the WeakWrapper.
		 * Thus, we'll store everything we need here, and won't use the WeakWrapper afterwards. */
		v8::WeakCallbackType weakCallbackType = wrapper->Type();
        typename v8::WeakCallbackInfo<void>::Callback callback = wrapper->Callback();
		typename v8::WeakCallbackInfo<void>::Callback secondCallback = nullptr;
		v8::WeakCallbackInfo<void> callbackInfo(wrapper->Isolate(), wrapper->CallbackParameter(), embedderFields, &secondCallback);
		wrapper = nullptr;

		// Invoke the user callback for the first time
		callback(callbackInfo);

		// Deallocate JSC's weak handle
		JSC::WeakSet::deallocate(JSC::WeakImpl::asWeakImpl(slot));

		// If the user has called SetSecondPassCallback, and is allowed to do so - call the second callback
		if (secondCallback)
		{
			if (v8::WeakCallbackType::kFinalizer == weakCallbackType)
			{
				// TODO: This is not possible, right?
			}
			else
			{
				typename v8::WeakCallbackInfo<void>::Callback originalSecondCallback = secondCallback;
				secondCallback(callbackInfo);

				// It's not legal to ask for another callback
				// TODO: This will not catch calling SetSecondCallback again with the same callback
				if (originalSecondCallback != secondCallback)
				{
					WTFLogAlwaysAndCrash("v8::WeakCallbackInfo: SetSecondPassCallback was called on the second pass");
				}
			}
		}
	}

private:
	// Note: Based on v8's GlobalHandles::Node::CollectPhantomCallbackData (global-handles.cc)
	void FillEmbedderFields(const v8::jscshim::WeakWrapper * wrapper, JSC::JSCell * wrappedCell, void * embedderFields)
	{
		// Don't need to copy embedder fields on WeakCallbackType::kParameter (see v8's WeakCallbackType docs in v8.h)
		if (v8::WeakCallbackType::kParameter == wrapper->Type())
		{
			return;
		}

		// Only jscshim objects have embedder\internal fields
		v8::jscshim::Object * wrappedShimObject = JSC::jsDynamicCast<v8::jscshim::Object *>(*wrappedCell->vm(), wrappedCell);
		if (nullptr == wrappedShimObject)
		{
			return;
		}

		auto& objectInternalFields = wrappedShimObject->internalFields();
		size_t fieldCount = objectInternalFields.Size();

		for (size_t i = 0; (i < v8::kEmbedderFieldsInWeakCallback) && (i < fieldCount); i++)
		{
			auto& field = objectInternalFields.GetFieldUnsafe(i);

			// TODO: v8's good only copies the field if field->IsSmi() is true. Should mimic that?
			if (field.m_isJscValue)
			{
				embedderFields = &field.m_data.value;
			}
			else
			{
				embedderFields = field.m_data.rawValue;
			}
		}
	}
};

PersistentBaseWeakHandleOwner g_weakHandleOwner;

}

namespace v8 { namespace jscshim
{

// Note: we count on PersistentBase to make sure "value" is an object
WeakWrapper::WeakWrapper(v8::Isolate * isolate,
						 const JSC::JSValue& value, 
						 WeakCallbackType type, 
						 typename WeakCallbackInfo<void>::Callback callback, 
						 void * callbackParameter) :
	m_isolate(isolate),
	m_weak(value.getObject(), &g_weakHandleOwner, this),
	m_callback(callback),
	m_callbackParameter(callbackParameter),
	m_type(type)
{
}

}} // v8::jscshim
