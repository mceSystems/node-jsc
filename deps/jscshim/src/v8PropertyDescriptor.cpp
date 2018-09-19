/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"

#include <JavaScriptCore/PropertyDescriptor.h>
#include <JavaScriptCore/JSCInlines.h>

namespace v8
{

PropertyDescriptor::PropertyDescriptor() : 
	private_(new PrivateData()),
	isPrivateDataOwner_(true)
{
}

PropertyDescriptor::PropertyDescriptor(Local<Value> value) :
	private_(new PrivateData()),
	isPrivateDataOwner_(true),
	value_(value.val_)
{
	private_->setValue(value.val_);
}

PropertyDescriptor::PropertyDescriptor(Local<Value> value, bool writable) :
	PropertyDescriptor(value)
{
	private_->setWritable(writable);
}

PropertyDescriptor::PropertyDescriptor(Local<Value> get, Local<Value> set) :
	private_(new PrivateData()),
	isPrivateDataOwner_(true),
	getter_(get.val_),
	setter_(set.val_)
{
	private_->setGetter(get.val_);
	private_->setSetter(set.val_);
}

PropertyDescriptor::PropertyDescriptor(PrivateData * existingPrivateData) :
	private_(existingPrivateData),
	isPrivateDataOwner_(false)
{
}

PropertyDescriptor::~PropertyDescriptor()
{
	if (isPrivateDataOwner_)
	{
		delete private_;
	}
}

Local<Value> PropertyDescriptor::value() const
{
	return Local<Value>::New(private_->value());
}

bool PropertyDescriptor::has_value() const
{
	return !!private_->value();
}

Local<Value> PropertyDescriptor::get() const
{
	return Local<Value>::New(private_->getter());
}

bool PropertyDescriptor::has_get() const
{
	return private_->getterPresent();
}

Local<Value> PropertyDescriptor::set() const
{
	return Local<Value>::New(private_->setter());
}

bool PropertyDescriptor::has_set() const
{
	return private_->setterPresent();
}

void PropertyDescriptor::set_enumerable(bool enumerable)
{
	private_->setEnumerable(enumerable);
}

bool PropertyDescriptor::enumerable() const
{
	return private_->enumerable();
}

bool PropertyDescriptor::has_enumerable() const
{
	return private_->enumerablePresent();
}

void PropertyDescriptor::set_configurable(bool configurable)
{
	private_->setConfigurable(configurable);
}

bool PropertyDescriptor::configurable() const
{
	return private_->configurable();
}

bool PropertyDescriptor::has_configurable() const
{
	return private_->configurablePresent();
}

bool PropertyDescriptor::writable() const
{
	return private_->writable();
}

bool PropertyDescriptor::has_writable() const
{
	return private_->writablePresent();
}

} // v8