/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HTMLViewSourceParser_h
#define HTMLViewSourceParser_h

#include "core/dom/DecodedDataDocumentParser.h"
#include "core/html/HTMLViewSourceDocument.h"
#include "core/html/parser/HTMLInputStream.h"
#include "core/html/parser/HTMLSourceTracker.h"
#include "core/html/parser/HTMLTokenizer.h"

namespace WebCore {

class HTMLViewSourceParser FINAL :  public DecodedDataDocumentParser {
public:
    static PassRefPtrWillBeRawPtr<HTMLViewSourceParser> create(HTMLViewSourceDocument& document, const String& mimeType)
    {
        return adoptRefWillBeNoop(new HTMLViewSourceParser(document, mimeType));
    }
    virtual ~HTMLViewSourceParser() { }

private:
    HTMLViewSourceParser(HTMLViewSourceDocument&, const String& mimeType);

    // DocumentParser
    virtual void insert(const SegmentedString&) OVERRIDE { ASSERT_NOT_REACHED(); }
    virtual void append(PassRefPtr<StringImpl>) OVERRIDE;
    virtual void finish() OVERRIDE;

    HTMLViewSourceDocument* document() const { return static_cast<HTMLViewSourceDocument*>(DecodedDataDocumentParser::document()); }

    void pumpTokenizer();
    void updateTokenizerState();

    HTMLInputStream m_input;
    HTMLToken m_token;
    HTMLSourceTracker m_sourceTracker;
    OwnPtr<HTMLTokenizer> m_tokenizer;
};

}

#endif
