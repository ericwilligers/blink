// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "core/dom/ChildFrameDisconnector.h"

#include "core/dom/shadow/ElementShadow.h"
#include "core/dom/shadow/ShadowRoot.h"
#include "wtf/Assertions.h"

namespace WebCore {

#ifndef NDEBUG
static unsigned checkConnectedSubrameCountIsConsistent(Node&);
#endif

void ChildFrameDisconnector::disconnect(DisconnectPolicy policy)
{
#ifndef NDEBUG
    checkConnectedSubrameCountIsConsistent(m_root);
#endif

    if (!m_root.connectedSubframeCount())
        return;

    if (policy == RootAndDescendants) {
        collectFrameOwners(m_root);
    } else {
        for (Node* child = m_root.firstChild(); child; child = child->nextSibling())
            collectFrameOwners(*child);
    }

    disconnectCollectedFrameOwners();
}

void ChildFrameDisconnector::collectFrameOwners(Node& root)
{
    if (!root.connectedSubframeCount())
        return;

    if (root.isHTMLElement() && root.isFrameOwnerElement())
        m_frameOwners.append(&toHTMLFrameOwnerElement(root));

    for (Node* child = root.firstChild(); child; child = child->nextSibling())
        collectFrameOwners(*child);

    ElementShadow* shadow = root.isElementNode() ? toElement(root).shadow() : 0;
    if (shadow)
        collectFrameOwners(*shadow);
}

void ChildFrameDisconnector::disconnectCollectedFrameOwners()
{
    // Must disable frame loading in the subtree so an unload handler cannot
    // insert more frames and create loaded frames in detached subtrees.
    SubframeLoadingDisabler disabler(m_root);

    for (unsigned i = 0; i < m_frameOwners.size(); ++i) {
        HTMLFrameOwnerElement* owner = m_frameOwners[i].get();
        // Don't need to traverse up the tree for the first owner since no
        // script could have moved it.
        if (!i || m_root.containsIncludingShadowDOM(owner))
            owner->disconnectContentFrame();
    }
}

void ChildFrameDisconnector::collectFrameOwners(ElementShadow& shadow)
{
    for (ShadowRoot* root = shadow.youngestShadowRoot(); root; root = root->olderShadowRoot())
        collectFrameOwners(*root);
}

#ifndef NDEBUG
static unsigned checkConnectedSubrameCountIsConsistent(Node& node)
{
    unsigned count = 0;

    if (node.isElementNode()) {
        if (node.isFrameOwnerElement() && toHTMLFrameOwnerElement(node).contentFrame())
            count++;

        if (ElementShadow* shadow = toElement(node).shadow()) {
            for (ShadowRoot* root = shadow->youngestShadowRoot(); root; root = root->olderShadowRoot())
                count += checkConnectedSubrameCountIsConsistent(*root);
        }
    }

    for (Node* child = node.firstChild(); child; child = child->nextSibling())
        count += checkConnectedSubrameCountIsConsistent(*child);

    // If we undercount there's possibly a security bug since we'd leave frames
    // in subtrees outside the document.
    ASSERT(node.connectedSubframeCount() >= count);

    // If we overcount it's safe, but not optimal because it means we'll traverse
    // through the document in ChildFrameDisconnector looking for frames that have
    // already been disconnected.
    ASSERT(node.connectedSubframeCount() == count);

    return count;
}
#endif

}
