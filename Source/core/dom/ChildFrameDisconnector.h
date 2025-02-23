// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ChildFrameDisconnector_h
#define ChildFrameDisconnector_h

#include "core/dom/Node.h"
#include "core/html/HTMLFrameOwnerElement.h"
#include "platform/heap/Handle.h"

namespace WebCore {

class ChildFrameDisconnector {
    STACK_ALLOCATED();
public:
    enum DisconnectPolicy {
        RootAndDescendants,
        DescendantsOnly
    };

    explicit ChildFrameDisconnector(Node& root)
        : m_root(root)
    {
    }

    void disconnect(DisconnectPolicy = RootAndDescendants);

private:
    void collectFrameOwners(Node&);
    void collectFrameOwners(ElementShadow&);
    void disconnectCollectedFrameOwners();

    WillBeHeapVector<RefPtrWillBeMember<HTMLFrameOwnerElement>, 10> m_frameOwners;
    Node& m_root;
};

} // namespace WebCore

#endif // ChildFrameDisconnector_h
