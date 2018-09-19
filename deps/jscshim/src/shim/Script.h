/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include <v8.h>
#include <JavaScriptCore/JSDestructibleObject.h>
#include <JavaScriptCore/JSString.h>

namespace v8 { namespace jscshim
{

class Script : public JSC::JSDestructibleObject {
private:
	JSC::WriteBarrier<JSC::JSString> m_source;
	JSC::WriteBarrier<JSC::JSString> m_resourceName;
	JSC::WriteBarrier<JSC::JSString> m_sourceMapUrl;
	unsigned int m_resourceLine;
	unsigned int m_resourceColumn;

public:
	typedef JSDestructibleObject Base;

	static Script * create(JSC::VM&		  vm, 
						   JSC::Structure * structure, 
						   JSC::JSString  * source, 
						   JSC::JSString  * resourceName, 
						   JSC::JSString  * sourceMapUrl, 
						   int			  resourceLine, 
						   int			  resourceColumn)
	{
		Script* cell = new (NotNull, JSC::allocateCell<Script>(vm.heap)) Script(vm, structure);
		cell->finishCreation(vm, source, resourceName, sourceMapUrl, resourceLine, resourceColumn);
		return cell;
	}

	DECLARE_INFO;

	static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
	{
		return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
	}

	JSC::JSString * source() const { return m_source.get(); }
	JSC::JSString * resourceName() const { return m_resourceName.get(); }
	JSC::JSString * sourceMapUrl() const { return m_sourceMapUrl.get(); }
	unsigned int resourceLine() const { return m_resourceLine; }
	unsigned int resourceColumn() const { return m_resourceColumn; }

private:
	Script(JSC::VM& vm, JSC::Structure* structure) : Base(vm, structure),
		m_resourceLine(v8::Message::kNoLineNumberInfo),
		m_resourceColumn(v8::Message::kNoColumnInfo)
	{
	}

	void finishCreation(JSC::VM&	  vm, 
						JSC::JSString * source, 
						JSC::JSString * resourceName, 
						JSC::JSString * sourceMapUrl, 
						int			  resourceLine, 
						int			  resourceColumn);

	static void visitChildren(JSC::JSCell*, JSC::SlotVisitor&);
};

}}