/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "util.h"

#include <JavaScriptCore/PropertyDescriptor.h>
#include <JavaScriptCore/JSObject.h>

#define MAGIC "!JSCSHIM_JSC_TYPES_START!"
#define MAX_TYPE_NAME_SIZE (120)
#define MAX_JSC_TYPES_COUNT (64)

PACK_STRUCT(struct GeneratorTypesData
{
	PACK_STRUCT(struct JSCType
	{
		char declaration[MAX_TYPE_NAME_SIZE];
		unsigned int size;
	})

	char m_magic[sizeof(MAGIC)];
	JSCType m_types[MAX_JSC_TYPES_COUNT];
})

#define MAKE_JSC_TYPE(typeDeclaration, typeName) { STRINGIFY(typeDeclaration) " " STRINGIFY(typeName), JSCHIM_HTONL(sizeof(typeName)) }
#define MAKE_JSC_TYPE_WITHOUT_SIZE(type) { STRINGIFY(type), 0 }
#define MAKE_END_TYPE_MARKER() {"", 0}

namespace JSC
{

const GeneratorTypesData SHIM_JSC_TYPES = {
	MAGIC,
	{
		MAKE_JSC_TYPE(class, JSObject),
		MAKE_JSC_TYPE(class, PropertyDescriptor),
		MAKE_JSC_TYPE(class, PropertyName),
		
		MAKE_JSC_TYPE_WITHOUT_SIZE(enum class Unknown),
		MAKE_JSC_TYPE_WITHOUT_SIZE(template <typename T> class Weak),
		MAKE_JSC_TYPE(template <> class, Weak<Unknown>),
		
		MAKE_END_TYPE_MARKER()
	}
};

}

int main()
{
	printf("%s\n", JSC::SHIM_JSC_TYPES.m_types[0].declaration);
	return 0;
}

