/*
 * Copyright (C) 2006, 2008, 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PluginDocument_h
#define PluginDocument_h

#include "core/html/HTMLDocument.h"

namespace WebCore {

class Node;
class Widget;

class PluginDocument FINAL : public HTMLDocument {
public:
    static PassRefPtrWillBeRawPtr<PluginDocument> create(const DocumentInit& initializer = DocumentInit())
    {
        return adoptRefWillBeRefCountedGarbageCollected(new PluginDocument(initializer));
    }

    void setPluginNode(Node* pluginNode) { m_pluginNode = pluginNode; }

    Widget* pluginWidget();
    Node* pluginNode();

    virtual void detach(const AttachContext& = AttachContext()) OVERRIDE;

    bool shouldLoadPluginManually() { return m_shouldLoadPluginManually; }

    virtual void trace(Visitor*) OVERRIDE;

private:
    explicit PluginDocument(const DocumentInit&);

    virtual PassRefPtrWillBeRawPtr<DocumentParser> createParser() OVERRIDE;

    void setShouldLoadPluginManually(bool loadManually) { m_shouldLoadPluginManually = loadManually; }

    bool m_shouldLoadPluginManually;
    RefPtrWillBeMember<Node> m_pluginNode;
};

DEFINE_DOCUMENT_TYPE_CASTS(PluginDocument);

}

#endif // PluginDocument_h
