/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "public/web/WebDOMError.h"

#include "V8DOMError.h"
#include "bindings/v8/V8Binding.h"
#include "core/dom/DOMError.h"
#include "wtf/PassOwnPtr.h"

using namespace WebCore;

namespace blink {

WebDOMError WebDOMError::create(const WebString& name, const WebString& message)
{
    return WebDOMError(DOMError::create(name, message));
}

void WebDOMError::reset()
{
    m_private.reset();
}

void WebDOMError::assign(const WebDOMError& other)
{
    m_private = other.m_private;
}

WebString WebDOMError::name() const
{
    if (!m_private.get())
        return WebString();
    return m_private->name();
}

WebString WebDOMError::message() const
{
    if (!m_private.get())
        return WebString();
    return m_private->message();
}

v8::Handle<v8::Value>  WebDOMError::toV8Value()
{
    if (!m_private.get())
        return v8::Handle<v8::Value>();
    return toV8(m_private.get(), v8::Handle<v8::Object>(), v8::Isolate::GetCurrent());
}

v8::Handle<v8::Value>  WebDOMError::toV8Value(v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    if (!m_private.get())
        return v8::Handle<v8::Value>();
    return toV8(m_private.get(), creationContext, isolate);
}

WebDOMError::WebDOMError(const PassRefPtrWillBeRawPtr<WebCore::DOMError>& error)
    : m_private(error)
{
}

WebDOMError& WebDOMError::operator=(const PassRefPtrWillBeRawPtr<WebCore::DOMError>& error)
{
    m_private = error;
    return *this;
}

} // namespace blink
