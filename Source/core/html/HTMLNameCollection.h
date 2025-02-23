/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef HTMLNameCollection_h
#define HTMLNameCollection_h

#include "core/html/HTMLCollection.h"

#include "wtf/text/AtomicString.h"

namespace WebCore {

class Document;

class HTMLNameCollection FINAL : public HTMLCollection {
public:
    static PassRefPtrWillBeRawPtr<HTMLNameCollection> create(ContainerNode& document, CollectionType type, const AtomicString& name)
    {
        return adoptRefWillBeNoop(new HTMLNameCollection(document, type, name));
    }

    ~HTMLNameCollection();

private:
    HTMLNameCollection(ContainerNode&, CollectionType, const AtomicString& name);

    virtual Element* virtualItemAfter(Element*) const OVERRIDE;

    AtomicString m_name;
};

}

#endif
