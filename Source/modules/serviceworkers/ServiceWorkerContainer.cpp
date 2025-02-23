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
#include "modules/serviceworkers/ServiceWorkerContainer.h"

#include "RuntimeEnabledFeatures.h"
#include "bindings/v8/CallbackPromiseAdapter.h"
#include "bindings/v8/ScriptPromiseResolverWithContext.h"
#include "bindings/v8/ScriptState.h"
#include "bindings/v8/SerializedScriptValue.h"
#include "core/dom/DOMException.h"
#include "core/dom/Document.h"
#include "core/dom/ExceptionCode.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/MessagePort.h"
#include "core/events/MessageEvent.h"
#include "modules/serviceworkers/RegistrationOptionList.h"
#include "modules/serviceworkers/ServiceWorker.h"
#include "modules/serviceworkers/ServiceWorkerContainerClient.h"
#include "modules/serviceworkers/ServiceWorkerError.h"
#include "public/platform/WebServiceWorker.h"
#include "public/platform/WebServiceWorkerProvider.h"
#include "public/platform/WebString.h"
#include "public/platform/WebURL.h"
#include "v8.h"

using blink::WebServiceWorker;
using blink::WebServiceWorkerProvider;

namespace WebCore {

PassRefPtr<ServiceWorkerContainer> ServiceWorkerContainer::create(ExecutionContext* executionContext)
{
    return adoptRef(new ServiceWorkerContainer(executionContext));
}

ServiceWorkerContainer::~ServiceWorkerContainer()
{
}

void ServiceWorkerContainer::detachClient()
{
    if (m_provider) {
        m_provider->setClient(0);
        m_provider = 0;
    }
}

ScriptPromise ServiceWorkerContainer::registerServiceWorker(ExecutionContext* executionContext, const String& url, const Dictionary& dictionary)
{
    RegistrationOptionList options(dictionary);
    ASSERT(RuntimeEnabledFeatures::serviceWorkerEnabled());
    RefPtr<ScriptPromiseResolverWithContext> resolver = ScriptPromiseResolverWithContext::create(ScriptState::current(toIsolate(executionContext)));
    ScriptPromise promise = resolver->promise();

    if (!m_provider) {
        resolver->reject(DOMException::create(InvalidStateError, "No associated provider is available"));
        return promise;
    }

    RefPtr<SecurityOrigin> documentOrigin = executionContext->securityOrigin();
    KURL patternURL = executionContext->completeURL(options.scope);
    patternURL.removeFragmentIdentifier();
    if (!documentOrigin->canRequest(patternURL)) {
        resolver->reject(DOMException::create(SecurityError, "Can only register for patterns in the document's origin."));
        return promise;
    }

    KURL scriptURL = executionContext->completeURL(url);
    scriptURL.removeFragmentIdentifier();
    if (!documentOrigin->canRequest(scriptURL)) {
        resolver->reject(DOMException::create(SecurityError, "Script must be in document's origin."));
        return promise;
    }

    m_provider->registerServiceWorker(patternURL, scriptURL, new CallbackPromiseAdapter<ServiceWorker, ServiceWorkerError>(resolver));
    return promise;
}

class UndefinedValue {
public:
    typedef WebServiceWorker WebType;
    static V8UndefinedType from(ScriptPromiseResolverWithContext* resolver, WebServiceWorker* worker)
    {
        ASSERT(!worker); // Anything passed here will be leaked.
        return V8UndefinedType();
    }

private:
    UndefinedValue();
};

ScriptPromise ServiceWorkerContainer::unregisterServiceWorker(ExecutionContext* executionContext, const String& pattern)
{
    ASSERT(RuntimeEnabledFeatures::serviceWorkerEnabled());
    RefPtr<ScriptPromiseResolverWithContext> resolver = ScriptPromiseResolverWithContext::create(ScriptState::current(toIsolate(executionContext)));
    ScriptPromise promise = resolver->promise();

    if (!m_provider) {
        resolver->reject(DOMException::create(InvalidStateError, "No associated provider is available"));
        return promise;
    }

    RefPtr<SecurityOrigin> documentOrigin = executionContext->securityOrigin();
    KURL patternURL = executionContext->completeURL(pattern);
    patternURL.removeFragmentIdentifier();
    if (!pattern.isEmpty() && !documentOrigin->canRequest(patternURL)) {
        resolver->reject(DOMException::create(SecurityError, "Can only unregister for patterns in the document's origin."));
        return promise;
    }

    m_provider->unregisterServiceWorker(patternURL, new CallbackPromiseAdapter<UndefinedValue, ServiceWorkerError>(resolver));
    return promise;
}

void ServiceWorkerContainer::setCurrentServiceWorker(blink::WebServiceWorker* serviceWorker)
{
    if (!executionContext()) {
        delete serviceWorker;
        return;
    }
    m_current = ServiceWorker::create(executionContext(), adoptPtr(serviceWorker));
}

void ServiceWorkerContainer::dispatchMessageEvent(const blink::WebString& message, const blink::WebMessagePortChannelArray& webChannels)
{
    if (!executionContext() || !window())
        return;

    OwnPtr<MessagePortArray> ports = MessagePort::toMessagePortArray(executionContext(), webChannels);
    RefPtr<SerializedScriptValue> value = SerializedScriptValue::createFromWire(message);
    window()->dispatchEvent(MessageEvent::create(ports.release(), value));
}

ServiceWorkerContainer::ServiceWorkerContainer(ExecutionContext* executionContext)
    : ContextLifecycleObserver(executionContext)
    , DOMWindowLifecycleObserver(executionContext->isDocument() ? toDocument(executionContext)->domWindow() : 0)
    , m_provider(0)
{
    ScriptWrappable::init(this);

    if (!executionContext)
        return;

    if (ServiceWorkerContainerClient* client = ServiceWorkerContainerClient::from(executionContext)) {
        m_provider = client->provider();
        if (m_provider)
            m_provider->setClient(this);
    }
}

} // namespace WebCore
