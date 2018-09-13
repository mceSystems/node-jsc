/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"
#include "libplatform/v8-tracing.h"

namespace v8 { namespace platform { namespace tracing
{

TracingController::TracingController()
{
}

TracingController::~TracingController()
{
	StopTracing();
}

void TracingController::Initialize(TraceBuffer* trace_buffer)
{
	// TODO: IMPLEMENT
}

void TracingController::StartTracing(TraceConfig* trace_config)
{
	// TODO: IMPLEMENT
}

void TracingController::StopTracing()
{
	// TODO: IMPLEMENT
}

TraceWriter * TraceWriter::CreateJSONTraceWriter(std::ostream& stream)
{
	// TODO: IMPLEMENT
	return nullptr;
}

TraceBufferChunk::TraceBufferChunk(uint32_t seq)
{
	// TODO: IMPLEMENT
}

void TraceBufferChunk::Reset(uint32_t new_seq)
{
	// TODO: IMPLEMENT
}

void TraceConfig::AddIncludedCategory(const char* included_category)
{
	// TODO: IMPLEMENT
}

}}} // v8::platform::tracing