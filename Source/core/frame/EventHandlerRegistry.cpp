// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "core/frame/EventHandlerRegistry.h"

#include "core/events/ThreadLocalEventNames.h"
#include "core/frame/DOMWindow.h"
#include "core/frame/LocalFrame.h"
#include "core/html/HTMLFrameOwnerElement.h"
#include "core/page/Page.h"
#include "core/page/scrolling/ScrollingCoordinator.h"

namespace WebCore {

EventHandlerRegistry::EventHandlerRegistry(FrameHost& frameHost)
    : m_frameHost(frameHost)
{
}

EventHandlerRegistry::~EventHandlerRegistry()
{
    checkConsistency();
}

bool EventHandlerRegistry::eventTypeToClass(const AtomicString& eventType, EventHandlerClass* result)
{
    if (eventType == EventTypeNames::scroll) {
        *result = ScrollEvent;
#if ASSERT_ENABLED
    } else if (eventType == EventTypeNames::load || eventType == EventTypeNames::mousemove || eventType == EventTypeNames::touchstart) {
        *result = EventsForTesting;
#endif
    } else {
        return false;
    }
    return true;
}

const EventTargetSet* EventHandlerRegistry::eventHandlerTargets(EventHandlerClass handlerClass) const
{
    checkConsistency();
    return &m_targets[handlerClass];
}

bool EventHandlerRegistry::hasEventHandlers(EventHandlerClass handlerClass) const
{
    return m_targets[handlerClass].size();
}

bool EventHandlerRegistry::updateEventHandlerTargets(ChangeOperation op, EventHandlerClass handlerClass, EventTarget* target)
{
    EventTargetSet* targets = &m_targets[handlerClass];
    if (op == Add) {
        if (!targets->add(target).isNewEntry) {
            // Just incremented refcount, no real change.
            return false;
        }
    } else {
        ASSERT(op == Remove || op == RemoveAll);
        ASSERT(op == RemoveAll || targets->contains(target));

        if (op == RemoveAll) {
            if (!targets->contains(target))
                return false;
            targets->removeAll(target);
        } else {
            if (!targets->remove(target)) {
                // Just decremented refcount, no real update.
                return false;
            }
        }
    }
    return true;
}

void EventHandlerRegistry::updateEventHandlerInternal(ChangeOperation op, EventHandlerClass handlerClass, EventTarget* target)
{
    bool hadHandlers = hasEventHandlers(handlerClass);
    updateEventHandlerTargets(op, handlerClass, target);
    bool hasHandlers = hasEventHandlers(handlerClass);

    if (hadHandlers != hasHandlers) {
        notifyHasHandlersChanged(handlerClass, hasHandlers);
    }
    checkConsistency();
}

void EventHandlerRegistry::updateEventHandlerOfType(ChangeOperation op, const AtomicString& eventType, EventTarget* target)
{
    EventHandlerClass handlerClass;
    if (!eventTypeToClass(eventType, &handlerClass))
        return;
    updateEventHandlerInternal(op, handlerClass, target);
}

void EventHandlerRegistry::didAddEventHandler(EventTarget& target, const AtomicString& eventType)
{
    updateEventHandlerOfType(Add, eventType, &target);
}

void EventHandlerRegistry::didRemoveEventHandler(EventTarget& target, const AtomicString& eventType)
{
    updateEventHandlerOfType(Remove, eventType, &target);
}

void EventHandlerRegistry::didAddEventHandler(EventTarget& target, EventHandlerClass handlerClass)
{
    updateEventHandlerInternal(Add, handlerClass, &target);
}

void EventHandlerRegistry::didRemoveEventHandler(EventTarget& target, EventHandlerClass handlerClass)
{
    updateEventHandlerInternal(Remove, handlerClass, &target);
}

void EventHandlerRegistry::didMoveIntoFrameHost(EventTarget& target)
{
    updateAllEventHandlers(Add, target);
}

void EventHandlerRegistry::didMoveOutOfFrameHost(EventTarget& target)
{
    updateAllEventHandlers(RemoveAll, target);
}

void EventHandlerRegistry::didRemoveAllEventHandlers(EventTarget& target)
{
    for (size_t i = 0; i < EventHandlerClassCount; ++i) {
        EventHandlerClass handlerClass = static_cast<EventHandlerClass>(i);
        updateEventHandlerInternal(RemoveAll, handlerClass, &target);
    }
}

void EventHandlerRegistry::updateAllEventHandlers(ChangeOperation op, EventTarget& target)
{
    if (!target.hasEventListeners())
        return;

    Vector<AtomicString> eventTypes = target.eventTypes();
    for (size_t i = 0; i < eventTypes.size(); ++i) {
        EventHandlerClass handlerClass;
        if (!eventTypeToClass(eventTypes[i], &handlerClass))
            continue;
        if (op == RemoveAll) {
            updateEventHandlerInternal(op, handlerClass, &target);
            continue;
        }
        for (unsigned count = target.getEventListeners(eventTypes[i]).size(); count > 0; --count)
            updateEventHandlerInternal(op, handlerClass, &target);
    }
}

void EventHandlerRegistry::notifyHasHandlersChanged(EventHandlerClass handlerClass, bool hasActiveHandlers)
{
    ScrollingCoordinator* scrollingCoordinator = m_frameHost.page().scrollingCoordinator();

    switch (handlerClass) {
    case ScrollEvent:
        if (scrollingCoordinator)
            scrollingCoordinator->updateHaveScrollEventHandlers();
        break;
#if ASSERT_ENABLED
    case EventsForTesting:
        break;
#endif
    default:
        ASSERT_NOT_REACHED();
        break;
    }
}

void EventHandlerRegistry::trace(Visitor* visitor)
{
    visitor->registerWeakMembers<EventHandlerRegistry, &EventHandlerRegistry::clearWeakMembers>(this);
}

void EventHandlerRegistry::clearWeakMembers(Visitor* visitor)
{
    Vector<EventTarget*> deadTargets;
    for (size_t i = 0; i < EventHandlerClassCount; ++i) {
        EventHandlerClass handlerClass = static_cast<EventHandlerClass>(i);
        const EventTargetSet* targets = &m_targets[handlerClass];
        for (EventTargetSet::const_iterator it = targets->begin(); it != targets->end(); ++it) {
            Node* node = it->key->toNode();
            DOMWindow* window = it->key->toDOMWindow();
            if (node && !visitor->isAlive(node)) {
                deadTargets.append(node);
            } else if (window && !visitor->isAlive(window)) {
                deadTargets.append(window);
            }
        }
    }
    for (size_t i = 0; i < deadTargets.size(); ++i)
        didRemoveAllEventHandlers(*deadTargets[i]);
}

void EventHandlerRegistry::documentDetached(Document& document)
{
    // Remove all event targets under the detached document.
    for (size_t handlerClassIndex = 0; handlerClassIndex < EventHandlerClassCount; ++handlerClassIndex) {
        EventHandlerClass handlerClass = static_cast<EventHandlerClass>(handlerClassIndex);
        Vector<EventTarget*> targetsToRemove;
        const EventTargetSet* targets = &m_targets[handlerClass];
        for (EventTargetSet::const_iterator iter = targets->begin(); iter != targets->end(); ++iter) {
            if (Node* node = iter->key->toNode()) {
                for (Document* doc = &node->document(); doc; doc = doc->ownerElement() ? &doc->ownerElement()->document() : 0) {
                    if (doc == &document) {
                        targetsToRemove.append(iter->key);
                        break;
                    }
                }
            } else if (iter->key->toDOMWindow()) {
                // DOMWindows may outlive their documents, so we shouldn't remove their handlers
                // here.
            } else {
                ASSERT_NOT_REACHED();
            }
        }
        for (size_t i = 0; i < targetsToRemove.size(); ++i)
            updateEventHandlerInternal(RemoveAll, handlerClass, targetsToRemove[i]);
    }
}

void EventHandlerRegistry::checkConsistency() const
{
#if ASSERT_ENABLED
    for (size_t i = 0; i < EventHandlerClassCount; ++i) {
        EventHandlerClass handlerClass = static_cast<EventHandlerClass>(i);
        const EventTargetSet* targets = &m_targets[handlerClass];
        for (EventTargetSet::const_iterator iter = targets->begin(); iter != targets->end(); ++iter) {
            if (Node* node = iter->key->toNode()) {
                // See the comment for |documentDetached| if either of these assertions fails.
                ASSERT(node->document().frameHost());
                ASSERT(node->document().frameHost() == &m_frameHost);
            } else if (DOMWindow* window = iter->key->toDOMWindow()) {
                // If any of these assertions fail, DOMWindow failed to unregister its handlers
                // properly.
                ASSERT(window->frame());
                ASSERT(window->frame()->host());
                ASSERT(window->frame()->host() == &m_frameHost);
            }
        }
    }
#endif // ASSERT_ENABLED
}

} // namespace WebCore
