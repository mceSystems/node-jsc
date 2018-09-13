/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"
#include <JavaScriptCore/JSDataView.h>
#include <JavaScriptCore/JSArrayBuffer.h>
#include <JavaScriptCore/JSCInlines.h>

namespace
{

template <typename T>
inline JSC::JSObject * CreateJscDateView(T * v8ArrayBuffer, size_t byteOffset, size_t length)
{
	JSC::ExecState * exec = v8::jscshim::GetCurrentExecState();
	JSC::JSArrayBuffer* jsBuffer = v8::jscshim::GetJscCellFromV8<JSC::JSArrayBuffer>(v8ArrayBuffer);
	DECLARE_SHIM_EXCEPTION_SCOPE(exec);

	return JSC::JSDataView::create(exec,
									exec->lexicalGlobalObject()->typedArrayStructure(JSC::TypeDataView),
									jsBuffer->impl(),
									byteOffset,
									length);
}

}

namespace v8
{

Local<DataView> DataView::New(Local<ArrayBuffer> array_buffer,
							  size_t byte_offset, size_t length)
{
	return Local<DataView>(JSC::JSValue(CreateJscDateView(*array_buffer, byte_offset, length)));
}

Local<DataView> DataView::New(Local<SharedArrayBuffer> shared_array_buffer,
							  size_t byte_offset, size_t length)
{
	return Local<DataView>(JSC::JSValue(CreateJscDateView(*shared_array_buffer, byte_offset, length)));
}

} // v8