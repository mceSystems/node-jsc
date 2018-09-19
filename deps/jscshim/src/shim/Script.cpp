/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "Script.h"

#include <JavaScriptCore/JSCInlines.h>

namespace v8 { namespace jscshim
{

const JSC::ClassInfo Script::s_info = { "JSCShimScript", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(Script) };

void Script::finishCreation(JSC::VM&	  vm, 
							JSC::JSString * source, 
							JSC::JSString * resourceName, 
							JSC::JSString * sourceMapUrl, 
							int			  resourceLine, 
							int			  resourceColumn)
{
	Base::finishCreation(vm);

	m_source.set(vm, this, source);
	m_resourceName.setMayBeNull(vm, this, resourceName);
	m_sourceMapUrl.setMayBeNull(vm, this, sourceMapUrl);
	m_resourceLine = resourceLine;
	m_resourceColumn = resourceColumn;
}

void Script::visitChildren(JSC::JSCell* cell, JSC::SlotVisitor& visitor)
{
	Base::visitChildren(cell, visitor);

	Script * thisObject = JSC::jsCast<Script *>(cell);
	visitor.append(thisObject->m_source);
	visitor.append(thisObject->m_resourceName);
	visitor.append(thisObject->m_sourceMapUrl);
}

}} // v8::jscshim