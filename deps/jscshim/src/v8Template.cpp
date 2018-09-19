/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/Template.h"
#include "shim/helpers.h"

#include <JavaScriptCore/JSCInlines.h>

#define GET_JSC_THIS() v8::jscshim::GetJscCellFromV8<jscshim::Template>(this)

/* Note: We currently treat AccessorNameGetterCallback/AccessorNameSetterCallback and 
 *		 AccessorGetterCallback/AccessorSetterCallback the same. This is hacky, but currently
 *		 work, as the only difference is in the name paramter (Name vs. String), 
 *		 so casting should be OK as long as we match the expected type when creating the accesor (which we do).*/

namespace v8
{

void Template::Set(Local<Name> name, Local<Data> value, PropertyAttribute attributes)
{
	GET_JSC_THIS()->SetProperty(jscshim::GetCurrentExecState(),
								name.val_.asCell(),
								value.val_,
								jscshim::v8PropertyAttributesToJscAttributes(attributes));
}

// TODO: Handle settings (AccessControl)
void Template::SetAccessorProperty(Local<Name> name,
								   Local<FunctionTemplate> getter,
								   Local<FunctionTemplate> setter,
								   PropertyAttribute attribute,
								   AccessControl settings)
{
	GET_JSC_THIS()->SetAccessorProperty(jscshim::GetCurrentExecState(),
										name.val_.asCell(),
										jscshim::GetJscCellFromV8<jscshim::FunctionTemplate>(*getter),
										jscshim::GetJscCellFromV8<jscshim::FunctionTemplate>(*setter),
										jscshim::v8PropertyAttributesToJscAttributes(attribute));
}

void Template::SetNativeDataProperty(Local<String> name,
									 AccessorGetterCallback getter,
									 AccessorSetterCallback setter,
									 Local<Value> data,
									 PropertyAttribute attribute,
									 Local<AccessorSignature> signature,
									 AccessControl settings)
{
	GET_JSC_THIS()->SetAccessor(jscshim::GetCurrentExecState(),
								name.val_.asCell(),
								reinterpret_cast<AccessorNameGetterCallback>(getter),
								reinterpret_cast<AccessorNameSetterCallback>(setter),
								data.val_,
								jscshim::GetJscCellFromV8<jscshim::FunctionTemplate>(*signature),
								jscshim::v8PropertyAttributesToJscAttributes(attribute),
								false);

}
void Template::SetNativeDataProperty(Local<Name> name, 
									 AccessorNameGetterCallback getter,
									 AccessorNameSetterCallback setter,
									 Local<Value> data, 
									 PropertyAttribute attribute,
									 Local<AccessorSignature> signature,
									 AccessControl settings)
{
	GET_JSC_THIS()->SetAccessor(jscshim::GetCurrentExecState(),
								name.val_.asCell(),
								getter,
								setter,
								data.val_,
								jscshim::GetJscCellFromV8<jscshim::FunctionTemplate>(*signature),
								jscshim::v8PropertyAttributesToJscAttributes(attribute),
								true);
}

} // v8