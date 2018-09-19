/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include "v8.h"
#include "Template.h"
#include "ObjectTemplate.h"
#include "helpers.h"

#include <JavaScriptCore/JSDestructibleObject.h>

namespace v8 { namespace jscshim
{
class Function;

class FunctionTemplate : public Template {
private:
	enum class TemplateFlags
	{
		None = 0x0,
		RemovePrototype = 0x1,
		ReadOnlyPrototype = 0x2,
		HiddenPrototype = 0x4,

		/* In JSC (and v8) objects can have a class name, used in places like exception messages.
		 * In JSC, this is usually provided by the object's contructor function (see JSC::JSObject::calculatedClassName
		 * for example). JSC::InternalFunction exposes "calculatedDisplayName" function, 
		 * which tries the functions's "displayName" first and fallbacks to the class name passed to it's 
		 * (c++) constructor. 
		 * In JSC, the default name for objects is "Object" and "Function" for functions. This leaves
		 * us with a problem: If we're "just" a function, we should return "Function", but if we're
		 * used as a constructor, we should return the user's class name, or the default "Object".
		 * Since we don't have a definite way to know, we'll make an educated guess: If our instance
		 * or prototype templates were accessed (therefore created), or someone called setTemplateClassName,
		 * and "remove prototype" flag wasn't set, we'll assume we should behave like a constructor here. */
		ProbablyIsConstructor = 0x8
	};

private:
	friend class Function;

	// Store the length to speedup Function creation (instances needs length)
	int m_length;

	/* TODO: Since prototype template and prototype provider template are mutually exclusive, 
	 * should we group them in a union? */
	JSC::WriteBarrier<FunctionTemplate> m_parentTemplate;
	JSC::WriteBarrier<FunctionTemplate> m_prototypeProviderTemplate;
	JSC::WriteBarrier<ObjectTemplate> m_prototypeTemplate;
	JSC::WriteBarrier<ObjectTemplate> m_instanceTemplate;
	JSC::WriteBarrier<FunctionTemplate> m_signature;
	JSC::WriteBarrier<JSC::JSString> m_templateClassName;
	
	unsigned char m_flags;

public:
	typedef Template Base;

	template<typename CellType>
	static JSC::IsoSubspace * subspaceFor(JSC::VM& vm)
	{
		jscshim::Isolate * currentIsolate = jscshim::Isolate::GetCurrent();
		RELEASE_ASSERT(&currentIsolate->VM() == &vm);

		return currentIsolate->FunctionTemplateSpace();
	}

	static FunctionTemplate* create(JSC::VM&			 vm, 
									JSC::Structure		 * structure, 
									JSC::ExecState		 * exec, 
									v8::FunctionCallback callback, 
									int					 length, 
									JSC::JSValue		 data, 
									FunctionTemplate	 * signature)
	{
		GlobalObject * global = GetGlobalObject(exec);

		FunctionTemplate* cell = new (NotNull, JSC::allocateCell<FunctionTemplate>(vm.heap)) FunctionTemplate(vm, 
																											  structure, 
																											  global->isolate(), 
																											  callback, 
																											  length);
		cell->finishCreation(vm, length, data, signature);
		return cell;
	}

	DECLARE_INFO;

	static JSC::Structure * createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
	{
		return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::InternalFunctionType, StructureFlags), info());
	}

	Function * makeNewFunctionInstance(JSC::ExecState * exec);

	bool isTemplateFor(jscshim::Object * object);
	bool isTemplateFor(JSC::JSValue value);

	// Will crash if already instantiated
	void setCallHandler(v8::FunctionCallback callback, JSC::JSValue data);
	void setLength(int length);
	void setTemplateClassName(JSC::VM& vm, JSC::JSString * className);
	JSC::JSString * templateClassName() const { return m_templateClassName.get(); }
	void setParentTemplate(JSC::VM& vm, FunctionTemplate * parent);
	FunctionTemplate * parentTemplate() const { return m_parentTemplate.get(); }
	void setName(JSC::VM& vm, JSC::JSString * name);

	void setPrototypeProviderTemplate(JSC::ExecState * exec, FunctionTemplate * prototypeProviderTemplate);

	ObjectTemplate * getPrototypeTemplate(JSC::ExecState * exec);
	ObjectTemplate * getInstnaceTemplate(JSC::ExecState * exec);

	bool removePrototype()   const { return !!(m_flags & static_cast<unsigned char>(TemplateFlags::RemovePrototype)); }
	bool readOnlyPrototype() const { return !!(m_flags & static_cast<unsigned char>(TemplateFlags::ReadOnlyPrototype)); }
	bool hiddenPrototype()   const { return !!(m_flags & static_cast<unsigned char>(TemplateFlags::HiddenPrototype)); }

	void setRemovePrototype(bool value);
	void setReadOnlyPrototype(bool value);
	void setHiddenPrototype(bool value);

	// A utility for getting the call "holder" (and signature verfication) when handling calls
	static JSC::JSValue GetCompatibleReceiver(JSC::VM& vm, JSC::JSValue receiver, v8::jscshim::FunctionTemplate * signature);
	
private:
	/* Note that while FunctionTemplate can be called as functions (when used in an accessor, for example), 
	 * they can't be called as constructors (passing callHostFunctionAsConstructor will cause our getConstructData
	 * to return ConstructType::None) */
	FunctionTemplate(JSC::VM&			  vm, 
					 JSC::Structure		  * structure,
					 Isolate			  * isolate, 
					 v8::FunctionCallback callback, 
					 int				  length) : Base(vm, structure, isolate, functionCall, JSC::callHostFunctionAsConstructor),
		m_length(length),
		m_flags(static_cast<unsigned char>(TemplateFlags::None))
	{
		m_callAsFunctionCallback = callback;
	}

	static JSC::EncodedJSValue JSC_HOST_CALL functionCall(JSC::ExecState * exec);

	JSC::EncodedJSValue handleFunctionCall(JSC::ExecState * exec);

	void finishCreation(JSC::VM& vm, int length, JSC::JSValue data, FunctionTemplate * signature);
	static void visitChildren(JSC::JSCell*, JSC::SlotVisitor&);

	JSC::JSObject * instantiatePrototype(JSC::ExecState * exec, jscshim::Function * constructor);
};

}}