#include "node_test_fixture.h"

ArrayBufferUniquePtr NodeTestFixture::allocator{nullptr, nullptr};
uv_loop_t NodeTestFixture::current_loop;
#if NODE_USE_V8_PLATFORM
NodePlatformUniquePtr NodeTestFixture::platform;
#endif
TracingControllerUniquePtr NodeTestFixture::tracing_controller;
