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

#ifndef HTMLImportChild_h
#define HTMLImportChild_h

#include "core/html/imports/HTMLImport.h"
#include "platform/heap/Handle.h"
#include "platform/weborigin/KURL.h"
#include "wtf/Vector.h"
#include "wtf/WeakPtr.h"

namespace WebCore {

class CustomElementMicrotaskImportStep;
class HTMLImportLoader;
class HTMLImportChildClient;
class HTMLLinkElement;

//
// An import tree node subclas to encapsulate imported document
// lifecycle. This class is owned by HTMLImportsController. The actual loading
// is done by HTMLImportLoader, which can be shared among multiple
// HTMLImportChild of same link URL.
//
class HTMLImportChild FINAL : public HTMLImport {
public:
    HTMLImportChild(const KURL&, HTMLImportLoader*, SyncMode);
    virtual ~HTMLImportChild();

    HTMLLinkElement* link() const;
    Document* importedDocument() const;
    const KURL& url() const { return m_url; }

    void didShareLoader();
    void didStartLoading();
    void importDestroyed();
    WeakPtr<HTMLImportChild> weakPtr() { return m_weakFactory.createWeakPtr(); }

    // HTMLImport
    virtual bool isChild() const OVERRIDE { return true; }
    virtual Document* document() const OVERRIDE;
    virtual bool isDone() const OVERRIDE;
    virtual HTMLImportLoader* loader() const OVERRIDE { return m_loader; }
    virtual void stateWillChange() OVERRIDE;
    virtual void stateDidChange() OVERRIDE;

#if !defined(NDEBUG)
    virtual void showThis() OVERRIDE;
#endif

    void setClient(HTMLImportChildClient*);
    void clearClient();
    bool loaderHasError() const;

    void didFinishLoading();
    void didFinishUpgradingCustomElements();
    bool isLoaded() const;
    bool isFirst() const;
    void normalize();

private:
    void didFinish();
    void shareLoader();
    void createCustomElementMicrotaskStepIfNeeded();

    KURL m_url;
    WeakPtrFactory<HTMLImportChild> m_weakFactory;
    WeakPtr<CustomElementMicrotaskImportStep> m_customElementMicrotaskStep;
    HTMLImportLoader* m_loader;
    HTMLImportChildClient* m_client;
};

inline HTMLImportChild* toHTMLImportChild(HTMLImport* import)
{
    ASSERT(!import || import->isChild());
    return static_cast<HTMLImportChild*>(import);
}

} // namespace WebCore

#endif // HTMLImportChild_h
