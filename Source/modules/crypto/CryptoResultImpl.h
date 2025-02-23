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

#ifndef CryptoResultImpl_h
#define CryptoResultImpl_h

#include "bindings/v8/ScriptPromise.h"
#include "platform/CryptoResult.h"
#include "public/platform/WebCrypto.h"
#include "wtf/Forward.h"
#include "wtf/WeakPtr.h"

namespace WebCore {

// Wrapper around a Promise to notify completion of the crypto operation.
//
// The thread on which CryptoResultImpl was created on is referred to as the
// "origin thread".
//
//  * At creation time there must be an active ExecutionContext.
//  * The CryptoResult interface must only be called from the origin thread.
//  * addref() and deref() can be called from any thread.
//  * One of the completeWith***() functions must be called, or the
//    PromiseState will be leaked until the ExecutionContext is destroyed.
class CryptoResultImpl FINAL : public CryptoResult {
public:
    ~CryptoResultImpl();

    static PassRefPtr<CryptoResultImpl> create(ScriptState*);

    virtual void completeWithError(blink::WebCryptoErrorType, const blink::WebString&) OVERRIDE;
    virtual void completeWithBuffer(const blink::WebArrayBuffer&) OVERRIDE;
    virtual void completeWithBoolean(bool) OVERRIDE;
    virtual void completeWithKey(const blink::WebCryptoKey&) OVERRIDE;
    virtual void completeWithKeyPair(const blink::WebCryptoKey& publicKey, const blink::WebCryptoKey& privateKey) OVERRIDE;

    // It is only valid to call this before completion.
    ScriptPromise promise();

private:
    explicit CryptoResultImpl(ScriptState*);

    class PromiseState;
    WeakPtr<PromiseState> m_promiseState;
};

} // namespace WebCore

#endif
