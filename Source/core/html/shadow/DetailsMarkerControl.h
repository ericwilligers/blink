/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef DetailsMarkerControl_h
#define DetailsMarkerControl_h

#include "core/html/HTMLDivElement.h"
#include "wtf/Forward.h"

namespace WebCore {

class HTMLSummaryElement;

class DetailsMarkerControl FINAL : public HTMLDivElement {
public:
    explicit DetailsMarkerControl(Document&);
    static PassRefPtrWillBeRawPtr<DetailsMarkerControl> create(Document&);

private:
    virtual RenderObject* createRenderer(RenderStyle*) OVERRIDE;
    virtual bool rendererIsNeeded(const RenderStyle&) OVERRIDE;

    HTMLSummaryElement* summaryElement();
};

inline PassRefPtrWillBeRawPtr<DetailsMarkerControl> DetailsMarkerControl::create(Document& document)
{
    RefPtrWillBeRawPtr<DetailsMarkerControl> element = adoptRefWillBeRefCountedGarbageCollected(new DetailsMarkerControl(document));
    element->setShadowPseudoId(AtomicString("-webkit-details-marker", AtomicString::ConstructFromLiteral));
    return element.release();
}

}

#endif
