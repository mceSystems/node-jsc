/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include "v8.h"

namespace v8 { namespace jscshim
{

// Named property interceptor (callback) types
struct NamedInterceptorInfoTypes
{
	using v8ConfigurationType = NamedPropertyHandlerConfiguration;
	using Getter = GenericNamedPropertyGetterCallback;
	using Setter = GenericNamedPropertySetterCallback;
	using Query = GenericNamedPropertyQueryCallback;
	using Deleter = GenericNamedPropertyDeleterCallback;
	using Enumerator = GenericNamedPropertyEnumeratorCallback;
	using Definer = GenericNamedPropertyDefinerCallback;
	using Descriptor = GenericNamedPropertyDescriptorCallback;
};

// Indexed property interceptor (callback) types
struct IndexedInterceptorInfoTypes
{
	using v8ConfigurationType = IndexedPropertyHandlerConfiguration;
	using Getter = IndexedPropertyGetterCallback;
	using Setter = IndexedPropertySetterCallback;
	using Query = IndexedPropertyQueryCallback;
	using Deleter = IndexedPropertyDeleterCallback;
	using Enumerator = IndexedPropertyEnumeratorCallback;
	using Definer = IndexedPropertyDefinerCallback;
	using Descriptor = IndexedPropertyDescriptorCallback;
};

// Generic property interceptor info, used by jscshim::ObjectTemplate to store named\indexed property interceptors
template <typename CallbackTypes>
struct InterceptorInfo
{
	InterceptorInfo(const typename CallbackTypes::v8ConfigurationType& info) :
		getter(info.getter),
		setter(info.setter),
		query(info.query),
		deleter(info.deleter),
		enumerator(info.enumerator),
		definer(info.definer),
		descriptor(info.descriptor),
		data(info.data.val_),
		flags(info.flags)
	{
	}

	typename CallbackTypes::Getter getter;
	typename CallbackTypes::Setter setter;
	typename CallbackTypes::Query query;
	typename CallbackTypes::Deleter deleter;
	typename CallbackTypes::Enumerator enumerator;
	typename CallbackTypes::Definer definer;
	typename CallbackTypes::Descriptor descriptor;
	JSC::JSValue data;
	PropertyHandlerFlags flags;
};

using NamedInterceptorInfo = InterceptorInfo<NamedInterceptorInfoTypes>;
using IndexedInterceptorInfo = InterceptorInfo<IndexedInterceptorInfoTypes>;

}} // v8::jscshim