// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef Response_h
#define Response_h

#include "bindings/v8/Dictionary.h"
#include "bindings/v8/ScriptWrappable.h"
#include "modules/serviceworkers/HeaderMap.h"
#include "wtf/RefCounted.h"
#include "wtf/RefPtr.h"
#include "wtf/text/WTFString.h"

namespace blink { class WebServiceWorkerResponse; }

namespace WebCore {

struct ResponseInit;

class Response FINAL : public ScriptWrappable, public RefCounted<Response> {
public:
    static PassRefPtr<Response> create();
    static PassRefPtr<Response> create(const Dictionary& responseInit);
    ~Response() { };

    unsigned short status() const { return m_status; }
    String statusText() const { return m_statusText; }
    PassRefPtr<HeaderMap> headers() const;

    void populateWebServiceWorkerResponse(blink::WebServiceWorkerResponse&);

private:
    explicit Response(const ResponseInit&);
    unsigned short m_status;
    String m_statusText;
    RefPtr<HeaderMap> m_headers;
};

} // namespace WebCore

#endif // Response_h
