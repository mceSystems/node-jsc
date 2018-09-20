/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"

// TODO: FIXME: This solves missing "CString" compilation error
#include <JavaScriptCore/JSCJSValue.h>

#include <JavaScriptCore/JSString.h>
#include <JavaScriptCore/JSStringInlines.h>
#include <JavaScriptCore/Operations.h>
#include <JavaScriptCore/FrameTracers.h>
#include <JavaScriptCore/JSCInlines.h>
#include <wtf/text/ExternalStringImpl.h>
#include <wtf/unicode/UTF8.h>
#include <wtf/unicode/CharacterNames.h>
#include <wtf/HashMap.h>

static_assert(sizeof(uint16_t) == sizeof(UChar), "uint16_t and UChar should be the same size");

// TODO: In member functions, we always access the current ExecState. Is it OK?

namespace
{

// TODO: Find a better solution for "attaching" a resource instance to a JSString?
WTF::HashMap<const WTF::ExternalStringImpl *, v8::String::ExternalStringResourceBase *> g_externalStringsResources;

// Based on JSC's Identifier canUseSingleCharacterString
template <typename T> ALWAYS_INLINE bool canUseSingleCharacterString(T);
template <> ALWAYS_INLINE bool canUseSingleCharacterString(LChar) { return true; }
template <> ALWAYS_INLINE bool canUseSingleCharacterString(UChar c) { return (c <= JSC::maxSingleCharacterString); }

// Based on JSC's Identifier::add (the non accessible version)
template <typename T>
Ref<StringImpl> GetAtomicString(JSC::ExecState * exec, const T * s, int length)
{
	if (length == 1) 
	{
		T c = s[0];
		if (canUseSingleCharacterString(c))
		{
			return exec->vm().smallStrings.singleCharacterStringRep(c);
		}
				
	}

	if (0 == length)
	{
		return *AtomicStringImpl::empty();
	}
			
	return *AtomicStringImpl::add(s, length);
}

inline const WTF::String& GetResolvedStringValue(const v8::String * v8String)
{
	return v8::jscshim::GetJscCellFromV8<JSC::JSString>(v8String)->tryGetValue();
}

inline const WTF::StringImpl * GetResolvedStringImpl(const v8::String * v8String)
{
	return GetResolvedStringValue(v8String).impl();
}

template <typename T>
inline void CopyChars(T * destination, const T * source, unsigned int numbOfCharacters)
{
	StringImpl::copyCharacters<T>(destination, source, numbOfCharacters);
}

inline void CopyChars(UChar * destination, const LChar * source, unsigned int numbOfCharacters)
{
	StringImpl::copyCharacters(destination, source, numbOfCharacters);
}

inline void CopyChars(LChar * destination, const UChar * source, unsigned int numbOfCharacters)
{
	WTF::copyLCharsFromUCharSource(destination, source, numbOfCharacters);
}

// Based on Spildershim's "Write", which is based on WriteHelper in V8's api.cc.
template <typename CharType>
inline int WriteHelper(const v8::String * string, CharType * buffer, int start, int length, int options)
{
	const WTF::StringImpl * thisImpl = GetResolvedStringImpl(string);
	int strLength = thisImpl->length();

	int end = start + length;
	if ((-1 == length) || (length > strLength - start))
	{
		end = strLength;
	}
	if (end < 0)
	{
		return 0;
	}

	if (thisImpl->is8Bit())
	{
		CopyChars(buffer, thisImpl->characters8() + start, end - start);
	}
	else
	{
		CopyChars(buffer, thisImpl->characters16() + start, end - start);
	}

	if (!(options & v8::String::NO_NULL_TERMINATION) &&
		(length == -1 || end - start < length))
	{
		buffer[end - start] = '\0';
	}

	return end - start;
}

// Taken from SpiderMonkey, since JSC only have conversion functions
// TODO: Make sure this matches JSC's string conversion (the size of the converted string)
template <typename CharT>
int GetDeflatedUTF8StringLength(const CharT* chars, int nchars)
{
	int nbytes = nchars;
	for (const CharT* end = chars + nchars; chars < end; chars++) {
		char16_t c = *chars;
		if (c < 0x80)
			continue;
		uint32_t v;
		if (0xD800 <= c && c <= 0xDFFF) {
			/* nbytes sets 1 length since this is surrogate pair. */
			if (c >= 0xDC00 || (chars + 1) == end) {
				nbytes += 2; /* Bad Surrogate */
				continue;
			}
			char16_t c2 = chars[1];
			if (c2 < 0xDC00 || c2 > 0xDFFF) {
				nbytes += 2; /* Bad Surrogate */
				continue;
			}
			v = ((c - 0xD800) << 10) + (c2 - 0xDC00) + 0x10000;
			nbytes--;
			chars++;
		}
		else {
			v = c;
		}
		v >>= 11;
		nbytes++;
		while (v) {
			v >>= 5;
			nbytes++;
		}
	}
	return nbytes;
}

// Taken from WTF's StringImpl.cpp
// Helper to write a three-byte UTF-8 code point to the buffer, caller must check room is available.
inline void putUTF8Triple(char*& buffer, UChar ch)
{
	ASSERT(ch >= 0x0800);
	*buffer++ = static_cast<char>(((ch >> 12) & 0x0F) | 0xE0);
	*buffer++ = static_cast<char>(((ch >> 6) & 0x3F) | 0x80);
	*buffer++ = static_cast<char>((ch & 0x3F) | 0x80);
}

template <typename CharType, typename ExternalResourceType>
JSC::JSValue CreateExternalStringHelper(v8::Isolate * isolate, ExternalResourceType * resource)
{
	JSC::ExecState * exec = v8::jscshim::GetExecStateForV8Context(*isolate->GetCurrentContext());

	WTF::Ref<StringImpl> newStringImpl = ExternalStringImpl::create(reinterpret_cast<const CharType *>(resource->data()),
																										resource->length(), 
																										[resource](ExternalStringImpl * externalStringImpl, void * buffer, unsigned bufferSize) -> void {
		resource->Dispose();
		g_externalStringsResources.remove(externalStringImpl);
	});

	g_externalStringsResources.add(static_cast<ExternalStringImpl *>(newStringImpl.ptr()), resource);
		
	JSC::JSString * newString = JSC::JSString::create(exec->vm(), WTFMove(newStringImpl));
	return JSC::JSValue(newString);
}

} // (anonymous namespace)

namespace v8
{

int String::Length() const
{
	return jscshim::GetJscCellFromV8<JSC::JSString>(this)->length();
}

int String::Utf8Length() const
{
	return Utf8Length(Isolate::GetCurrent());
}

int String::Utf8Length(Isolate * isolate) const
{
	const WTF::StringImpl * thisImpl = GetResolvedStringImpl(this);
	return thisImpl->is8Bit() ? GetDeflatedUTF8StringLength(thisImpl->characters8(), thisImpl->length()) :
								GetDeflatedUTF8StringLength(thisImpl->characters16(), thisImpl->length());
}

bool String::IsOneByte() const
{
	return GetResolvedStringValue(this).is8Bit();
}

Local<String> String::Empty(Isolate * isolate)
{	
	JSC::VM& vm = jscshim::V8IsolateToJscShimIsolate(isolate)->VM();
	JSC::JSString * newString = JSC::jsEmptyString(&vm);

	return Local<String>::New(JSC::JSValue(newString));
}

int String::Write(Isolate * isolate, uint16_t * buffer, int start, int length, int options) const
{
	return WriteHelper(this, reinterpret_cast<UChar *>(buffer), start, length, options);
}

int String::Write(uint16_t * buffer, int start, int length, int options) const
{
	return Write(Isolate::GetCurrent(), buffer, start, length, options);
}

int String::WriteOneByte(Isolate * isolate, uint8_t * buffer, int start, int length, int options) const
{
	return WriteHelper(this, reinterpret_cast<LChar *>(buffer), start, length, options);
}

int String::WriteOneByte(uint8_t * buffer, int start, int length, int options) const
{
	return WriteOneByte(Isolate::GetCurrent(), buffer, start, length, options);
}

// Based on StringImpl::utf8ForRange, StringImpl::utf8Impl, and spideshim's WriteUtf8
int String::WriteUtf8(Isolate * isolate, char * buffer, int length, int * nchars_ref, int options) const
{
	const WTF::StringImpl * thisImpl = GetResolvedStringImpl(this);

	bool nullTermination = !(options & NO_NULL_TERMINATION);

	// The number of bytes written to the buffer. convertUTF16ToUTF8
	// expects the initial value of this variable to be the buffer's length,
	// and it uses the value to determine whether/when to abort writing bytes
	// to the buffer.  However -1 is a valid argument to the v8 method, so we
	// need to use the maximum possible length in that case.
	size_t numBytes = (length >= 0) ? (size_t)length : (SIZE_MAX - (size_t)buffer);
	
	// The number of Unicode characters written to the buffer. This could be
	// less than the length of the string, if the buffer isn't big enough to hold
	// the whole string.
	size_t numChars = 0;

	WTF::Unicode::ConversionResult result = WTF::Unicode::sourceIllegal;
	if (thisImpl->is8Bit())
	{
		const LChar * sourceBuffer = thisImpl->characters8();
		const LChar * currentSourceBufferPosition = sourceBuffer;
		char * currentTargetBuffer = buffer;

		result = WTF::Unicode::convertLatin1ToUTF8(&currentSourceBufferPosition, sourceBuffer + thisImpl->length(), &currentTargetBuffer, buffer + numBytes);
		numChars = currentSourceBufferPosition - sourceBuffer;
		numBytes = currentTargetBuffer - buffer;
	}
	// TODO: Should we do a strict conversion?
	else
	{
		const UChar * sourceBuffer = thisImpl->characters16();
		const UChar * currentSourceBufferPosition = sourceBuffer;
		char * currentTargetBuffer = buffer;
		char * bufferEnd = buffer + numBytes;

		// Taken from StringImpl::utf8Impl
		if (options & String::REPLACE_INVALID_UTF8)
		{
			const UChar * sourceEnd = sourceBuffer + thisImpl->length();
			
			while (currentSourceBufferPosition < sourceEnd)
			{
				result = WTF::Unicode::convertUTF16ToUTF8(&currentSourceBufferPosition, sourceEnd, &currentTargetBuffer, bufferEnd, true);				
				if (result != WTF::Unicode::conversionOK)
				{
					if (WTF::Unicode::targetExhausted == result)
					{
						break;
					}

					// Conversion fails when there is an unpaired surrogate.
					// Put replacement character (U+FFFD) instead of the unpaired surrogate.
					ASSERT((0xD800 <= *currentSourceBufferPosition && *currentSourceBufferPosition <= 0xDFFF));
					// There should be room left, since one UChar hasn't been converted.
					ASSERT((currentTargetBuffer + 3) <= bufferEnd);
					putUTF8Triple(currentTargetBuffer, WTF::Unicode::replacementCharacter);
					++currentSourceBufferPosition;

					// Override the result to make sure we'll null terminate the string after we finish the conversion
					result = WTF::Unicode::conversionOK;
				}
			}
		}
		else
		{
			result = WTF::Unicode::convertUTF16ToUTF8(&currentSourceBufferPosition, sourceBuffer + thisImpl->length(), &currentTargetBuffer, bufferEnd, false);
			
			// Check for an unconverted high surrogate.
			if (result == WTF::Unicode::sourceExhausted)
			{
				// This should be one unpaired high surrogate. Treat it the same
				// was as an unpaired high surrogate would have been handled in
				// the middle of a string with non-strict conversion - which is
				// to say, simply encode it to UTF-8.
				ASSERT_UNUSED(sourceBuffer, (currentSourceBufferPosition + 1) == (sourceBuffer + thisImpl->length()));
				ASSERT((*currentSourceBufferPosition >= 0xD800) && (*currentSourceBufferPosition <= 0xDBFF));
				// There should be room left, since one UChar hasn't been converted.
				ASSERT((currentTargetBuffer + 3) <= (bufferEnd));
				putUTF8Triple(currentTargetBuffer, *currentSourceBufferPosition);
			}
		}

		numChars = currentSourceBufferPosition - sourceBuffer;
		numBytes = currentTargetBuffer - buffer;
	}

	if (nullTermination)
	{
		// Deflation was aborted, so don't null terminate, regardless of what
		// the caller requested. Presumably this is because deflation only aborts
		// when the buffer runs out of space (although it's possible for the buffer
		// to still have space for null termination in that case, if the abort
		// happens on a character that deflates to multiple bytes, and the buffer
		// has at least one free byte, but not enough to hold the whole character).
		if (WTF::Unicode::conversionOK != result)
		{
			nullTermination = false;
		}
		// If the caller requested null termination, but the buffer doesn't have
		// any more space for it, then disable null termination.
		else if ((size_t)length == numBytes)
		{
			nullTermination = false;
		}
	}
	
	if (nullTermination)
	{
		buffer[numBytes++] = '\0';
	}

	if (nullptr != nchars_ref)
	{
		*nchars_ref = numChars;
	}

	return numBytes;
}

int String::WriteUtf8(char * buffer, int length, int * nchars_ref, int options) const
{
	return WriteUtf8(Isolate::GetCurrent(), buffer, length, nchars_ref, options);
}

bool String::IsExternal() const
{
	return GetResolvedStringImpl(this)->isExternal();
}

bool String::IsExternalOneByte() const
{
	const WTF::StringImpl * thisImpl = GetResolvedStringImpl(this);
	return thisImpl->is8Bit() && thisImpl->isExternal();
}

String::ExternalStringResourceBase* String::GetExternalStringResourceBase(Encoding * encoding_out) const
{
	ASSERT(encoding_out);

	const WTF::StringImpl * thisImpl = GetResolvedStringImpl(this);
	if (false == thisImpl->isExternal())
	{
		*encoding_out = String::UNKNOWN_ENCODING;
		return nullptr;
	}

	if (thisImpl->is8Bit())
	{
		*encoding_out = String::ONE_BYTE_ENCODING;
	}
	else
	{
		*encoding_out = String::TWO_BYTE_ENCODING;
	}

	return reinterpret_cast<String::ExternalStringResourceBase *>(g_externalStringsResources.inlineGet(reinterpret_cast<const ExternalStringImpl *>(thisImpl)));
}

String::ExternalStringResource * String::GetExternalStringResource() const
{
	const WTF::StringImpl * thisImpl = GetResolvedStringImpl(this);
	if ((thisImpl->is8Bit()) || (false == thisImpl->isExternal()))
	{
		return nullptr;
	}

	return reinterpret_cast<String::ExternalStringResource *>(g_externalStringsResources.inlineGet(reinterpret_cast<const ExternalStringImpl *>(thisImpl)));
}

const String::ExternalOneByteStringResource * String::GetExternalOneByteStringResource() const
{
	const WTF::StringImpl * thisImpl = GetResolvedStringImpl(this);
	if ((false == thisImpl->is8Bit()) || (false == thisImpl->isExternal()))
	{
		return nullptr;
	}

	return reinterpret_cast<String::ExternalOneByteStringResource *>(g_externalStringsResources.inlineGet(reinterpret_cast<const ExternalStringImpl *>(thisImpl)));
}

MaybeLocal<String> String::NewFromOneByte(Isolate * isolate, const uint8_t * data, v8::NewStringType type, int length)
{
	// length is optional
	if (length < 0)
	{
		length = strlen(reinterpret_cast<const char*>(data));
	}

	JSC::ExecState * exec = jscshim::GetExecStateForV8Isolate(isolate);
	
	JSC::JSString * newString = nullptr;
	if (v8::NewStringType::kInternalized == type) 
	{
		newString = JSC::JSString::create(exec->vm(), GetAtomicString(exec, reinterpret_cast<const LChar *>(data), length));
	}
	else
	{
		newString = JSC::jsString(exec, WTF::String(data, length));
	}
		
	return Local<String>::New(JSC::JSValue(newString));
}

MaybeLocal<String> String::NewFromTwoByte(Isolate* isolate, const uint16_t* data, v8::NewStringType type, int length)
{
	// length is optional
	if (length < 0)
	{
		length = wcslen(reinterpret_cast<const wchar_t *>(data));
	}

	JSC::ExecState * exec = jscshim::GetExecStateForV8Isolate(isolate);

	JSC::JSString * newString = nullptr;
	if (v8::NewStringType::kInternalized == type)
	{
		newString = JSC::JSString::create(exec->vm(), GetAtomicString(exec, reinterpret_cast<const UChar *>(data), length));
	}
	else
	{
		newString = JSC::jsString(exec, WTF::String(reinterpret_cast<const UChar *>(data), length));
	}

	return Local<String>::New(JSC::JSValue(newString));
}

Local<String> String::NewFromTwoByte(Isolate* isolate, const uint16_t* data, NewStringType type, int length)
{
	return NewFromTwoByte(isolate, data, static_cast<v8::NewStringType>(type), length)
		.FromMaybe(Local<String>());
}

MaybeLocal<String> String::NewFromUtf8(Isolate* isolate, const char* data, v8::NewStringType type, int length)
{
	JSC::ExecState * exec = jscshim::GetExecStateForV8Isolate(isolate);

	JSC::JSString * newString = nullptr;
	if (v8::NewStringType::kInternalized == type)
	{
		// TODO: Handle one char strings ourselves?
		WTF::AtomicString atomicUtf8Value = length < 0 ? WTF::AtomicString::fromUTF8(data) : WTF::AtomicString::fromUTF8(data, length);
		newString = JSC::JSString::create(exec->vm(), *atomicUtf8Value.impl());
	}
	else
	{
		newString = JSC::jsString(exec, length < 0 ? WTF::String::fromUTF8(data) : WTF::String::fromUTF8(data, length));
	}

	return Local<String>::New(JSC::JSValue(newString));
}

Local<String> String::NewFromUtf8(Isolate* isolate, const char* data, NewStringType type, int length)
{
	return NewFromUtf8(isolate, data, static_cast<v8::NewStringType>(type), length)
		.FromMaybe(Local<String>());
}

MaybeLocal<String> String::NewExternalOneByte(Isolate * isolate, ExternalOneByteStringResource * resource)
{
	return Local<String>::New(CreateExternalStringHelper<LChar, ExternalOneByteStringResource>(isolate, resource));
}

MaybeLocal<String> String::NewExternalTwoByte(Isolate * isolate, ExternalStringResource * resource)
{
	return Local<String>::New(CreateExternalStringHelper<UChar, ExternalStringResource>(isolate, resource));
}

Local<String> String::Concat(Local<String> left, Local<String> right)
{
	return Concat(Isolate::GetCurrent(), left, right);
}

Local<String> String::Concat(Isolate * isolate, Local<String> left, Local<String> right)
{
	JSC::ExecState * exec = jscshim::GetCurrentExecState();

	JSC::JSString * s1 = jscshim::GetJscCellFromV8<JSC::JSString>(*left);
	JSC::JSString * s2 = jscshim::GetJscCellFromV8<JSC::JSString>(*right);

	JSC::JSString * newString = JSC::jsString(exec, s1, s2);
	return Local<String>::New(JSC::JSValue(newString));
}

String::Utf8Value::Utf8Value(Isolate * isolate, Local<v8::Value> obj)
{
	Local<String> strVal = obj->ToString();
	if (strVal.IsEmpty())
	{
		str_ = nullptr;
		length_ = 0;
		return;
	}

	const WTF::String& jscStringVal = GetResolvedStringValue(*strVal);
	if (jscStringVal.isEmpty())
	{
		str_ = nullptr;
		length_ = 0;
		return;
	}

	length_ = jscStringVal.is8Bit() ? GetDeflatedUTF8StringLength(jscStringVal.characters8(), jscStringVal.length()) :
									  GetDeflatedUTF8StringLength(jscStringVal.characters16(), jscStringVal.length());

	str_ = new char[length_ + 1];
	strVal->WriteUtf8(isolate, str_, length_);
	str_[length_] = '\0';
}

String::Utf8Value::Utf8Value(Local<v8::Value> obj) : Utf8Value(Isolate::GetCurrent(), obj)
{
}

String::Utf8Value::~Utf8Value()
{
	if (nullptr != str_)
	{
		delete str_;
	}
}

String::Value::Value(Isolate * isolate, Local<v8::Value> obj)
{
	Local<String> strVal = obj->ToString();
	if (strVal.IsEmpty())
	{
		str_ = nullptr;
		length_ = 0;
		return;
	}

	const WTF::String& jscStringVal = GetResolvedStringValue(*strVal);
	if (jscStringVal.isEmpty())
	{
		str_ = nullptr;
		length_ = 0;
		return;
	}

	length_ = jscStringVal.length();

	str_ = new uint16_t[length_ + 1];
	strVal->Write(isolate, str_, 0, length_);
	str_[length_] = '\0';
}

String::Value::~Value()
{
	if (nullptr != str_)
	{
		delete str_;
	}
}

} // v8