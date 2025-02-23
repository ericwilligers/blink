/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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

#ifndef PositionOptions_h
#define PositionOptions_h

#include "platform/heap/Handle.h"
#include "wtf/RefCounted.h"

namespace WebCore {

class PositionOptions : public RefCountedWillBeGarbageCollected<PositionOptions> {
public:
    static PassRefPtrWillBeRawPtr<PositionOptions> create() { return adoptRefWillBeNoop(new PositionOptions()); }
    void trace(Visitor*) { }

    bool enableHighAccuracy() const { return m_highAccuracy; }
    void setEnableHighAccuracy(bool enable) { m_highAccuracy = enable; }
    bool hasTimeout() const { return m_hasTimeout; }
    unsigned timeout() const
    {
        ASSERT(hasTimeout());
        return m_timeout;
    }
    void setTimeout(unsigned timeout)
    {
        m_hasTimeout = true;
        m_timeout = timeout;
    }
    bool hasMaximumAge() const { return m_hasMaximumAge; }
    unsigned maximumAge() const
    {
        ASSERT(hasMaximumAge());
        return m_maximumAge;
    }
    void clearMaximumAge() { m_hasMaximumAge = false; }
    void setMaximumAge(unsigned age)
    {
        m_hasMaximumAge = true;
        m_maximumAge = age;
    }

private:
    PositionOptions()
        : m_highAccuracy(false)
        , m_hasTimeout(false)
    {
        setMaximumAge(0);
    }

    bool m_highAccuracy;
    bool m_hasTimeout;
    unsigned m_timeout;
    bool m_hasMaximumAge;
    unsigned m_maximumAge;
};

} // namespace WebCore

#endif // PositionOptions_h
