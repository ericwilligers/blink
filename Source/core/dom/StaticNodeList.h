/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef StaticNodeList_h
#define StaticNodeList_h

#include "core/dom/Node.h"
#include "core/dom/NodeList.h"
#include "wtf/PassRefPtr.h"
#include "wtf/RefPtr.h"
#include "wtf/Vector.h"

namespace WebCore {

class Node;

class StaticNodeList FINAL : public NodeList {
public:
    static PassRefPtrWillBeRawPtr<StaticNodeList> adopt(Vector<RefPtr<Node> >& nodes);

    static PassRefPtrWillBeRawPtr<StaticNodeList> createEmpty()
    {
        return adoptRefWillBeNoop(new StaticNodeList);
    }

    virtual ~StaticNodeList();

    virtual unsigned length() const OVERRIDE;
    virtual Node* item(unsigned index) const OVERRIDE;

    virtual void trace(Visitor*) OVERRIDE;

private:
    // If AllocationSize() is larger than this, we report it as external
    // allocated memory to V8.
    const static ptrdiff_t externalMemoryReportSizeLimit = 1024;

    ptrdiff_t AllocationSize()
    {
        return m_nodes.capacity() * sizeof(RefPtrWillBeMember<Node>);
    }

    WillBeHeapVector<RefPtrWillBeMember<Node> > m_nodes;
};

} // namespace WebCore

#endif // StaticNodeList_h
