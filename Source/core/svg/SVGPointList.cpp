/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
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
 */

#include "config.h"
#include "core/svg/SVGPointList.h"

#include "core/svg/SVGAnimationElement.h"
#include "core/svg/SVGParserUtilities.h"
#include "platform/geometry/FloatPoint.h"
#include "wtf/text/StringBuilder.h"
#include "wtf/text/WTFString.h"

namespace WebCore {

inline PassRefPtr<SVGPointList> toSVGPointList(PassRefPtr<SVGPropertyBase> passBase)
{
    RefPtr<SVGPropertyBase> base = passBase;
    ASSERT(base->type() == SVGPointList::classType());
    return static_pointer_cast<SVGPointList>(base.release());
}

SVGPointList::SVGPointList()
{
}

SVGPointList::~SVGPointList()
{
}

PassRefPtr<SVGPointList> SVGPointList::clone()
{
    RefPtr<SVGPointList> svgPointList = SVGPointList::create();
    svgPointList->deepCopy(this);
    return svgPointList.release();
}

PassRefPtr<SVGPropertyBase> SVGPointList::cloneForAnimation(const String& value) const
{
    RefPtr<SVGPointList> svgPointList = SVGPointList::create();
    svgPointList->setValueAsString(value, IGNORE_EXCEPTION);
    return svgPointList.release();
}

String SVGPointList::valueAsString() const
{
    StringBuilder builder;

    ConstIterator it = begin();
    ConstIterator itEnd = end();
    if (it != itEnd) {
        builder.append(it->valueAsString());
        ++it;

        for (; it != itEnd; ++it) {
            builder.append(' ');
            builder.append(it->valueAsString());
        }
    }

    return builder.toString();
}

template <typename CharType>
bool SVGPointList::parse(const CharType*& ptr, const CharType* end)
{
    clear();

    skipOptionalSVGSpaces(ptr, end);
    if (ptr >= end)
        return true;

    for (;;) {
        float x = 0.0f;
        float y = 0.0f;
        bool valid = parseNumber(ptr, end, x) && parseNumber(ptr, end, y, false);
        if (!valid) {
            return false;
        }
        append(SVGPoint::create(FloatPoint(x, y)));

        skipOptionalSVGSpaces(ptr, end);
        if (ptr < end && *ptr == ',') {
            ++ptr;
            skipOptionalSVGSpaces(ptr, end);

            // ',' requires the list to be continued
            continue;
        }

        // check end of list
        if (ptr >= end)
            return true;
    }
}

void SVGPointList::setValueAsString(const String& value, ExceptionState& exceptionState)
{
    if (value.isEmpty()) {
        clear();
        return;
    }

    bool valid = false;
    if (value.is8Bit()) {
        const LChar* ptr = value.characters8();
        const LChar* end = ptr + value.length();
        valid = parse(ptr, end);
    } else {
        const UChar* ptr = value.characters16();
        const UChar* end = ptr + value.length();
        valid = parse(ptr, end);
    }

    if (!valid)
        exceptionState.throwDOMException(SyntaxError, "Problem parsing points=\""+value+"\"");
}

void SVGPointList::add(PassRefPtrWillBeRawPtr<SVGPropertyBase> other, SVGElement* contextElement)
{
    RefPtr<SVGPointList> otherList = toSVGPointList(other);

    if (length() != otherList->length())
        return;

    for (size_t i = 0; i < length(); ++i)
        at(i)->setValue(at(i)->value() + otherList->at(i)->value());
}

bool SVGPointList::adjustFromToListValues(PassRefPtr<SVGPointList> passFromList, PassRefPtr<SVGPointList> passToList, float percentage, bool isToAnimation, bool resizeAnimatedListIfNeeded)
{
    RefPtr<SVGPointList> fromList = passFromList;
    RefPtr<SVGPointList> toList = passToList;

    // If no 'to' value is given, nothing to animate.
    size_t toListSize = toList->length();
    if (!toListSize)
        return false;

    // If the 'from' value is given and it's length doesn't match the 'to' value list length, fallback to a discrete animation.
    size_t fromListSize = fromList->length();
    if (fromListSize != toListSize && fromListSize) {
        if (percentage < 0.5) {
            if (!isToAnimation)
                deepCopy(fromList);
        } else {
            deepCopy(toList);
        }

        return false;
    }

    ASSERT(!fromListSize || fromListSize == toListSize);
    if (resizeAnimatedListIfNeeded && length() < toListSize) {
        size_t paddingCount = toListSize - length();
        for (size_t i = 0; i < paddingCount; ++i)
            append(SVGPoint::create());
    }

    return true;
}

void SVGPointList::calculateAnimatedValue(SVGAnimationElement* animationElement, float percentage, unsigned repeatCount, PassRefPtr<SVGPropertyBase> fromValue, PassRefPtr<SVGPropertyBase> toValue, PassRefPtr<SVGPropertyBase> toAtEndOfDurationValue, SVGElement* contextElement)
{
    RefPtr<SVGPointList> fromList = toSVGPointList(fromValue);
    RefPtr<SVGPointList> toList = toSVGPointList(toValue);
    RefPtr<SVGPointList> toAtEndOfDurationList = toSVGPointList(toAtEndOfDurationValue);

    size_t fromPointListSize = fromList->length();
    size_t toPointListSize = toList->length();
    size_t toAtEndOfDurationListSize = toAtEndOfDurationList->length();

    if (!adjustFromToListValues(fromList, toList, percentage, animationElement->animationMode() == ToAnimation, true))
        return;

    for (size_t i = 0; i < toPointListSize; ++i) {
        float animatedX = at(i)->x();
        float animatedY = at(i)->y();

        FloatPoint effectiveFrom;
        if (fromPointListSize)
            effectiveFrom = fromList->at(i)->value();
        FloatPoint effectiveTo = toList->at(i)->value();
        FloatPoint effectiveToAtEnd;
        if (i < toAtEndOfDurationListSize)
            effectiveToAtEnd = toAtEndOfDurationList->at(i)->value();

        animationElement->animateAdditiveNumber(percentage, repeatCount, effectiveFrom.x(), effectiveTo.x(), effectiveToAtEnd.x(), animatedX);
        animationElement->animateAdditiveNumber(percentage, repeatCount, effectiveFrom.y(), effectiveTo.y(), effectiveToAtEnd.y(), animatedY);
        at(i)->setValue(FloatPoint(animatedX, animatedY));
    }
}

float SVGPointList::calculateDistance(PassRefPtr<SVGPropertyBase> to, SVGElement*)
{
    // FIXME: Distance calculation is not possible for SVGPointList right now. We need the distance for every single value.
    return -1;
}

}
