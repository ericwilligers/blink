/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 *
 * Contains diff method based on Javascript Diff Algorithm By John Resig
 * http://ejohn.org/files/jsdiff.js (released under the MIT license).
 */

/**
 * @param {string=} direction
 */
Node.prototype.rangeOfWord = function(offset, stopCharacters, stayWithinNode, direction)
{
    var startNode;
    var startOffset = 0;
    var endNode;
    var endOffset = 0;

    if (!stayWithinNode)
        stayWithinNode = this;

    if (!direction || direction === "backward" || direction === "both") {
        var node = this;
        while (node) {
            if (node === stayWithinNode) {
                if (!startNode)
                    startNode = stayWithinNode;
                break;
            }

            if (node.nodeType === Node.TEXT_NODE) {
                var start = (node === this ? (offset - 1) : (node.nodeValue.length - 1));
                for (var i = start; i >= 0; --i) {
                    if (stopCharacters.indexOf(node.nodeValue[i]) !== -1) {
                        startNode = node;
                        startOffset = i + 1;
                        break;
                    }
                }
            }

            if (startNode)
                break;

            node = node.traversePreviousNode(stayWithinNode);
        }

        if (!startNode) {
            startNode = stayWithinNode;
            startOffset = 0;
        }
    } else {
        startNode = this;
        startOffset = offset;
    }

    if (!direction || direction === "forward" || direction === "both") {
        node = this;
        while (node) {
            if (node === stayWithinNode) {
                if (!endNode)
                    endNode = stayWithinNode;
                break;
            }

            if (node.nodeType === Node.TEXT_NODE) {
                var start = (node === this ? offset : 0);
                for (var i = start; i < node.nodeValue.length; ++i) {
                    if (stopCharacters.indexOf(node.nodeValue[i]) !== -1) {
                        endNode = node;
                        endOffset = i;
                        break;
                    }
                }
            }

            if (endNode)
                break;

            node = node.traverseNextNode(stayWithinNode);
        }

        if (!endNode) {
            endNode = stayWithinNode;
            endOffset = stayWithinNode.nodeType === Node.TEXT_NODE ? stayWithinNode.nodeValue.length : stayWithinNode.childNodes.length;
        }
    } else {
        endNode = this;
        endOffset = offset;
    }

    var result = this.ownerDocument.createRange();
    result.setStart(startNode, startOffset);
    result.setEnd(endNode, endOffset);

    return result;
}

Node.prototype.traverseNextTextNode = function(stayWithin)
{
    var node = this.traverseNextNode(stayWithin);
    if (!node)
        return;

    while (node && node.nodeType !== Node.TEXT_NODE)
        node = node.traverseNextNode(stayWithin);

    return node;
}

Node.prototype.rangeBoundaryForOffset = function(offset)
{
    var node = this.traverseNextTextNode(this);
    while (node && offset > node.nodeValue.length) {
        offset -= node.nodeValue.length;
        node = node.traverseNextTextNode(this);
    }
    if (!node)
        return { container: this, offset: 0 };
    return { container: node, offset: offset };
}

Element.prototype.removeMatchingStyleClasses = function(classNameRegex)
{
    var regex = new RegExp("(^|\\s+)" + classNameRegex + "($|\\s+)");
    if (regex.test(this.className))
        this.className = this.className.replace(regex, " ");
}

/**
 * @param {number|undefined} x
 * @param {number|undefined} y
 * @param {!Element=} relativeTo
 */
Element.prototype.positionAt = function(x, y, relativeTo)
{
    var shift = {x: 0, y: 0};
    if (relativeTo)
       shift = relativeTo.boxInWindow(this.ownerDocument.defaultView);

    if (typeof x === "number")
        this.style.setProperty("left", (shift.x + x) + "px");
    else
        this.style.removeProperty("left");

    if (typeof y === "number")
        this.style.setProperty("top", (shift.y + y) + "px");
    else
        this.style.removeProperty("top");
}

Element.prototype.isScrolledToBottom = function()
{
    // This code works only for 0-width border.
    // Both clientHeight and scrollHeight are rounded to integer values, so we tolerate
    // one pixel error.
    return Math.abs(this.scrollTop + this.clientHeight - this.scrollHeight) <= 1;
}

/**
 * @param {!Node} fromNode
 * @param {!Node} toNode
 */
function removeSubsequentNodes(fromNode, toNode)
{
    for (var node = fromNode; node && node !== toNode; ) {
        var nodeToRemove = node;
        node = node.nextSibling;
        nodeToRemove.remove();
    }
}

/**
 * @constructor
 * @param {!Size} minimum
 * @param {?Size=} preferred
 */
function Constraints(minimum, preferred)
{
    /**
     * @type {!Size}
     */
    this.minimum = minimum;

    /**
     * @type {!Size}
     */
    this.preferred = preferred || minimum;

    if (this.minimum.width > this.preferred.width || this.minimum.height > this.preferred.height)
        throw new Error("Minimum size is greater than preferred.");
}

/**
 * @param {?Constraints} constraints
 * @return {boolean}
 */
Constraints.prototype.isEqual = function(constraints)
{
    return !!constraints && this.minimum.isEqual(constraints.minimum) && this.preferred.isEqual(constraints.preferred);
};

/**
 * @param {!Constraints|number} value
 * @return {!Constraints}
 */
Constraints.prototype.widthToMax = function(value)
{
    if (typeof value === "number")
        return new Constraints(this.minimum.widthToMax(value), this.preferred.widthToMax(value));
    return new Constraints(this.minimum.widthToMax(value.minimum), this.preferred.widthToMax(value.preferred));
};

/**
 * @param {!Constraints|number} value
 * @return {!Constraints}
 */
Constraints.prototype.addWidth = function(value)
{
    if (typeof value === "number")
        return new Constraints(this.minimum.addWidth(value), this.preferred.addWidth(value));
    return new Constraints(this.minimum.addWidth(value.minimum), this.preferred.addWidth(value.preferred));
};

/**
 * @param {!Constraints|number} value
 * @return {!Constraints}
 */
Constraints.prototype.heightToMax = function(value)
{
    if (typeof value === "number")
        return new Constraints(this.minimum.heightToMax(value), this.preferred.heightToMax(value));
    return new Constraints(this.minimum.heightToMax(value.minimum), this.preferred.heightToMax(value.preferred));
};

/**
 * @param {!Constraints|number} value
 * @return {!Constraints}
 */
Constraints.prototype.addHeight = function(value)
{
    if (typeof value === "number")
        return new Constraints(this.minimum.addHeight(value), this.preferred.addHeight(value));
    return new Constraints(this.minimum.addHeight(value.minimum), this.preferred.addHeight(value.preferred));
};

/**
 * @param {?Element=} containerElement
 * @return {!Size}
 */
Element.prototype.measurePreferredSize = function(containerElement)
{
    containerElement = containerElement || document.body;
    containerElement.appendChild(this);
    this.positionAt(0, 0);
    var result = new Size(this.offsetWidth, this.offsetHeight);
    this.positionAt(undefined, undefined);
    this.remove();
    return result;
}

/**
 * @param {!Event} event
 * @return {boolean}
 */
Element.prototype.containsEventPoint = function(event)
{
    var box = this.getBoundingClientRect();
    return box.left < event.x  && event.x < box.right &&
           box.top < event.y && event.y < box.bottom;
}

Node.prototype.enclosingNodeOrSelfWithNodeNameInArray = function(nameArray)
{
    for (var node = this; node && node !== this.ownerDocument; node = node.parentNode)
        for (var i = 0; i < nameArray.length; ++i)
            if (node.nodeName.toLowerCase() === nameArray[i].toLowerCase())
                return node;
    return null;
}

Node.prototype.enclosingNodeOrSelfWithNodeName = function(nodeName)
{
    return this.enclosingNodeOrSelfWithNodeNameInArray([nodeName]);
}

/**
 * @param {string} className
 * @param {!Element=} stayWithin
 */
Node.prototype.enclosingNodeOrSelfWithClass = function(className, stayWithin)
{
    for (var node = this; node && node !== stayWithin && node !== this.ownerDocument; node = node.parentNode)
        if (node.nodeType === Node.ELEMENT_NODE && node.classList.contains(className))
            return node;
    return null;
}

Element.prototype.query = function(query)
{
    return this.ownerDocument.evaluate(query, this, null, XPathResult.FIRST_ORDERED_NODE_TYPE, null).singleNodeValue;
}

Element.prototype.removeChildren = function()
{
    if (this.firstChild)
        this.textContent = "";
}

Element.prototype.isInsertionCaretInside = function()
{
    var selection = window.getSelection();
    if (!selection.rangeCount || !selection.isCollapsed)
        return false;
    var selectionRange = selection.getRangeAt(0);
    return selectionRange.startContainer.isSelfOrDescendant(this);
}

/**
 * @param {string} elementName
 * @param {string=} className
 */
Document.prototype.createElementWithClass = function(elementName, className)
{
    var element = this.createElement(elementName);
    if (className)
        element.className = className;
    return element;
}

/**
 * @param {string=} className
 */
Element.prototype.createChild = function(elementName, className)
{
    var element = this.ownerDocument.createElementWithClass(elementName, className);
    this.appendChild(element);
    return element;
}

DocumentFragment.prototype.createChild = Element.prototype.createChild;

/**
 * @param {string} text
 */
Element.prototype.createTextChild = function(text)
{
    var element = this.ownerDocument.createTextNode(text);
    this.appendChild(element);
    return element;
}

DocumentFragment.prototype.createTextChild = Element.prototype.createTextChild;

/**
 * @return {number}
 */
Element.prototype.totalOffsetLeft = function()
{
    return this.totalOffset().left;
}

/**
 * @return {number}
 */
Element.prototype.totalOffsetTop = function()
{
    return this.totalOffset().top;

}

Element.prototype.totalOffset = function()
{
    var rect = this.getBoundingClientRect();
    return { left: rect.left, top: rect.top };
}

Element.prototype.scrollOffset = function()
{
    var curLeft = 0;
    var curTop = 0;
    for (var element = this; element; element = element.scrollParent) {
        curLeft += element.scrollLeft;
        curTop += element.scrollTop;
    }
    return { left: curLeft, top: curTop };
}

/**
 * @constructor
 * @param {number=} x
 * @param {number=} y
 * @param {number=} width
 * @param {number=} height
 */
function AnchorBox(x, y, width, height)
{
    this.x = x || 0;
    this.y = y || 0;
    this.width = width || 0;
    this.height = height || 0;
}

/**
 * @param {!AnchorBox} box
 * @return {!AnchorBox}
 */
AnchorBox.prototype.relativeTo = function(box)
{
    return new AnchorBox(
        this.x - box.x, this.y - box.y, this.width, this.height);
};

/**
 * @param {!Element} element
 * @return {!AnchorBox}
 */
AnchorBox.prototype.relativeToElement = function(element)
{
    return this.relativeTo(element.boxInWindow(element.ownerDocument.defaultView));
};

/**
 * @param {?AnchorBox} anchorBox
 * @return {boolean}
 */
AnchorBox.prototype.equals = function(anchorBox)
{
    return !!anchorBox && this.x === anchorBox.x && this.y === anchorBox.y && this.width === anchorBox.width && this.height === anchorBox.height;
};

/**
 * @param {!Window} targetWindow
 * @return {!AnchorBox}
 */
Element.prototype.offsetRelativeToWindow = function(targetWindow)
{
    var elementOffset = new AnchorBox();
    var curElement = this;
    var curWindow = this.ownerDocument.defaultView;
    while (curWindow && curElement) {
        elementOffset.x += curElement.totalOffsetLeft();
        elementOffset.y += curElement.totalOffsetTop();
        if (curWindow === targetWindow)
            break;

        curElement = curWindow.frameElement;
        curWindow = curWindow.parent;
    }

    return elementOffset;
}

/**
 * @param {!Window} targetWindow
 * @return {!AnchorBox}
 */
Element.prototype.boxInWindow = function(targetWindow)
{
    targetWindow = targetWindow || this.ownerDocument.defaultView;

    var anchorBox = this.offsetRelativeToWindow(window);
    anchorBox.width = Math.min(this.offsetWidth, window.innerWidth - anchorBox.x);
    anchorBox.height = Math.min(this.offsetHeight, window.innerHeight - anchorBox.y);

    return anchorBox;
}

/**
 * @param {string} text
 */
Element.prototype.setTextAndTitle = function(text)
{
    this.textContent = text;
    this.title = text;
}

KeyboardEvent.prototype.__defineGetter__("data", function()
{
    // Emulate "data" attribute from DOM 3 TextInput event.
    // See http://www.w3.org/TR/DOM-Level-3-Events/#events-Events-TextEvent-data
    switch (this.type) {
        case "keypress":
            if (!this.ctrlKey && !this.metaKey)
                return String.fromCharCode(this.charCode);
            else
                return "";
        case "keydown":
        case "keyup":
            if (!this.ctrlKey && !this.metaKey && !this.altKey)
                return String.fromCharCode(this.which);
            else
                return "";
    }
});

/**
 * @param {boolean=} preventDefault
 */
Event.prototype.consume = function(preventDefault)
{
    this.stopImmediatePropagation();
    if (preventDefault)
        this.preventDefault();
    this.handled = true;
}

Text.prototype.select = function(start, end)
{
    start = start || 0;
    end = end || this.textContent.length;

    if (start < 0)
        start = end + start;

    var selection = this.ownerDocument.defaultView.getSelection();
    selection.removeAllRanges();
    var range = this.ownerDocument.createRange();
    range.setStart(this, start);
    range.setEnd(this, end);
    selection.addRange(range);
    return this;
}

Element.prototype.selectionLeftOffset = function()
{
    // Calculate selection offset relative to the current element.

    var selection = window.getSelection();
    if (!selection.containsNode(this, true))
        return null;

    var leftOffset = selection.anchorOffset;
    var node = selection.anchorNode;

    while (node !== this) {
        while (node.previousSibling) {
            node = node.previousSibling;
            leftOffset += node.textContent.length;
        }
        node = node.parentNode;
    }

    return leftOffset;
}

Node.prototype.isAncestor = function(node)
{
    if (!node)
        return false;

    var currentNode = node.parentNode;
    while (currentNode) {
        if (this === currentNode)
            return true;
        currentNode = currentNode.parentNode;
    }
    return false;
}

Node.prototype.isDescendant = function(descendant)
{
    return !!descendant && descendant.isAncestor(this);
}

Node.prototype.isSelfOrAncestor = function(node)
{
    return !!node && (node === this || this.isAncestor(node));
}

Node.prototype.isSelfOrDescendant = function(node)
{
    return !!node && (node === this || this.isDescendant(node));
}

Node.prototype.traverseNextNode = function(stayWithin)
{
    var node = this.firstChild;
    if (node)
        return node;

    if (stayWithin && this === stayWithin)
        return null;

    node = this.nextSibling;
    if (node)
        return node;

    node = this;
    while (node && !node.nextSibling && (!stayWithin || !node.parentNode || node.parentNode !== stayWithin))
        node = node.parentNode;
    if (!node)
        return null;

    return node.nextSibling;
}

Node.prototype.traversePreviousNode = function(stayWithin)
{
    if (stayWithin && this === stayWithin)
        return null;
    var node = this.previousSibling;
    while (node && node.lastChild)
        node = node.lastChild;
    if (node)
        return node;
    return this.parentNode;
}

/**
 * @param {*} text
 * @param {string=} placeholder
 * @return {boolean} true if was truncated
 */
Node.prototype.setTextContentTruncatedIfNeeded = function(text, placeholder)
{
    // Huge texts in the UI reduce rendering performance drastically.
    // Moreover, Blink/WebKit uses <unsigned short> internally for storing text content
    // length, so texts longer than 65535 are inherently displayed incorrectly.
    const maxTextContentLength = 65535;

    if (typeof text === "string" && text.length > maxTextContentLength) {
        this.textContent = typeof placeholder === "string" ? placeholder : text.trimEnd(maxTextContentLength);
        return true;
    }

    this.textContent = text;
    return false;
}

/**
 * @return {boolean}
 */
function isEnterKey(event) {
    // Check if in IME.
    return event.keyCode !== 229 && event.keyIdentifier === "Enter";
}

function consumeEvent(e)
{
    e.consume();
}
