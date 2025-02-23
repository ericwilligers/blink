/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2012 Digia Plc. and/or its subsidiary(-ies)
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
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "core/page/EventHandler.h"

#include "HTMLNames.h"
#include "RuntimeEnabledFeatures.h"
#include "SVGNames.h"
#include "bindings/v8/ExceptionStatePlaceholder.h"
#include "core/clipboard/Clipboard.h"
#include "core/clipboard/DataObject.h"
#include "core/dom/Document.h"
#include "core/dom/DocumentMarkerController.h"
#include "core/dom/FullscreenElementStack.h"
#include "core/dom/NodeRenderingTraversal.h"
#include "core/dom/TouchList.h"
#include "core/dom/shadow/ShadowRoot.h"
#include "core/editing/Editor.h"
#include "core/editing/FrameSelection.h"
#include "core/editing/TextIterator.h"
#include "core/editing/htmlediting.h"
#include "core/events/DOMWindowEventQueue.h"
#include "core/events/EventPath.h"
#include "core/events/KeyboardEvent.h"
#include "core/events/MouseEvent.h"
#include "core/events/TextEvent.h"
#include "core/events/TouchEvent.h"
#include "core/events/WheelEvent.h"
#include "core/fetch/ImageResource.h"
#include "core/frame/FrameView.h"
#include "core/frame/LocalFrame.h"
#include "core/html/HTMLDialogElement.h"
#include "core/html/HTMLFrameElementBase.h"
#include "core/html/HTMLFrameSetElement.h"
#include "core/html/HTMLInputElement.h"
#include "core/loader/FrameLoader.h"
#include "core/loader/FrameLoaderClient.h"
#include "core/page/AutoscrollController.h"
#include "core/page/BackForwardClient.h"
#include "core/page/Chrome.h"
#include "core/page/ChromeClient.h"
#include "core/page/DragController.h"
#include "core/page/DragState.h"
#include "core/page/EditorClient.h"
#include "core/page/FocusController.h"
#include "core/page/FrameTree.h"
#include "core/inspector/InspectorController.h"
#include "core/page/MouseEventWithHitTestResults.h"
#include "core/page/Page.h"
#include "core/frame/Settings.h"
#include "core/page/SpatialNavigation.h"
#include "core/page/TouchAdjustment.h"
#include "core/rendering/HitTestRequest.h"
#include "core/rendering/HitTestResult.h"
#include "core/rendering/RenderFlowThread.h"
#include "core/rendering/RenderLayer.h"
#include "core/rendering/RenderTextControlSingleLine.h"
#include "core/rendering/RenderView.h"
#include "core/rendering/RenderWidget.h"
#include "core/rendering/style/RenderStyle.h"
#include "core/svg/SVGDocumentExtensions.h"
#include "core/svg/SVGUseElement.h"
#include "platform/PlatformGestureEvent.h"
#include "platform/PlatformKeyboardEvent.h"
#include "platform/PlatformTouchEvent.h"
#include "platform/PlatformWheelEvent.h"
#include "platform/WindowsKeyboardCodes.h"
#include "platform/geometry/FloatPoint.h"
#include "platform/graphics/Image.h"
#include "platform/heap/Handle.h"
#include "platform/scroll/ScrollAnimator.h"
#include "platform/scroll/Scrollbar.h"
#include "wtf/Assertions.h"
#include "wtf/CurrentTime.h"
#include "wtf/StdLibExtras.h"
#include "wtf/TemporaryChange.h"

namespace WebCore {

using namespace HTMLNames;
using namespace SVGNames;

// The link drag hysteresis is much larger than the others because there
// needs to be enough space to cancel the link press without starting a link drag,
// and because dragging links is rare.
static const int LinkDragHysteresis = 40;
static const int ImageDragHysteresis = 5;
static const int TextDragHysteresis = 3;
static const int GeneralDragHysteresis = 3;

// The amount of time to wait before sending a fake mouse event, triggered
// during a scroll. The short interval is used if the content responds to the mouse events quickly enough,
// otherwise the long interval is used.
static const double fakeMouseMoveShortInterval = 0.1;
static const double fakeMouseMoveLongInterval = 0.250;

// The amount of time to wait for a cursor update on style and layout changes
// Set to 50Hz, no need to be faster than common screen refresh rate
static const double cursorUpdateInterval = 0.02;

static const int maximumCursorSize = 128;

// It's pretty unlikely that a scale of less than one would ever be used. But all we really
// need to ensure here is that the scale isn't so small that integer overflow can occur when
// dividing cursor sizes (limited above) by the scale.
static const double minimumCursorScale = 0.001;

// The minimum amount of time an element stays active after a ShowPress
// This is roughly 9 frames, which should be long enough to be noticeable.
static const double minimumActiveInterval = 0.15;

#if OS(MACOSX)
static const double TextDragDelay = 0.15;
#else
static const double TextDragDelay = 0.0;
#endif

enum NoCursorChangeType { NoCursorChange };

class OptionalCursor {
public:
    OptionalCursor(NoCursorChangeType) : m_isCursorChange(false) { }
    OptionalCursor(const Cursor& cursor) : m_isCursorChange(true), m_cursor(cursor) { }

    bool isCursorChange() const { return m_isCursorChange; }
    const Cursor& cursor() const { ASSERT(m_isCursorChange); return m_cursor; }

private:
    bool m_isCursorChange;
    Cursor m_cursor;
};

class MaximumDurationTracker {
public:
    explicit MaximumDurationTracker(double *maxDuration)
        : m_maxDuration(maxDuration)
        , m_start(monotonicallyIncreasingTime())
    {
    }

    ~MaximumDurationTracker()
    {
        *m_maxDuration = max(*m_maxDuration, monotonicallyIncreasingTime() - m_start);
    }

private:
    double* m_maxDuration;
    double m_start;
};

static inline ScrollGranularity wheelGranularityToScrollGranularity(unsigned deltaMode)
{
    switch (deltaMode) {
    case WheelEvent::DOM_DELTA_PAGE:
        return ScrollByPage;
    case WheelEvent::DOM_DELTA_LINE:
        return ScrollByLine;
    case WheelEvent::DOM_DELTA_PIXEL:
        return ScrollByPixel;
    default:
        return ScrollByPixel;
    }
}

// Refetch the event target node if it is removed or currently is the shadow node inside an <input> element.
// If a mouse event handler changes the input element type to one that has a widget associated,
// we'd like to EventHandler::handleMousePressEvent to pass the event to the widget and thus the
// event target node can't still be the shadow node.
static inline bool shouldRefetchEventTarget(const MouseEventWithHitTestResults& mev)
{
    Node* targetNode = mev.targetNode();
    if (!targetNode || !targetNode->parentNode())
        return true;
    return targetNode->isShadowRoot() && isHTMLInputElement(*toShadowRoot(targetNode)->host());
}

EventHandler::EventHandler(LocalFrame* frame)
    : m_frame(frame)
    , m_mousePressed(false)
    , m_capturesDragging(false)
    , m_mouseDownMayStartSelect(false)
    , m_mouseDownMayStartDrag(false)
    , m_mouseDownWasSingleClickInSelection(false)
    , m_selectionInitiationState(HaveNotStartedSelection)
    , m_hoverTimer(this, &EventHandler::hoverTimerFired)
    , m_cursorUpdateTimer(this, &EventHandler::cursorUpdateTimerFired)
    , m_mouseDownMayStartAutoscroll(false)
    , m_mouseDownWasInSubframe(false)
    , m_fakeMouseMoveEventTimer(this, &EventHandler::fakeMouseMoveEventTimerFired)
    , m_svgPan(false)
    , m_resizeScrollableArea(0)
    , m_eventHandlerWillResetCapturingMouseEventsNode(0)
    , m_clickCount(0)
    , m_shouldOnlyFireDragOverEvent(false)
    , m_mousePositionIsUnknown(true)
    , m_mouseDownTimestamp(0)
    , m_widgetIsLatched(false)
    , m_touchPressed(false)
    , m_scrollGestureHandlingNode(nullptr)
    , m_lastHitTestResultOverWidget(false)
    , m_maxMouseMovedDuration(0)
    , m_baseEventType(PlatformEvent::NoType)
    , m_didStartDrag(false)
    , m_longTapShouldInvokeContextMenu(false)
    , m_activeIntervalTimer(this, &EventHandler::activeIntervalTimerFired)
    , m_lastShowPressTimestamp(0)
{
}

EventHandler::~EventHandler()
{
    ASSERT(!m_fakeMouseMoveEventTimer.isActive());
}

DragState& EventHandler::dragState()
{
#if ENABLE(OILPAN)
    DEFINE_STATIC_LOCAL(Persistent<DragState>, state, (new DragState()));
    return *state;
#else
    DEFINE_STATIC_LOCAL(DragState, state, ());
    return state;
#endif
}

void EventHandler::clear()
{
    m_hoverTimer.stop();
    m_cursorUpdateTimer.stop();
    m_fakeMouseMoveEventTimer.stop();
    m_activeIntervalTimer.stop();
    m_resizeScrollableArea = 0;
    m_nodeUnderMouse = nullptr;
    m_lastNodeUnderMouse = nullptr;
    m_lastMouseMoveEventSubframe = nullptr;
    m_lastScrollbarUnderMouse = nullptr;
    m_clickCount = 0;
    m_clickNode = nullptr;
    m_frameSetBeingResized = nullptr;
    m_dragTarget = nullptr;
    m_shouldOnlyFireDragOverEvent = false;
    m_mousePositionIsUnknown = true;
    m_lastKnownMousePosition = IntPoint();
    m_lastKnownMouseGlobalPosition = IntPoint();
    m_lastMouseDownUserGestureToken.clear();
    m_mousePressNode = nullptr;
    m_mousePressed = false;
    m_capturesDragging = false;
    m_capturingMouseEventsNode = nullptr;
    m_latchedWheelEventNode = nullptr;
    m_previousWheelScrolledNode = nullptr;
    m_targetForTouchID.clear();
    m_touchSequenceDocument.clear();
    m_scrollGestureHandlingNode = nullptr;
    m_lastHitTestResultOverWidget = false;
    m_previousGestureScrolledNode = nullptr;
    m_scrollbarHandlingScrollGesture = nullptr;
    m_maxMouseMovedDuration = 0;
    m_baseEventType = PlatformEvent::NoType;
    m_didStartDrag = false;
    m_touchPressed = false;
    m_mouseDownMayStartSelect = false;
    m_mouseDownMayStartDrag = false;
    m_lastShowPressTimestamp = 0;
    m_lastDeferredTapElement = nullptr;
}

void EventHandler::nodeWillBeRemoved(Node& nodeToBeRemoved)
{
    if (!nodeToBeRemoved.containsIncludingShadowDOM(m_clickNode.get()))
        return;
    if (nodeToBeRemoved.isInShadowTree()) {
        m_clickNode = nodeToBeRemoved.parentOrShadowHostNode();
    } else {
        // We don't dispatch click events if the mousedown node is removed
        // before a mouseup event. It is compatible with IE and Firefox.
        m_clickNode = nullptr;
    }
}

static void setSelectionIfNeeded(FrameSelection& selection, const VisibleSelection& newSelection)
{
    if (selection.selection() != newSelection)
        selection.setSelection(newSelection);
}

static inline bool dispatchSelectStart(Node* node)
{
    if (!node || !node->renderer())
        return true;

    return node->dispatchEvent(Event::createCancelableBubble(EventTypeNames::selectstart));
}

static VisibleSelection expandSelectionToRespectUserSelectAll(Node* targetNode, const VisibleSelection& selection)
{
    Node* rootUserSelectAll = Position::rootUserSelectAllForNode(targetNode);
    if (!rootUserSelectAll)
        return selection;

    VisibleSelection newSelection(selection);
    newSelection.setBase(positionBeforeNode(rootUserSelectAll).upstream(CanCrossEditingBoundary));
    newSelection.setExtent(positionAfterNode(rootUserSelectAll).downstream(CanCrossEditingBoundary));

    return newSelection;
}

bool EventHandler::updateSelectionForMouseDownDispatchingSelectStart(Node* targetNode, const VisibleSelection& selection, TextGranularity granularity)
{
    if (Position::nodeIsUserSelectNone(targetNode))
        return false;

    if (!dispatchSelectStart(targetNode))
        return false;

    if (selection.isRange())
        m_selectionInitiationState = ExtendedSelection;
    else {
        granularity = CharacterGranularity;
        m_selectionInitiationState = PlacedCaret;
    }

    m_frame->selection().setNonDirectionalSelectionIfNeeded(selection, granularity);

    return true;
}

void EventHandler::selectClosestWordFromHitTestResult(const HitTestResult& result, AppendTrailingWhitespace appendTrailingWhitespace)
{
    Node* innerNode = result.targetNode();
    VisibleSelection newSelection;

    if (innerNode && innerNode->renderer()) {
        VisiblePosition pos(innerNode->renderer()->positionForPoint(result.localPoint()));
        if (pos.isNotNull()) {
            newSelection = VisibleSelection(pos);
            newSelection.expandUsingGranularity(WordGranularity);
        }

        if (appendTrailingWhitespace == ShouldAppendTrailingWhitespace && newSelection.isRange())
            newSelection.appendTrailingWhitespace();

        updateSelectionForMouseDownDispatchingSelectStart(innerNode, expandSelectionToRespectUserSelectAll(innerNode, newSelection), WordGranularity);
    }
}

void EventHandler::selectClosestMisspellingFromHitTestResult(const HitTestResult& result, AppendTrailingWhitespace appendTrailingWhitespace)
{
    Node* innerNode = result.targetNode();
    VisibleSelection newSelection;

    if (innerNode && innerNode->renderer()) {
        VisiblePosition pos(innerNode->renderer()->positionForPoint(result.localPoint()));
        Position start = pos.deepEquivalent();
        Position end = pos.deepEquivalent();
        if (pos.isNotNull()) {
            WillBeHeapVector<DocumentMarker*> markers = innerNode->document().markers().markersInRange(makeRange(pos, pos).get(), DocumentMarker::MisspellingMarkers());
            if (markers.size() == 1) {
                start.moveToOffset(markers[0]->startOffset());
                end.moveToOffset(markers[0]->endOffset());
                newSelection = VisibleSelection(start, end);
            }
        }

        if (appendTrailingWhitespace == ShouldAppendTrailingWhitespace && newSelection.isRange())
            newSelection.appendTrailingWhitespace();

        updateSelectionForMouseDownDispatchingSelectStart(innerNode, expandSelectionToRespectUserSelectAll(innerNode, newSelection), WordGranularity);
    }
}

void EventHandler::selectClosestWordFromMouseEvent(const MouseEventWithHitTestResults& result)
{
    if (m_mouseDownMayStartSelect) {
        selectClosestWordFromHitTestResult(result.hitTestResult(),
            (result.event().clickCount() == 2 && m_frame->editor().isSelectTrailingWhitespaceEnabled()) ? ShouldAppendTrailingWhitespace : DontAppendTrailingWhitespace);
    }
}

void EventHandler::selectClosestMisspellingFromMouseEvent(const MouseEventWithHitTestResults& result)
{
    if (m_mouseDownMayStartSelect) {
        selectClosestMisspellingFromHitTestResult(result.hitTestResult(),
            (result.event().clickCount() == 2 && m_frame->editor().isSelectTrailingWhitespaceEnabled()) ? ShouldAppendTrailingWhitespace : DontAppendTrailingWhitespace);
    }
}

void EventHandler::selectClosestWordOrLinkFromMouseEvent(const MouseEventWithHitTestResults& result)
{
    if (!result.hitTestResult().isLiveLink())
        return selectClosestWordFromMouseEvent(result);

    Node* innerNode = result.targetNode();

    if (innerNode && innerNode->renderer() && m_mouseDownMayStartSelect) {
        VisibleSelection newSelection;
        Element* URLElement = result.hitTestResult().URLElement();
        VisiblePosition pos(innerNode->renderer()->positionForPoint(result.localPoint()));
        if (pos.isNotNull() && pos.deepEquivalent().deprecatedNode()->isDescendantOf(URLElement))
            newSelection = VisibleSelection::selectionFromContentsOfNode(URLElement);

        updateSelectionForMouseDownDispatchingSelectStart(innerNode, expandSelectionToRespectUserSelectAll(innerNode, newSelection), WordGranularity);
    }
}

bool EventHandler::handleMousePressEventDoubleClick(const MouseEventWithHitTestResults& event)
{
    TRACE_EVENT0("webkit", "EventHandler::handleMousePressEventDoubleClick");

    if (event.event().button() != LeftButton)
        return false;

    if (m_frame->selection().isRange()) {
        // A double-click when range is already selected
        // should not change the selection.  So, do not call
        // selectClosestWordFromMouseEvent, but do set
        // m_beganSelectingText to prevent handleMouseReleaseEvent
        // from setting caret selection.
        m_selectionInitiationState = ExtendedSelection;
    } else {
        selectClosestWordFromMouseEvent(event);
    }
    return true;
}

bool EventHandler::handleMousePressEventTripleClick(const MouseEventWithHitTestResults& event)
{
    TRACE_EVENT0("webkit", "EventHandler::handleMousePressEventTripleClick");

    if (event.event().button() != LeftButton)
        return false;

    Node* innerNode = event.targetNode();
    if (!(innerNode && innerNode->renderer() && m_mouseDownMayStartSelect))
        return false;

    VisibleSelection newSelection;
    VisiblePosition pos(innerNode->renderer()->positionForPoint(event.localPoint()));
    if (pos.isNotNull()) {
        newSelection = VisibleSelection(pos);
        newSelection.expandUsingGranularity(ParagraphGranularity);
    }

    return updateSelectionForMouseDownDispatchingSelectStart(innerNode, expandSelectionToRespectUserSelectAll(innerNode, newSelection), ParagraphGranularity);
}

static int textDistance(const Position& start, const Position& end)
{
    RefPtrWillBeRawPtr<Range> range = Range::create(*start.document(), start, end);
    return TextIterator::rangeLength(range.get(), true);
}

bool EventHandler::handleMousePressEventSingleClick(const MouseEventWithHitTestResults& event)
{
    TRACE_EVENT0("webkit", "EventHandler::handleMousePressEventSingleClick");

    m_frame->document()->updateLayoutIgnorePendingStylesheets();
    Node* innerNode = event.targetNode();
    if (!(innerNode && innerNode->renderer() && m_mouseDownMayStartSelect))
        return false;

    // Extend the selection if the Shift key is down, unless the click is in a link.
    bool extendSelection = event.event().shiftKey() && !event.isOverLink();

    // Don't restart the selection when the mouse is pressed on an
    // existing selection so we can allow for text dragging.
    if (FrameView* view = m_frame->view()) {
        LayoutPoint vPoint = view->windowToContents(event.event().position());
        if (!extendSelection && m_frame->selection().contains(vPoint)) {
            m_mouseDownWasSingleClickInSelection = true;
            return false;
        }
    }

    VisiblePosition visiblePos(innerNode->renderer()->positionForPoint(event.localPoint()));
    if (visiblePos.isNull())
        visiblePos = VisiblePosition(firstPositionInOrBeforeNode(innerNode), DOWNSTREAM);
    Position pos = visiblePos.deepEquivalent();

    VisibleSelection newSelection = m_frame->selection().selection();
    TextGranularity granularity = CharacterGranularity;

    if (extendSelection && newSelection.isCaretOrRange()) {
        VisibleSelection selectionInUserSelectAll(expandSelectionToRespectUserSelectAll(innerNode, VisibleSelection(VisiblePosition(pos))));
        if (selectionInUserSelectAll.isRange()) {
            if (comparePositions(selectionInUserSelectAll.start(), newSelection.start()) < 0)
                pos = selectionInUserSelectAll.start();
            else if (comparePositions(newSelection.end(), selectionInUserSelectAll.end()) < 0)
                pos = selectionInUserSelectAll.end();
        }

        if (!m_frame->editor().behavior().shouldConsiderSelectionAsDirectional()) {
            if (pos.isNotNull()) {
                // See <rdar://problem/3668157> REGRESSION (Mail): shift-click deselects when selection
                // was created right-to-left
                Position start = newSelection.start();
                Position end = newSelection.end();
                int distanceToStart = textDistance(start, pos);
                int distanceToEnd = textDistance(pos, end);
                if (distanceToStart <= distanceToEnd)
                    newSelection = VisibleSelection(end, pos);
                else
                    newSelection = VisibleSelection(start, pos);
            }
        } else
            newSelection.setExtent(pos);

        if (m_frame->selection().granularity() != CharacterGranularity) {
            granularity = m_frame->selection().granularity();
            newSelection.expandUsingGranularity(m_frame->selection().granularity());
        }
    } else {
        newSelection = expandSelectionToRespectUserSelectAll(innerNode, VisibleSelection(visiblePos));
    }

    bool handled = updateSelectionForMouseDownDispatchingSelectStart(innerNode, newSelection, granularity);
    return handled;
}

static inline bool canMouseDownStartSelect(Node* node)
{
    if (!node || !node->renderer())
        return true;

    if (!node->canStartSelection())
        return false;

    return true;
}

bool EventHandler::handleMousePressEvent(const MouseEventWithHitTestResults& event)
{
    TRACE_EVENT0("webkit", "EventHandler::handleMousePressEvent");

    // Reset drag state.
    dragState().m_dragSrc = nullptr;

    cancelFakeMouseMoveEvent();

    m_frame->document()->updateLayoutIgnorePendingStylesheets();

    if (ScrollView* scrollView = m_frame->view()) {
        if (scrollView->isPointInScrollbarCorner(event.event().position()))
            return false;
    }

    bool singleClick = event.event().clickCount() <= 1;

    // If we got the event back, that must mean it wasn't prevented,
    // so it's allowed to start a drag or selection if it wasn't in a scrollbar.
    m_mouseDownMayStartSelect = canMouseDownStartSelect(event.targetNode()) && !event.scrollbar();

    m_mouseDownMayStartDrag = singleClick;

    m_mouseDownWasSingleClickInSelection = false;

    m_mouseDown = event.event();

    if (event.isOverWidget() && passWidgetMouseDownEventToWidget(event))
        return true;

    if (m_frame->document()->isSVGDocument() && m_frame->document()->accessSVGExtensions().zoomAndPanEnabled()) {
        if (event.event().shiftKey() && singleClick) {
            m_svgPan = true;
            m_frame->document()->accessSVGExtensions().startPan(m_frame->view()->windowToContents(event.event().position()));
            return true;
        }
    }

    // We don't do this at the start of mouse down handling,
    // because we don't want to do it until we know we didn't hit a widget.
    if (singleClick)
        focusDocumentView();

    Node* innerNode = event.targetNode();

    m_mousePressNode = innerNode;
    m_dragStartPos = event.event().position();

    bool swallowEvent = false;
    m_mousePressed = true;
    m_selectionInitiationState = HaveNotStartedSelection;

    if (event.event().clickCount() == 2)
        swallowEvent = handleMousePressEventDoubleClick(event);
    else if (event.event().clickCount() >= 3)
        swallowEvent = handleMousePressEventTripleClick(event);
    else
        swallowEvent = handleMousePressEventSingleClick(event);

    m_mouseDownMayStartAutoscroll = m_mouseDownMayStartSelect
        || (m_mousePressNode && m_mousePressNode->renderBox() && m_mousePressNode->renderBox()->canBeProgramaticallyScrolled());

    return swallowEvent;
}

bool EventHandler::handleMouseDraggedEvent(const MouseEventWithHitTestResults& event)
{
    TRACE_EVENT0("webkit", "EventHandler::handleMouseDraggedEvent");

    if (!m_mousePressed)
        return false;

    if (handleDrag(event, ShouldCheckDragHysteresis))
        return true;

    Node* targetNode = event.targetNode();
    if (event.event().button() != LeftButton || !targetNode)
        return false;

    RenderObject* renderer = targetNode->renderer();
    if (!renderer) {
        Node* parent = NodeRenderingTraversal::parent(targetNode);
        if (!parent)
            return false;

        renderer = parent->renderer();
        if (!renderer || !renderer->isListBox())
            return false;
    }

    m_mouseDownMayStartDrag = false;

    if (m_mouseDownMayStartAutoscroll && !panScrollInProgress()) {
        if (AutoscrollController* controller = autoscrollController()) {
            controller->startAutoscrollForSelection(renderer);
            m_mouseDownMayStartAutoscroll = false;
        }
    }

    if (m_selectionInitiationState != ExtendedSelection) {
        HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::ConfusingAndOftenMisusedDisallowShadowContent);
        HitTestResult result(m_mouseDownPos);
        m_frame->document()->renderView()->hitTest(request, result);

        updateSelectionForMouseDrag(result);
    }
    updateSelectionForMouseDrag(event.hitTestResult());
    return true;
}

void EventHandler::updateSelectionForMouseDrag()
{
    FrameView* view = m_frame->view();
    if (!view)
        return;
    RenderView* renderer = m_frame->contentRenderer();
    if (!renderer)
        return;

    HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::Move | HitTestRequest::ConfusingAndOftenMisusedDisallowShadowContent);
    HitTestResult result(view->windowToContents(m_lastKnownMousePosition));
    renderer->hitTest(request, result);
    updateSelectionForMouseDrag(result);
}

void EventHandler::updateSelectionForMouseDrag(const HitTestResult& hitTestResult)
{
    if (!m_mouseDownMayStartSelect)
        return;

    Node* target = hitTestResult.targetNode();
    if (!target)
        return;

    VisiblePosition targetPosition = m_frame->selection().selection().visiblePositionRespectingEditingBoundary(hitTestResult.localPoint(), target);
    // Don't modify the selection if we're not on a node.
    if (targetPosition.isNull())
        return;

    // Restart the selection if this is the first mouse move. This work is usually
    // done in handleMousePressEvent, but not if the mouse press was on an existing selection.
    VisibleSelection newSelection = m_frame->selection().selection();

    // Special case to limit selection to the containing block for SVG text.
    // FIXME: Isn't there a better non-SVG-specific way to do this?
    if (Node* selectionBaseNode = newSelection.base().deprecatedNode())
        if (RenderObject* selectionBaseRenderer = selectionBaseNode->renderer())
            if (selectionBaseRenderer->isSVGText())
                if (target->renderer()->containingBlock() != selectionBaseRenderer->containingBlock())
                    return;

    if (m_selectionInitiationState == HaveNotStartedSelection && !dispatchSelectStart(target))
        return;

    if (m_selectionInitiationState != ExtendedSelection) {
        // Always extend selection here because it's caused by a mouse drag
        m_selectionInitiationState = ExtendedSelection;
        newSelection = VisibleSelection(targetPosition);
    }

    if (RuntimeEnabledFeatures::userSelectAllEnabled()) {
        Node* rootUserSelectAllForMousePressNode = Position::rootUserSelectAllForNode(m_mousePressNode.get());
        if (rootUserSelectAllForMousePressNode && rootUserSelectAllForMousePressNode == Position::rootUserSelectAllForNode(target)) {
            newSelection.setBase(positionBeforeNode(rootUserSelectAllForMousePressNode).upstream(CanCrossEditingBoundary));
            newSelection.setExtent(positionAfterNode(rootUserSelectAllForMousePressNode).downstream(CanCrossEditingBoundary));
        } else {
            // Reset base for user select all when base is inside user-select-all area and extent < base.
            if (rootUserSelectAllForMousePressNode && comparePositions(target->renderer()->positionForPoint(hitTestResult.localPoint()), m_mousePressNode->renderer()->positionForPoint(m_dragStartPos)) < 0)
                newSelection.setBase(positionAfterNode(rootUserSelectAllForMousePressNode).downstream(CanCrossEditingBoundary));

            Node* rootUserSelectAllForTarget = Position::rootUserSelectAllForNode(target);
            if (rootUserSelectAllForTarget && m_mousePressNode->renderer() && comparePositions(target->renderer()->positionForPoint(hitTestResult.localPoint()), m_mousePressNode->renderer()->positionForPoint(m_dragStartPos)) < 0)
                newSelection.setExtent(positionBeforeNode(rootUserSelectAllForTarget).upstream(CanCrossEditingBoundary));
            else if (rootUserSelectAllForTarget && m_mousePressNode->renderer())
                newSelection.setExtent(positionAfterNode(rootUserSelectAllForTarget).downstream(CanCrossEditingBoundary));
            else
                newSelection.setExtent(targetPosition);
        }
    } else {
        newSelection.setExtent(targetPosition);
    }

    if (m_frame->selection().granularity() != CharacterGranularity)
        newSelection.expandUsingGranularity(m_frame->selection().granularity());

    m_frame->selection().setNonDirectionalSelectionIfNeeded(newSelection, m_frame->selection().granularity(),
        FrameSelection::AdjustEndpointsAtBidiBoundary);
}

bool EventHandler::handleMouseReleaseEvent(const MouseEventWithHitTestResults& event)
{
    AutoscrollController* controller = autoscrollController();
    if (controller && controller->autoscrollInProgress())
        stopAutoscroll();

    // Used to prevent mouseMoveEvent from initiating a drag before
    // the mouse is pressed again.
    m_mousePressed = false;
    m_capturesDragging = false;
    m_mouseDownMayStartDrag = false;
    m_mouseDownMayStartSelect = false;
    m_mouseDownMayStartAutoscroll = false;
    m_mouseDownWasInSubframe = false;

    bool handled = false;

    // Clear the selection if the mouse didn't move after the last mouse
    // press and it's not a context menu click.  We do this so when clicking
    // on the selection, the selection goes away.  However, if we are
    // editing, place the caret.
    if (m_mouseDownWasSingleClickInSelection && m_selectionInitiationState != ExtendedSelection
            && m_dragStartPos == event.event().position()
            && m_frame->selection().isRange()
            && event.event().button() != RightButton) {
        VisibleSelection newSelection;
        Node* node = event.targetNode();
        bool caretBrowsing = m_frame->settings() && m_frame->settings()->caretBrowsingEnabled();
        if (node && node->renderer() && (caretBrowsing || node->rendererIsEditable())) {
            VisiblePosition pos = VisiblePosition(node->renderer()->positionForPoint(event.localPoint()));
            newSelection = VisibleSelection(pos);
        }

        setSelectionIfNeeded(m_frame->selection(), newSelection);

        handled = true;
    }

    m_frame->selection().notifyRendererOfSelectionChange(UserTriggered);

    m_frame->selection().selectFrameElementInParentIfFullySelected();

    if (event.event().button() == MiddleButton && !event.isOverLink()) {
        // Ignore handled, since we want to paste to where the caret was placed anyway.
        handled = handlePasteGlobalSelection(event.event()) || handled;
    }

    return handled;
}

#if OS(WIN)

void EventHandler::startPanScrolling(RenderObject* renderer)
{
    if (!renderer->isBox())
        return;
    AutoscrollController* controller = autoscrollController();
    if (!controller)
        return;
    controller->startPanScrolling(toRenderBox(renderer), lastKnownMousePosition());
    invalidateClick();
}

#endif // OS(WIN)

AutoscrollController* EventHandler::autoscrollController() const
{
    if (Page* page = m_frame->page())
        return &page->autoscrollController();
    return 0;
}

bool EventHandler::panScrollInProgress() const
{
    return autoscrollController() && autoscrollController()->panScrollInProgress();
}

HitTestResult EventHandler::hitTestResultAtPoint(const LayoutPoint& point, HitTestRequest::HitTestRequestType hitType, const LayoutSize& padding)
{
    TRACE_EVENT0("webkit", "EventHandler::hitTestResultAtPoint");

    // We always send hitTestResultAtPoint to the main frame if we have one,
    // otherwise we might hit areas that are obscured by higher frames.
    if (Page* page = m_frame->page()) {
        LocalFrame* mainFrame = page->mainFrame();
        if (m_frame != mainFrame) {
            FrameView* frameView = m_frame->view();
            FrameView* mainView = mainFrame->view();
            if (frameView && mainView) {
                IntPoint mainFramePoint = mainView->rootViewToContents(frameView->contentsToRootView(roundedIntPoint(point)));
                return mainFrame->eventHandler().hitTestResultAtPoint(mainFramePoint, hitType, padding);
            }
        }
    }

    HitTestResult result(point, padding.height(), padding.width(), padding.height(), padding.width());

    // RenderView::hitTest causes a layout, and we don't want to hit that until the first
    // layout because until then, there is nothing shown on the screen - the user can't
    // have intentionally clicked on something belonging to this page. Furthermore,
    // mousemove events before the first layout should not lead to a premature layout()
    // happening, which could show a flash of white.
    // See also the similar code in Document::prepareMouseEvent.
    if (!m_frame->contentRenderer() || !m_frame->view() || !m_frame->view()->didFirstLayout())
        return result;

    // hitTestResultAtPoint is specifically used to hitTest into all frames, thus it always allows child frame content.
    HitTestRequest request(hitType | HitTestRequest::AllowChildFrameContent);
    m_frame->contentRenderer()->hitTest(request, result);
    if (!request.readOnly())
        m_frame->document()->updateHoverActiveState(request, result.innerElement());

    if (request.disallowsShadowContent())
        result.setToNodesInDocumentTreeScope();

    return result;
}

void EventHandler::stopAutoscroll()
{
    if (AutoscrollController* controller = autoscrollController())
        controller->stopAutoscroll();
}

Node* EventHandler::mousePressNode() const
{
    return m_mousePressNode.get();
}

bool EventHandler::scroll(ScrollDirection direction, ScrollGranularity granularity, Node* startNode, Node** stopNode, float delta, IntPoint absolutePoint)
{
    if (!delta)
        return false;

    Node* node = startNode;

    if (!node)
        node = m_frame->document()->focusedElement();

    if (!node)
        node = m_mousePressNode.get();

    if (!node || !node->renderer())
        return false;

    RenderBox* curBox = node->renderer()->enclosingBox();
    while (curBox && !curBox->isRenderView()) {
        ScrollDirection physicalDirection = toPhysicalDirection(
            direction, curBox->isHorizontalWritingMode(), curBox->style()->isFlippedBlocksWritingMode());

        // If we're at the stopNode, we should try to scroll it but we shouldn't bubble past it
        bool shouldStopBubbling = stopNode && *stopNode && curBox->node() == *stopNode;
        bool didScroll = curBox->scroll(physicalDirection, granularity, delta);

        if (didScroll && stopNode)
            *stopNode = curBox->node();

        if (didScroll || shouldStopBubbling) {
            setFrameWasScrolledByUser();
            return true;
        }

        curBox = curBox->containingBlock();
    }

    return false;
}

bool EventHandler::bubblingScroll(ScrollDirection direction, ScrollGranularity granularity, Node* startingNode)
{
    // The layout needs to be up to date to determine if we can scroll. We may be
    // here because of an onLoad event, in which case the final layout hasn't been performed yet.
    m_frame->document()->updateLayoutIgnorePendingStylesheets();
    if (scroll(direction, granularity, startingNode))
        return true;
    LocalFrame* frame = m_frame;
    FrameView* view = frame->view();
    if (view && view->scroll(direction, granularity))
        return true;
    frame = frame->tree().parent();
    if (!frame)
        return false;
    return frame->eventHandler().bubblingScroll(direction, granularity, m_frame->ownerElement());
}

IntPoint EventHandler::lastKnownMousePosition() const
{
    return m_lastKnownMousePosition;
}

static LocalFrame* subframeForTargetNode(Node* node)
{
    if (!node)
        return 0;

    RenderObject* renderer = node->renderer();
    if (!renderer || !renderer->isWidget())
        return 0;

    // FIXME: This explicit check is needed only until RemoteFrames have RemoteFrameViews.
    if (isHTMLFrameElementBase(node) && toHTMLFrameElementBase(node)->contentFrame() && toHTMLFrameElementBase(node)->contentFrame()->isRemoteFrameTemporary())
        return 0;

    Widget* widget = toRenderWidget(renderer)->widget();
    if (!widget || !widget->isFrameView())
        return 0;

    return &toFrameView(widget)->frame();
}

static LocalFrame* subframeForHitTestResult(const MouseEventWithHitTestResults& hitTestResult)
{
    if (!hitTestResult.isOverWidget())
        return 0;
    return subframeForTargetNode(hitTestResult.targetNode());
}

static bool isSubmitImage(Node* node)
{
    return isHTMLInputElement(node) && toHTMLInputElement(node)->isImageButton();
}

bool EventHandler::useHandCursor(Node* node, bool isOverLink)
{
    if (!node)
        return false;

    return ((isOverLink || isSubmitImage(node)) && !node->rendererIsEditable());
}

void EventHandler::cursorUpdateTimerFired(Timer<EventHandler>*)
{
    ASSERT(m_frame);
    ASSERT(m_frame->document());

    updateCursor();
}

void EventHandler::updateCursor()
{
    if (m_mousePositionIsUnknown)
        return;

    FrameView* view = m_frame->view();
    if (!view || !view->shouldSetCursor())
        return;

    RenderView* renderView = view->renderView();
    if (!renderView)
        return;

    m_frame->document()->updateLayout();

    HitTestRequest request(HitTestRequest::ReadOnly);
    HitTestResult result(view->windowToContents(m_lastKnownMousePosition));
    renderView->hitTest(request, result);

    OptionalCursor optionalCursor = selectCursor(result);
    if (optionalCursor.isCursorChange()) {
        m_currentMouseCursor = optionalCursor.cursor();
        view->setCursor(m_currentMouseCursor);
    }
}

OptionalCursor EventHandler::selectCursor(const HitTestResult& result)
{
    if (m_resizeScrollableArea && m_resizeScrollableArea->inResizeMode())
        return NoCursorChange;

    Page* page = m_frame->page();
    if (!page)
        return NoCursorChange;
#if OS(WIN)
    if (panScrollInProgress())
        return NoCursorChange;
#endif

    Node* node = result.innerPossiblyPseudoNode();
    if (!node)
        return selectAutoCursor(result, node, iBeamCursor());

    RenderObject* renderer = node->renderer();
    RenderStyle* style = renderer ? renderer->style() : 0;

    if (renderer) {
        Cursor overrideCursor;
        switch (renderer->getCursor(roundedIntPoint(result.localPoint()), overrideCursor)) {
        case SetCursorBasedOnStyle:
            break;
        case SetCursor:
            return overrideCursor;
        case DoNotSetCursor:
            return NoCursorChange;
        }
    }

    if (style && style->cursors()) {
        const CursorList* cursors = style->cursors();
        for (unsigned i = 0; i < cursors->size(); ++i) {
            StyleImage* styleImage = (*cursors)[i].image();
            if (!styleImage)
                continue;
            ImageResource* cachedImage = styleImage->cachedImage();
            if (!cachedImage)
                continue;
            float scale = styleImage->imageScaleFactor();
            // Get hotspot and convert from logical pixels to physical pixels.
            IntPoint hotSpot = (*cursors)[i].hotSpot();
            hotSpot.scale(scale, scale);
            IntSize size = cachedImage->imageForRenderer(renderer)->size();
            if (cachedImage->errorOccurred())
                continue;
            // Limit the size of cursors (in UI pixels) so that they cannot be
            // used to cover UI elements in chrome.
            size.scale(1 / scale);
            if (size.width() > maximumCursorSize || size.height() > maximumCursorSize)
                continue;

            Image* image = cachedImage->imageForRenderer(renderer);
            // Ensure no overflow possible in calculations above.
            if (scale < minimumCursorScale)
                continue;
            return Cursor(image, hotSpot, scale);
        }
    }

    switch (style ? style->cursor() : CURSOR_AUTO) {
    case CURSOR_AUTO: {
        bool horizontalText = !style || style->isHorizontalWritingMode();
        const Cursor& iBeam = horizontalText ? iBeamCursor() : verticalTextCursor();
        return selectAutoCursor(result, node, iBeam);
    }
    case CURSOR_CROSS:
        return crossCursor();
    case CURSOR_POINTER:
        return handCursor();
    case CURSOR_MOVE:
        return moveCursor();
    case CURSOR_ALL_SCROLL:
        return moveCursor();
    case CURSOR_E_RESIZE:
        return eastResizeCursor();
    case CURSOR_W_RESIZE:
        return westResizeCursor();
    case CURSOR_N_RESIZE:
        return northResizeCursor();
    case CURSOR_S_RESIZE:
        return southResizeCursor();
    case CURSOR_NE_RESIZE:
        return northEastResizeCursor();
    case CURSOR_SW_RESIZE:
        return southWestResizeCursor();
    case CURSOR_NW_RESIZE:
        return northWestResizeCursor();
    case CURSOR_SE_RESIZE:
        return southEastResizeCursor();
    case CURSOR_NS_RESIZE:
        return northSouthResizeCursor();
    case CURSOR_EW_RESIZE:
        return eastWestResizeCursor();
    case CURSOR_NESW_RESIZE:
        return northEastSouthWestResizeCursor();
    case CURSOR_NWSE_RESIZE:
        return northWestSouthEastResizeCursor();
    case CURSOR_COL_RESIZE:
        return columnResizeCursor();
    case CURSOR_ROW_RESIZE:
        return rowResizeCursor();
    case CURSOR_TEXT:
        return iBeamCursor();
    case CURSOR_WAIT:
        return waitCursor();
    case CURSOR_HELP:
        return helpCursor();
    case CURSOR_VERTICAL_TEXT:
        return verticalTextCursor();
    case CURSOR_CELL:
        return cellCursor();
    case CURSOR_CONTEXT_MENU:
        return contextMenuCursor();
    case CURSOR_PROGRESS:
        return progressCursor();
    case CURSOR_NO_DROP:
        return noDropCursor();
    case CURSOR_ALIAS:
        return aliasCursor();
    case CURSOR_COPY:
        return copyCursor();
    case CURSOR_NONE:
        return noneCursor();
    case CURSOR_NOT_ALLOWED:
        return notAllowedCursor();
    case CURSOR_DEFAULT:
        return pointerCursor();
    case CURSOR_ZOOM_IN:
        return zoomInCursor();
    case CURSOR_ZOOM_OUT:
        return zoomOutCursor();
    case CURSOR_WEBKIT_GRAB:
        return grabCursor();
    case CURSOR_WEBKIT_GRABBING:
        return grabbingCursor();
    }
    return pointerCursor();
}

OptionalCursor EventHandler::selectAutoCursor(const HitTestResult& result, Node* node, const Cursor& iBeam)
{
    bool editable = (node && node->rendererIsEditable());

    if (useHandCursor(node, result.isOverLink()))
        return handCursor();

    bool inResizer = false;
    RenderObject* renderer = node ? node->renderer() : 0;
    if (renderer && m_frame->view()) {
        RenderLayer* layer = renderer->enclosingLayer();
        inResizer = layer->scrollableArea() && layer->scrollableArea()->isPointInResizeControl(result.roundedPointInMainFrame(), ResizerForPointer);
    }

    // During selection, use an I-beam no matter what we're over.
    // If a drag may be starting or we're capturing mouse events for a particular node, don't treat this as a selection.
    if (m_mousePressed && m_mouseDownMayStartSelect
        && !m_mouseDownMayStartDrag
        && m_frame->selection().isCaretOrRange()
        && !m_capturingMouseEventsNode) {
        return iBeam;
    }

    if ((editable || (renderer && renderer->isText() && node->canStartSelection())) && !inResizer && !result.scrollbar())
        return iBeam;
    return pointerCursor();
}

static LayoutPoint documentPointForWindowPoint(LocalFrame* frame, const IntPoint& windowPoint)
{
    FrameView* view = frame->view();
    // FIXME: Is it really OK to use the wrong coordinates here when view is 0?
    // Historically the code would just crash; this is clearly no worse than that.
    return view ? view->windowToContents(windowPoint) : windowPoint;
}

bool EventHandler::handleMousePressEvent(const PlatformMouseEvent& mouseEvent)
{
    TRACE_EVENT0("webkit", "EventHandler::handleMousePressEvent");

    RefPtr<FrameView> protector(m_frame->view());

    UserGestureIndicator gestureIndicator(DefinitelyProcessingUserGesture);
    m_frame->localFrameRoot()->eventHandler().m_lastMouseDownUserGestureToken = gestureIndicator.currentToken();

    cancelFakeMouseMoveEvent();
    if (m_eventHandlerWillResetCapturingMouseEventsNode)
        m_capturingMouseEventsNode = nullptr;
    m_mousePressed = true;
    m_capturesDragging = true;
    setLastKnownMousePosition(mouseEvent);
    m_mouseDownTimestamp = mouseEvent.timestamp();
    m_mouseDownMayStartDrag = false;
    m_mouseDownMayStartSelect = false;
    m_mouseDownMayStartAutoscroll = false;
    if (FrameView* view = m_frame->view())
        m_mouseDownPos = view->windowToContents(mouseEvent.position());
    else {
        invalidateClick();
        return false;
    }
    m_mouseDownWasInSubframe = false;

    HitTestRequest::HitTestRequestType hitType = HitTestRequest::Active | HitTestRequest::ConfusingAndOftenMisusedDisallowShadowContent;
    if (mouseEvent.fromTouch())
        hitType |= HitTestRequest::ReadOnly;
    HitTestRequest request(hitType);
    // Save the document point we generate in case the window coordinate is invalidated by what happens
    // when we dispatch the event.
    LayoutPoint documentPoint = documentPointForWindowPoint(m_frame, mouseEvent.position());
    MouseEventWithHitTestResults mev = m_frame->document()->prepareMouseEvent(request, documentPoint, mouseEvent);

    if (!mev.targetNode()) {
        invalidateClick();
        return false;
    }

    m_mousePressNode = mev.targetNode();

    RefPtr<LocalFrame> subframe = subframeForHitTestResult(mev);
    if (subframe && passMousePressEventToSubframe(mev, subframe.get())) {
        // Start capturing future events for this frame.  We only do this if we didn't clear
        // the m_mousePressed flag, which may happen if an AppKit widget entered a modal event loop.
        m_capturesDragging = subframe->eventHandler().capturesDragging();
        if (m_mousePressed && m_capturesDragging) {
            m_capturingMouseEventsNode = mev.targetNode();
            m_eventHandlerWillResetCapturingMouseEventsNode = true;
        }
        invalidateClick();
        return true;
    }

#if OS(WIN)
    // We store whether pan scrolling is in progress before calling stopAutoscroll()
    // because it will set m_autoscrollType to NoAutoscroll on return.
    bool isPanScrollInProgress = panScrollInProgress();
    stopAutoscroll();
    if (isPanScrollInProgress) {
        // We invalidate the click when exiting pan scrolling so that we don't inadvertently navigate
        // away from the current page (e.g. the click was on a hyperlink). See <rdar://problem/6095023>.
        invalidateClick();
        return true;
    }
#endif

    m_clickCount = mouseEvent.clickCount();
    m_clickNode = mev.targetNode()->isTextNode() ?  NodeRenderingTraversal::parent(mev.targetNode()) : mev.targetNode();

    if (FrameView* view = m_frame->view()) {
        RenderLayer* layer = mev.targetNode()->renderer() ? mev.targetNode()->renderer()->enclosingLayer() : 0;
        IntPoint p = view->windowToContents(mouseEvent.position());
        if (layer && layer->scrollableArea() && layer->scrollableArea()->isPointInResizeControl(p, ResizerForPointer)) {
            m_resizeScrollableArea = layer->scrollableArea();
            m_resizeScrollableArea->setInResizeMode(true);
            m_offsetFromResizeCorner = m_resizeScrollableArea->offsetFromResizeCorner(p);
            invalidateClick();
            return true;
        }
    }

    m_frame->selection().setCaretBlinkingSuspended(true);

    bool swallowEvent = !dispatchMouseEvent(EventTypeNames::mousedown, mev.targetNode(), m_clickCount, mouseEvent, true);
    swallowEvent = swallowEvent || !handleMouseFocus(mouseEvent);
    m_capturesDragging = !swallowEvent || mev.scrollbar();

    // If the hit testing originally determined the event was in a scrollbar, refetch the MouseEventWithHitTestResults
    // in case the scrollbar widget was destroyed when the mouse event was handled.
    if (mev.scrollbar()) {
        const bool wasLastScrollBar = mev.scrollbar() == m_lastScrollbarUnderMouse.get();
        HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::ConfusingAndOftenMisusedDisallowShadowContent);
        mev = m_frame->document()->prepareMouseEvent(request, documentPoint, mouseEvent);
        if (wasLastScrollBar && mev.scrollbar() != m_lastScrollbarUnderMouse.get())
            m_lastScrollbarUnderMouse = nullptr;
    }

    if (swallowEvent) {
        // scrollbars should get events anyway, even disabled controls might be scrollable
        Scrollbar* scrollbar = mev.scrollbar();

        updateLastScrollbarUnderMouse(scrollbar, true);

        if (scrollbar)
            passMousePressEventToScrollbar(mev, scrollbar);
    } else {
        if (shouldRefetchEventTarget(mev)) {
            HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::ConfusingAndOftenMisusedDisallowShadowContent);
            mev = m_frame->document()->prepareMouseEvent(request, documentPoint, mouseEvent);
        }

        FrameView* view = m_frame->view();
        Scrollbar* scrollbar = view ? view->scrollbarAtPoint(mouseEvent.position()) : 0;
        if (!scrollbar)
            scrollbar = mev.scrollbar();

        updateLastScrollbarUnderMouse(scrollbar, true);

        if (scrollbar && passMousePressEventToScrollbar(mev, scrollbar))
            swallowEvent = true;
        else
            swallowEvent = handleMousePressEvent(mev);
    }

    return swallowEvent;
}

static RenderLayer* layerForNode(Node* node)
{
    if (!node)
        return 0;

    RenderObject* renderer = node->renderer();
    if (!renderer)
        return 0;

    RenderLayer* layer = renderer->enclosingLayer();
    if (!layer)
        return 0;

    return layer;
}

ScrollableArea* EventHandler::associatedScrollableArea(const RenderLayer* layer) const
{
    if (RenderLayerScrollableArea* scrollableArea = layer->scrollableArea()) {
        if (scrollableArea->scrollsOverflow())
            return scrollableArea;
    }

    return 0;
}

bool EventHandler::handleMouseMoveEvent(const PlatformMouseEvent& event)
{
    TRACE_EVENT0("webkit", "EventHandler::handleMouseMoveEvent");

    RefPtr<FrameView> protector(m_frame->view());
    MaximumDurationTracker maxDurationTracker(&m_maxMouseMovedDuration);

    HitTestResult hoveredNode = HitTestResult(LayoutPoint());
    bool result = handleMouseMoveOrLeaveEvent(event, &hoveredNode);

    Page* page = m_frame->page();
    if (!page)
        return result;

    if (RenderLayer* layer = layerForNode(hoveredNode.innerNode())) {
        if (ScrollableArea* layerScrollableArea = associatedScrollableArea(layer))
            layerScrollableArea->mouseMovedInContentArea();
    }

    if (FrameView* frameView = m_frame->view())
        frameView->mouseMovedInContentArea();

    hoveredNode.setToShadowHostIfInUserAgentShadowRoot();
    page->chrome().mouseDidMoveOverElement(hoveredNode, event.modifierFlags());
    page->chrome().setToolTip(hoveredNode);

    return result;
}

void EventHandler::handleMouseLeaveEvent(const PlatformMouseEvent& event)
{
    TRACE_EVENT0("webkit", "EventHandler::handleMouseLeaveEvent");

    RefPtr<FrameView> protector(m_frame->view());
    handleMouseMoveOrLeaveEvent(event);
}

bool EventHandler::handleMouseMoveOrLeaveEvent(const PlatformMouseEvent& mouseEvent, HitTestResult* hoveredNode, bool onlyUpdateScrollbars)
{
    ASSERT(m_frame);
    ASSERT(m_frame->view());

    setLastKnownMousePosition(mouseEvent);

    if (m_hoverTimer.isActive())
        m_hoverTimer.stop();

    m_cursorUpdateTimer.stop();

    cancelFakeMouseMoveEvent();

    if (m_svgPan) {
        m_frame->document()->accessSVGExtensions().updatePan(m_frame->view()->windowToContents(m_lastKnownMousePosition));
        return true;
    }

    if (m_frameSetBeingResized)
        return !dispatchMouseEvent(EventTypeNames::mousemove, m_frameSetBeingResized.get(), 0, mouseEvent, false);

    // Send events right to a scrollbar if the mouse is pressed.
    if (m_lastScrollbarUnderMouse && m_mousePressed) {
        m_lastScrollbarUnderMouse->mouseMoved(mouseEvent);
        return true;
    }

    HitTestRequest::HitTestRequestType hitType = HitTestRequest::Move | HitTestRequest::ConfusingAndOftenMisusedDisallowShadowContent;
    if (mouseEvent.fromTouch())
        hitType |= HitTestRequest::ReadOnly;

    if (m_mousePressed)
        hitType |= HitTestRequest::Active;
    else if (onlyUpdateScrollbars) {
        // Mouse events should be treated as "read-only" if we're updating only scrollbars. This
        // means that :hover and :active freeze in the state they were in, rather than updating
        // for nodes the mouse moves while the window is not key (which will be the case if
        // onlyUpdateScrollbars is true).
        hitType |= HitTestRequest::ReadOnly;
    }

    // Treat any mouse move events as readonly if the user is currently touching the screen.
    if (m_touchPressed)
        hitType |= HitTestRequest::Active | HitTestRequest::ReadOnly;
    HitTestRequest request(hitType);
    MouseEventWithHitTestResults mev = prepareMouseEvent(request, mouseEvent);
    if (hoveredNode)
        *hoveredNode = mev.hitTestResult();

    Scrollbar* scrollbar = 0;

    if (m_resizeScrollableArea && m_resizeScrollableArea->inResizeMode())
        m_resizeScrollableArea->resize(mouseEvent, m_offsetFromResizeCorner);
    else {
        if (FrameView* view = m_frame->view())
            scrollbar = view->scrollbarAtPoint(mouseEvent.position());

        if (!scrollbar)
            scrollbar = mev.scrollbar();

        updateLastScrollbarUnderMouse(scrollbar, !m_mousePressed);
        if (onlyUpdateScrollbars)
            return true;
    }

    bool swallowEvent = false;
    RefPtr<LocalFrame> newSubframe = m_capturingMouseEventsNode.get() ? subframeForTargetNode(m_capturingMouseEventsNode.get()) : subframeForHitTestResult(mev);

    // We want mouseouts to happen first, from the inside out.  First send a move event to the last subframe so that it will fire mouseouts.
    if (m_lastMouseMoveEventSubframe && m_lastMouseMoveEventSubframe->tree().isDescendantOf(m_frame) && m_lastMouseMoveEventSubframe != newSubframe)
        passMouseMoveEventToSubframe(mev, m_lastMouseMoveEventSubframe.get());

    if (newSubframe) {
        // Update over/out state before passing the event to the subframe.
        updateMouseEventTargetNode(mev.targetNode(), mouseEvent, true);

        // Event dispatch in updateMouseEventTargetNode may have caused the subframe of the target
        // node to be detached from its FrameView, in which case the event should not be passed.
        if (newSubframe->view())
            swallowEvent |= passMouseMoveEventToSubframe(mev, newSubframe.get(), hoveredNode);
    } else {
        if (scrollbar && !m_mousePressed)
            scrollbar->mouseMoved(mouseEvent); // Handle hover effects on platforms that support visual feedback on scrollbar hovering.
        if (FrameView* view = m_frame->view()) {
            OptionalCursor optionalCursor = selectCursor(mev.hitTestResult());
            if (optionalCursor.isCursorChange()) {
                m_currentMouseCursor = optionalCursor.cursor();
                view->setCursor(m_currentMouseCursor);
            }
        }
    }

    m_lastMouseMoveEventSubframe = newSubframe;

    if (swallowEvent)
        return true;

    swallowEvent = !dispatchMouseEvent(EventTypeNames::mousemove, mev.targetNode(), 0, mouseEvent, true);
    if (!swallowEvent)
        swallowEvent = handleMouseDraggedEvent(mev);

    return swallowEvent;
}

void EventHandler::invalidateClick()
{
    m_clickCount = 0;
    m_clickNode = nullptr;
}

static Node* parentForClickEvent(const Node& node)
{
    // IE doesn't dispatch click events for mousedown/mouseup events across form
    // controls.
    if (node.isHTMLElement() && toHTMLElement(node).isInteractiveContent())
        return 0;
    return NodeRenderingTraversal::parent(&node);
}

bool EventHandler::handleMouseReleaseEvent(const PlatformMouseEvent& mouseEvent)
{
    TRACE_EVENT0("webkit", "EventHandler::handleMouseReleaseEvent");

    RefPtr<FrameView> protector(m_frame->view());

    m_frame->selection().setCaretBlinkingSuspended(false);

    OwnPtr<UserGestureIndicator> gestureIndicator;

    if (m_frame->localFrameRoot()->eventHandler().m_lastMouseDownUserGestureToken)
        gestureIndicator = adoptPtr(new UserGestureIndicator(m_frame->tree().top()->eventHandler().m_lastMouseDownUserGestureToken.release()));
    else
        gestureIndicator = adoptPtr(new UserGestureIndicator(DefinitelyProcessingUserGesture));

#if OS(WIN)
    if (Page* page = m_frame->page())
        page->autoscrollController().handleMouseReleaseForPanScrolling(m_frame, mouseEvent);
#endif

    m_mousePressed = false;
    setLastKnownMousePosition(mouseEvent);

    if (m_svgPan) {
        m_svgPan = false;
        m_frame->document()->accessSVGExtensions().updatePan(m_frame->view()->windowToContents(m_lastKnownMousePosition));
        return true;
    }

    if (m_frameSetBeingResized)
        return !dispatchMouseEvent(EventTypeNames::mouseup, m_frameSetBeingResized.get(), m_clickCount, mouseEvent, false);

    if (m_lastScrollbarUnderMouse) {
        invalidateClick();
        m_lastScrollbarUnderMouse->mouseUp(mouseEvent);
        bool setUnder = false;
        return !dispatchMouseEvent(EventTypeNames::mouseup, m_lastNodeUnderMouse.get(), m_clickCount, mouseEvent, setUnder);
    }

    HitTestRequest::HitTestRequestType hitType = HitTestRequest::Release | HitTestRequest::ConfusingAndOftenMisusedDisallowShadowContent;
    if (mouseEvent.fromTouch())
        hitType |= HitTestRequest::ReadOnly;
    HitTestRequest request(hitType);
    MouseEventWithHitTestResults mev = prepareMouseEvent(request, mouseEvent);
    LocalFrame* subframe = m_capturingMouseEventsNode.get() ? subframeForTargetNode(m_capturingMouseEventsNode.get()) : subframeForHitTestResult(mev);
    if (m_eventHandlerWillResetCapturingMouseEventsNode)
        m_capturingMouseEventsNode = nullptr;
    if (subframe && passMouseReleaseEventToSubframe(mev, subframe))
        return true;

    bool swallowMouseUpEvent = !dispatchMouseEvent(EventTypeNames::mouseup, mev.targetNode(), m_clickCount, mouseEvent, false);

    bool contextMenuEvent = mouseEvent.button() == RightButton;
#if OS(MACOSX)
    // FIXME: The Mac port achieves the same behavior by checking whether the context menu is currently open in WebPage::mouseEvent(). Consider merging the implementations.
    if (mouseEvent.button() == LeftButton && mouseEvent.modifiers() & PlatformEvent::CtrlKey)
        contextMenuEvent = true;
#endif

    bool swallowClickEvent = false;
    if (m_clickCount > 0 && !contextMenuEvent && mev.targetNode() && m_clickNode) {
        if (Node* clickTargetNode = mev.targetNode()->commonAncestor(*m_clickNode, parentForClickEvent))
            swallowClickEvent = !dispatchMouseEvent(EventTypeNames::click, clickTargetNode, m_clickCount, mouseEvent, true);
    }

    if (m_resizeScrollableArea) {
        m_resizeScrollableArea->setInResizeMode(false);
        m_resizeScrollableArea = 0;
    }

    bool swallowMouseReleaseEvent = false;
    if (!swallowMouseUpEvent)
        swallowMouseReleaseEvent = handleMouseReleaseEvent(mev);

    invalidateClick();

    return swallowMouseUpEvent || swallowClickEvent || swallowMouseReleaseEvent;
}

bool EventHandler::handlePasteGlobalSelection(const PlatformMouseEvent& mouseEvent)
{
    // If the event was a middle click, attempt to copy global selection in after
    // the newly set caret position.
    //
    // This code is called from either the mouse up or mouse down handling. There
    // is some debate about when the global selection is pasted:
    //   xterm: pastes on up.
    //   GTK: pastes on down.
    //   Qt: pastes on up.
    //   Firefox: pastes on up.
    //   Chromium: pastes on up.
    //
    // There is something of a webcompat angle to this well, as highlighted by
    // crbug.com/14608. Pages can clear text boxes 'onclick' and, if we paste on
    // down then the text is pasted just before the onclick handler runs and
    // clears the text box. So it's important this happens after the event
    // handlers have been fired.
    if (mouseEvent.type() != PlatformEvent::MouseReleased)
        return false;

    if (!m_frame->page())
        return false;
    Frame* focusFrame = m_frame->page()->focusController().focusedOrMainFrame();
    // Do not paste here if the focus was moved somewhere else.
    if (m_frame == focusFrame && m_frame->editor().behavior().supportsGlobalSelection())
        return m_frame->editor().command("PasteGlobalSelection").execute();

    return false;
}


bool EventHandler::dispatchDragEvent(const AtomicString& eventType, Node* dragTarget, const PlatformMouseEvent& event, Clipboard* clipboard)
{
    FrameView* view = m_frame->view();

    // FIXME: We might want to dispatch a dragleave even if the view is gone.
    if (!view)
        return false;

    RefPtrWillBeRawPtr<MouseEvent> me = MouseEvent::create(eventType,
        true, true, m_frame->document()->domWindow(),
        0, event.globalPosition().x(), event.globalPosition().y(), event.position().x(), event.position().y(),
        event.movementDelta().x(), event.movementDelta().y(),
        event.ctrlKey(), event.altKey(), event.shiftKey(), event.metaKey(),
        0, nullptr, clipboard);

    dragTarget->dispatchEvent(me.get(), IGNORE_EXCEPTION);
    return me->defaultPrevented();
}

static bool targetIsFrame(Node* target, LocalFrame*& frame)
{
    if (!isHTMLFrameElementBase(target))
        return false;

    // Cross-process drag and drop is not yet supported.
    if (toHTMLFrameElementBase(target)->contentFrame() && !toHTMLFrameElementBase(target)->contentFrame()->isLocalFrame())
        return false;

    frame = toLocalFrame(toHTMLFrameElementBase(target)->contentFrame());
    return true;
}

static bool findDropZone(Node* target, Clipboard* clipboard)
{
    Element* element = target->isElementNode() ? toElement(target) : target->parentElement();
    for (; element; element = element->parentElement()) {
        bool matched = false;
        AtomicString dropZoneStr = element->fastGetAttribute(webkitdropzoneAttr);

        if (dropZoneStr.isEmpty())
            continue;

        dropZoneStr = dropZoneStr.lower();

        SpaceSplitString keywords(dropZoneStr, false);
        if (keywords.isNull())
            continue;

        DragOperation dragOperation = DragOperationNone;
        for (unsigned i = 0; i < keywords.size(); i++) {
            DragOperation op = convertDropZoneOperationToDragOperation(keywords[i]);
            if (op != DragOperationNone) {
                if (dragOperation == DragOperationNone)
                    dragOperation = op;
            } else
                matched = matched || clipboard->hasDropZoneType(keywords[i].string());

            if (matched && dragOperation != DragOperationNone)
                break;
        }
        if (matched) {
            clipboard->setDropEffect(convertDragOperationToDropZoneOperation(dragOperation));
            return true;
        }
    }
    return false;
}

bool EventHandler::updateDragAndDrop(const PlatformMouseEvent& event, Clipboard* clipboard)
{
    bool accept = false;

    if (!m_frame->view())
        return false;

    HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::ConfusingAndOftenMisusedDisallowShadowContent);
    MouseEventWithHitTestResults mev = prepareMouseEvent(request, event);

    // Drag events should never go to text nodes (following IE, and proper mouseover/out dispatch)
    RefPtrWillBeRawPtr<Node> newTarget = mev.targetNode();
    if (newTarget && newTarget->isTextNode())
        newTarget = NodeRenderingTraversal::parent(newTarget.get());

    if (AutoscrollController* controller = autoscrollController())
        controller->updateDragAndDrop(newTarget.get(), event.position(), event.timestamp());

    if (m_dragTarget != newTarget) {
        // FIXME: this ordering was explicitly chosen to match WinIE. However,
        // it is sometimes incorrect when dragging within subframes, as seen with
        // LayoutTests/fast/events/drag-in-frames.html.
        //
        // Moreover, this ordering conforms to section 7.9.4 of the HTML 5 spec. <http://dev.w3.org/html5/spec/Overview.html#drag-and-drop-processing-model>.
        LocalFrame* targetFrame;
        if (targetIsFrame(newTarget.get(), targetFrame)) {
            if (targetFrame)
                accept = targetFrame->eventHandler().updateDragAndDrop(event, clipboard);
        } else if (newTarget) {
            // As per section 7.9.4 of the HTML 5 spec., we must always fire a drag event before firing a dragenter, dragleave, or dragover event.
            if (dragState().m_dragSrc) {
                // for now we don't care if event handler cancels default behavior, since there is none
                dispatchDragSrcEvent(EventTypeNames::drag, event);
            }
            accept = dispatchDragEvent(EventTypeNames::dragenter, newTarget.get(), event, clipboard);
            if (!accept)
                accept = findDropZone(newTarget.get(), clipboard);
        }

        if (targetIsFrame(m_dragTarget.get(), targetFrame)) {
            if (targetFrame)
                accept = targetFrame->eventHandler().updateDragAndDrop(event, clipboard);
        } else if (m_dragTarget)
            dispatchDragEvent(EventTypeNames::dragleave, m_dragTarget.get(), event, clipboard);

        if (newTarget) {
            // We do not explicitly call dispatchDragEvent here because it could ultimately result in the appearance that
            // two dragover events fired. So, we mark that we should only fire a dragover event on the next call to this function.
            m_shouldOnlyFireDragOverEvent = true;
        }
    } else {
        LocalFrame* targetFrame;
        if (targetIsFrame(newTarget.get(), targetFrame)) {
            if (targetFrame)
                accept = targetFrame->eventHandler().updateDragAndDrop(event, clipboard);
        } else if (newTarget) {
            // Note, when dealing with sub-frames, we may need to fire only a dragover event as a drag event may have been fired earlier.
            if (!m_shouldOnlyFireDragOverEvent && dragState().m_dragSrc) {
                // for now we don't care if event handler cancels default behavior, since there is none
                dispatchDragSrcEvent(EventTypeNames::drag, event);
            }
            accept = dispatchDragEvent(EventTypeNames::dragover, newTarget.get(), event, clipboard);
            if (!accept)
                accept = findDropZone(newTarget.get(), clipboard);
            m_shouldOnlyFireDragOverEvent = false;
        }
    }
    m_dragTarget = newTarget;

    return accept;
}

void EventHandler::cancelDragAndDrop(const PlatformMouseEvent& event, Clipboard* clipboard)
{
    LocalFrame* targetFrame;
    if (targetIsFrame(m_dragTarget.get(), targetFrame)) {
        if (targetFrame)
            targetFrame->eventHandler().cancelDragAndDrop(event, clipboard);
    } else if (m_dragTarget.get()) {
        if (dragState().m_dragSrc)
            dispatchDragSrcEvent(EventTypeNames::drag, event);
        dispatchDragEvent(EventTypeNames::dragleave, m_dragTarget.get(), event, clipboard);
    }
    clearDragState();
}

bool EventHandler::performDragAndDrop(const PlatformMouseEvent& event, Clipboard* clipboard)
{
    LocalFrame* targetFrame;
    bool preventedDefault = false;
    if (targetIsFrame(m_dragTarget.get(), targetFrame)) {
        if (targetFrame)
            preventedDefault = targetFrame->eventHandler().performDragAndDrop(event, clipboard);
    } else if (m_dragTarget.get())
        preventedDefault = dispatchDragEvent(EventTypeNames::drop, m_dragTarget.get(), event, clipboard);
    clearDragState();
    return preventedDefault;
}

void EventHandler::clearDragState()
{
    stopAutoscroll();
    m_dragTarget = nullptr;
    m_capturingMouseEventsNode = nullptr;
    m_shouldOnlyFireDragOverEvent = false;
}

void EventHandler::setCapturingMouseEventsNode(PassRefPtrWillBeRawPtr<Node> n)
{
    m_capturingMouseEventsNode = n;
    m_eventHandlerWillResetCapturingMouseEventsNode = false;
}

MouseEventWithHitTestResults EventHandler::prepareMouseEvent(const HitTestRequest& request, const PlatformMouseEvent& mev)
{
    ASSERT(m_frame);
    ASSERT(m_frame->document());

    return m_frame->document()->prepareMouseEvent(request, documentPointForWindowPoint(m_frame, mev.position()), mev);
}

void EventHandler::updateMouseEventTargetNode(Node* targetNode, const PlatformMouseEvent& mouseEvent, bool fireMouseOverOut)
{
    Node* result = targetNode;

    // If we're capturing, we always go right to that node.
    if (m_capturingMouseEventsNode)
        result = m_capturingMouseEventsNode.get();
    else {
        // If the target node is a text node, dispatch on the parent node - rdar://4196646
        if (result && result->isTextNode())
            result = NodeRenderingTraversal::parent(result);
    }
    m_nodeUnderMouse = result;

    // Fire mouseout/mouseover if the mouse has shifted to a different node.
    if (fireMouseOverOut) {
        RenderLayer* layerForLastNode = layerForNode(m_lastNodeUnderMouse.get());
        RenderLayer* layerForNodeUnderMouse = layerForNode(m_nodeUnderMouse.get());
        Page* page = m_frame->page();

        if (m_lastNodeUnderMouse && (!m_nodeUnderMouse || m_nodeUnderMouse->document() != m_frame->document())) {
            // The mouse has moved between frames.
            if (LocalFrame* frame = m_lastNodeUnderMouse->document().frame()) {
                if (FrameView* frameView = frame->view())
                    frameView->mouseExitedContentArea();
            }
        } else if (page && (layerForLastNode && (!layerForNodeUnderMouse || layerForNodeUnderMouse != layerForLastNode))) {
            // The mouse has moved between layers.
            if (ScrollableArea* scrollableAreaForLastNode = associatedScrollableArea(layerForLastNode))
                scrollableAreaForLastNode->mouseExitedContentArea();
        }

        if (m_nodeUnderMouse && (!m_lastNodeUnderMouse || m_lastNodeUnderMouse->document() != m_frame->document())) {
            // The mouse has moved between frames.
            if (LocalFrame* frame = m_nodeUnderMouse->document().frame()) {
                if (FrameView* frameView = frame->view())
                    frameView->mouseEnteredContentArea();
            }
        } else if (page && (layerForNodeUnderMouse && (!layerForLastNode || layerForNodeUnderMouse != layerForLastNode))) {
            // The mouse has moved between layers.
            if (ScrollableArea* scrollableAreaForNodeUnderMouse = associatedScrollableArea(layerForNodeUnderMouse))
                scrollableAreaForNodeUnderMouse->mouseEnteredContentArea();
        }

        if (m_lastNodeUnderMouse && m_lastNodeUnderMouse->document() != m_frame->document()) {
            m_lastNodeUnderMouse = nullptr;
            m_lastScrollbarUnderMouse = nullptr;
        }

        if (m_lastNodeUnderMouse != m_nodeUnderMouse) {
            // send mouseout event to the old node
            if (m_lastNodeUnderMouse)
                m_lastNodeUnderMouse->dispatchMouseEvent(mouseEvent, EventTypeNames::mouseout, 0, m_nodeUnderMouse.get());
            // send mouseover event to the new node
            if (m_nodeUnderMouse)
                m_nodeUnderMouse->dispatchMouseEvent(mouseEvent, EventTypeNames::mouseover, 0, m_lastNodeUnderMouse.get());
        }
        m_lastNodeUnderMouse = m_nodeUnderMouse;
    }
}

// The return value means 'continue default handling.'
bool EventHandler::dispatchMouseEvent(const AtomicString& eventType, Node* targetNode, int clickCount, const PlatformMouseEvent& mouseEvent, bool setUnder)
{
    updateMouseEventTargetNode(targetNode, mouseEvent, setUnder);
    return !m_nodeUnderMouse || m_nodeUnderMouse->dispatchMouseEvent(mouseEvent, eventType, clickCount);
}

// The return value means 'continue default handling.'
bool EventHandler::handleMouseFocus(const PlatformMouseEvent& mouseEvent)
{
    // If clicking on a frame scrollbar, do not mess up with content focus.
    if (FrameView* view = m_frame->view()) {
        if (view->scrollbarAtPoint(mouseEvent.position()))
            return true;
    }

    // The layout needs to be up to date to determine if an element is focusable.
    m_frame->document()->updateLayoutIgnorePendingStylesheets();

    Element* element = 0;
    if (m_nodeUnderMouse)
        element = m_nodeUnderMouse->isElementNode() ? toElement(m_nodeUnderMouse) : m_nodeUnderMouse->parentOrShadowHostElement();
    for (; element; element = element->parentOrShadowHostElement()) {
        if (element->isFocusable() && element->focused())
            return true;
        if (element->isMouseFocusable())
            break;
    }
    ASSERT(!element || element->isMouseFocusable());

    // To fix <rdar://problem/4895428> Can't drag selected ToDo, we don't focus
    // a node on mouse down if it's selected and inside a focused node. It will
    // be focused if the user does a mouseup over it, however, because the
    // mouseup will set a selection inside it, which will call
    // FrameSelection::setFocusedNodeIfNeeded.
    if (element
        && m_frame->selection().isRange()
        && m_frame->selection().toNormalizedRange()->compareNode(element, IGNORE_EXCEPTION) == Range::NODE_INSIDE
        && element->isDescendantOf(m_frame->document()->focusedElement()))
        return true;

    // Only change the focus when clicking scrollbars if it can transfered to a
    // mouse focusable node.
    if (!element && isInsideScrollbar(mouseEvent.position()))
        return false;

    if (Page* page = m_frame->page()) {
        // If focus shift is blocked, we eat the event. Note we should never
        // clear swallowEvent if the page already set it (e.g., by canceling
        // default behavior).
        if (element) {
            if (!page->focusController().setFocusedElement(element, m_frame, FocusTypeMouse))
                return false;
        } else {
            // We call setFocusedElement even with !element in order to blur
            // current focus element when a link is clicked; this is expected by
            // some sites that rely on onChange handlers running from form
            // fields before the button click is processed.
            if (!page->focusController().setFocusedElement(0, m_frame))
                return false;
        }
    }

    return true;
}

bool EventHandler::isInsideScrollbar(const IntPoint& windowPoint) const
{
    if (RenderView* renderView = m_frame->contentRenderer()) {
        HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::ConfusingAndOftenMisusedDisallowShadowContent);
        HitTestResult result(windowPoint);
        renderView->hitTest(request, result);
        return result.scrollbar();
    }

    return false;
}

bool EventHandler::handleWheelEvent(const PlatformWheelEvent& event)
{
#define RETURN_WHEEL_EVENT_HANDLED() \
    { \
        setFrameWasScrolledByUser(); \
        return true; \
    }

    Document* doc = m_frame->document();

    if (!doc->renderView())
        return false;

    RefPtr<FrameView> protector(m_frame->view());

    FrameView* view = m_frame->view();
    if (!view)
        return false;

    LayoutPoint vPoint = view->windowToContents(event.position());

    HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::ConfusingAndOftenMisusedDisallowShadowContent);
    HitTestResult result(vPoint);
    doc->renderView()->hitTest(request, result);

    Node* node = result.innerNode();
    // Wheel events should not dispatch to text nodes.
    if (node && node->isTextNode())
        node = NodeRenderingTraversal::parent(node);

    bool isOverWidget;
    if (event.useLatchedEventNode()) {
        if (!m_latchedWheelEventNode) {
            m_latchedWheelEventNode = node;
            m_widgetIsLatched = result.isOverWidget();
        } else
            node = m_latchedWheelEventNode.get();

        isOverWidget = m_widgetIsLatched;
    } else {
        if (m_latchedWheelEventNode)
            m_latchedWheelEventNode = nullptr;
        if (m_previousWheelScrolledNode)
            m_previousWheelScrolledNode = nullptr;

        isOverWidget = result.isOverWidget();
    }

    if (node) {
        // Figure out which view to send the event to.
        RenderObject* target = node->renderer();

        if (isOverWidget && target && target->isWidget()) {
            Widget* widget = toRenderWidget(target)->widget();
            if (widget && passWheelEventToWidget(event, widget))
                RETURN_WHEEL_EVENT_HANDLED();
        }

        if (node && !node->dispatchWheelEvent(event))
            RETURN_WHEEL_EVENT_HANDLED();
    }

    // We do another check on the frame view because the event handler can run JS which results in the frame getting destroyed.
    view = m_frame->view();
    if (!view || !view->wheelEvent(event))
        return false;

    RETURN_WHEEL_EVENT_HANDLED();

#undef RETURN_WHEEL_EVENT_HANDLED
}

void EventHandler::defaultWheelEventHandler(Node* startNode, WheelEvent* wheelEvent)
{
    if (!startNode || !wheelEvent)
        return;

    // Ctrl + scrollwheel is reserved for triggering zoom in/out actions in Chromium.
    if (wheelEvent->ctrlKey())
        return;

    Node* stopNode = m_previousWheelScrolledNode.get();
    ScrollGranularity granularity = wheelGranularityToScrollGranularity(wheelEvent->deltaMode());

    // Break up into two scrolls if we need to.  Diagonal movement on
    // a MacBook pro is an example of a 2-dimensional mouse wheel event (where both deltaX and deltaY can be set).
    if (scroll(ScrollRight, granularity, startNode, &stopNode, wheelEvent->deltaX(), roundedIntPoint(wheelEvent->absoluteLocation())))
        wheelEvent->setDefaultHandled();

    if (scroll(ScrollDown, granularity, startNode, &stopNode, wheelEvent->deltaY(), roundedIntPoint(wheelEvent->absoluteLocation())))
        wheelEvent->setDefaultHandled();

    if (!m_latchedWheelEventNode)
        m_previousWheelScrolledNode = stopNode;
}

bool EventHandler::handleGestureShowPress()
{
    m_lastShowPressTimestamp = WTF::currentTime();

    FrameView* view = m_frame->view();
    if (!view)
        return false;
    if (ScrollAnimator* scrollAnimator = view->existingScrollAnimator())
        scrollAnimator->cancelAnimations();
    const FrameView::ScrollableAreaSet* areas = view->scrollableAreas();
    if (!areas)
        return false;
    for (FrameView::ScrollableAreaSet::const_iterator it = areas->begin(); it != areas->end(); ++it) {
        ScrollableArea* sa = *it;
        ScrollAnimator* animator = sa->scrollAnimator();
        if (animator)
            animator->cancelAnimations();
    }
    return false;
}

bool EventHandler::handleGestureEvent(const PlatformGestureEvent& gestureEvent)
{
    IntPoint adjustedPoint = gestureEvent.position();
    RefPtr<LocalFrame> subframe = nullptr;
    switch (gestureEvent.type()) {
    case PlatformEvent::GestureScrollBegin:
    case PlatformEvent::GestureScrollUpdate:
    case PlatformEvent::GestureScrollUpdateWithoutPropagation:
    case PlatformEvent::GestureScrollEnd:
    case PlatformEvent::GestureFlingStart:
        // Handle directly in main frame
        break;

    case PlatformEvent::GestureTap:
    case PlatformEvent::GestureTapUnconfirmed:
    case PlatformEvent::GestureTapDown:
    case PlatformEvent::GestureShowPress:
    case PlatformEvent::GestureTapDownCancel:
    case PlatformEvent::GestureTwoFingerTap:
    case PlatformEvent::GestureLongPress:
    case PlatformEvent::GestureLongTap:
    case PlatformEvent::GesturePinchBegin:
    case PlatformEvent::GesturePinchEnd:
    case PlatformEvent::GesturePinchUpdate:
        adjustGesturePosition(gestureEvent, adjustedPoint);
        subframe = getSubFrameForGestureEvent(adjustedPoint, gestureEvent);
        if (subframe)
            return subframe->eventHandler().handleGestureEvent(gestureEvent);
        break;

    default:
        ASSERT_NOT_REACHED();
    }

    RefPtrWillBeRawPtr<Node> eventTarget = nullptr;
    RefPtr<Scrollbar> scrollbar;
    if (gestureEvent.type() == PlatformEvent::GestureScrollEnd
        || gestureEvent.type() == PlatformEvent::GestureScrollUpdate
        || gestureEvent.type() == PlatformEvent::GestureScrollUpdateWithoutPropagation
        || gestureEvent.type() == PlatformEvent::GestureFlingStart) {
        scrollbar = m_scrollbarHandlingScrollGesture.get();
        eventTarget = m_scrollGestureHandlingNode.get();
    }

    HitTestRequest::HitTestRequestType hitType = HitTestRequest::TouchEvent;
    double activeInterval = 0;
    bool shouldKeepActiveForMinInterval = false;
    if (gestureEvent.type() == PlatformEvent::GestureShowPress
        || gestureEvent.type() == PlatformEvent::GestureTapUnconfirmed) {
        hitType |= HitTestRequest::Active;
    } else if (gestureEvent.type() == PlatformEvent::GestureTapDownCancel) {
        hitType |= HitTestRequest::Release;
        // A TapDownCancel received when no element is active shouldn't really be changing hover state.
        if (!m_frame->document()->activeHoverElement())
            hitType |= HitTestRequest::ReadOnly;
    } else if (gestureEvent.type() == PlatformEvent::GestureTap) {
        hitType |= HitTestRequest::Release;
        // If the Tap is received very shortly after ShowPress, we want to
        // delay clearing of the active state so that it's visible to the user
        // for at least a couple of frames.
        activeInterval = WTF::currentTime() - m_lastShowPressTimestamp;
        shouldKeepActiveForMinInterval = m_lastShowPressTimestamp && activeInterval < minimumActiveInterval;
        if (shouldKeepActiveForMinInterval)
            hitType |= HitTestRequest::ReadOnly;
    }
    else
        hitType |= HitTestRequest::Active | HitTestRequest::ReadOnly;

    if ((!scrollbar && !eventTarget) || !(hitType & HitTestRequest::ReadOnly)) {
        IntPoint hitTestPoint = m_frame->view()->windowToContents(adjustedPoint);
        HitTestResult result = hitTestResultAtPoint(hitTestPoint, hitType | HitTestRequest::AllowFrameScrollbars);

        if (shouldKeepActiveForMinInterval) {
            m_lastDeferredTapElement = result.innerElement();
            m_activeIntervalTimer.startOneShot(minimumActiveInterval - activeInterval, FROM_HERE);
        }

        eventTarget = result.targetNode();
        if (!scrollbar) {
            FrameView* view = m_frame->view();
            scrollbar = view ? view->scrollbarAtPoint(gestureEvent.position()) : 0;
        }
        if (!scrollbar)
            scrollbar = result.scrollbar();
    }

    if (scrollbar) {
        bool eventSwallowed = scrollbar->gestureEvent(gestureEvent);
        if (gestureEvent.type() == PlatformEvent::GestureTapDown && eventSwallowed) {
            m_scrollbarHandlingScrollGesture = scrollbar;
        } else if (gestureEvent.type() == PlatformEvent::GestureScrollEnd
            || gestureEvent.type() == PlatformEvent::GestureFlingStart
            || !eventSwallowed) {
            m_scrollbarHandlingScrollGesture = nullptr;
        }

        if (eventSwallowed)
            return true;
    }

    if (eventTarget) {
        bool eventSwallowed = false;
        if (handleScrollGestureOnResizer(eventTarget.get(), gestureEvent))
            eventSwallowed = true;
        else
            eventSwallowed = eventTarget->dispatchGestureEvent(gestureEvent);
        if (gestureEvent.type() == PlatformEvent::GestureScrollBegin || gestureEvent.type() == PlatformEvent::GestureScrollEnd) {
            if (eventSwallowed)
                m_scrollGestureHandlingNode = eventTarget;
        }

        if (eventSwallowed)
            return true;
    }

    // FIXME: A more general scroll system (https://bugs.webkit.org/show_bug.cgi?id=80596) will
    // eliminate the need for this.
    TemporaryChange<PlatformEvent::Type> baseEventType(m_baseEventType, gestureEvent.type());

    switch (gestureEvent.type()) {
    case PlatformEvent::GestureScrollBegin:
        return handleGestureScrollBegin(gestureEvent);
    case PlatformEvent::GestureScrollUpdate:
    case PlatformEvent::GestureScrollUpdateWithoutPropagation:
        return handleGestureScrollUpdate(gestureEvent);
    case PlatformEvent::GestureScrollEnd:
        return handleGestureScrollEnd(gestureEvent);
    case PlatformEvent::GestureTap:
        return handleGestureTap(gestureEvent, adjustedPoint);
    case PlatformEvent::GestureShowPress:
        return handleGestureShowPress();
    case PlatformEvent::GestureLongPress:
        return handleGestureLongPress(gestureEvent, adjustedPoint);
    case PlatformEvent::GestureLongTap:
        return handleGestureLongTap(gestureEvent, adjustedPoint);
    case PlatformEvent::GestureTwoFingerTap:
        return handleGestureTwoFingerTap(gestureEvent, adjustedPoint);
    case PlatformEvent::GestureTapDown:
    case PlatformEvent::GesturePinchBegin:
    case PlatformEvent::GesturePinchEnd:
    case PlatformEvent::GesturePinchUpdate:
    case PlatformEvent::GestureTapDownCancel:
    case PlatformEvent::GestureTapUnconfirmed:
    case PlatformEvent::GestureFlingStart:
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    return false;
}

bool EventHandler::handleGestureTap(const PlatformGestureEvent& gestureEvent, const IntPoint& adjustedPoint)
{
    // FIXME: Refactor this code to not hit test multiple times. We use the adjusted position to ensure that the correct node is targeted by the later redundant hit tests.

    unsigned modifierFlags = 0;
    if (gestureEvent.altKey())
        modifierFlags |= PlatformEvent::AltKey;
    if (gestureEvent.ctrlKey())
        modifierFlags |= PlatformEvent::CtrlKey;
    if (gestureEvent.metaKey())
        modifierFlags |= PlatformEvent::MetaKey;
    if (gestureEvent.shiftKey())
        modifierFlags |= PlatformEvent::ShiftKey;
    PlatformEvent::Modifiers modifiers = static_cast<PlatformEvent::Modifiers>(modifierFlags);

    PlatformMouseEvent fakeMouseMove(adjustedPoint, gestureEvent.globalPosition(),
        NoButton, PlatformEvent::MouseMoved, /* clickCount */ 0,
        modifiers, PlatformMouseEvent::FromTouch, gestureEvent.timestamp());
    handleMouseMoveEvent(fakeMouseMove);

    bool defaultPrevented = false;
    PlatformMouseEvent fakeMouseDown(adjustedPoint, gestureEvent.globalPosition(),
        LeftButton, PlatformEvent::MousePressed, gestureEvent.tapCount(),
        modifiers, PlatformMouseEvent::FromTouch,  gestureEvent.timestamp());
    defaultPrevented |= handleMousePressEvent(fakeMouseDown);

    PlatformMouseEvent fakeMouseUp(adjustedPoint, gestureEvent.globalPosition(),
        LeftButton, PlatformEvent::MouseReleased, gestureEvent.tapCount(),
        modifiers, PlatformMouseEvent::FromTouch,  gestureEvent.timestamp());
    defaultPrevented |= handleMouseReleaseEvent(fakeMouseUp);

    return defaultPrevented;
}

bool EventHandler::handleGestureLongPress(const PlatformGestureEvent& gestureEvent, const IntPoint& adjustedPoint)
{
    m_longTapShouldInvokeContextMenu = false;
    if (m_frame->settings() && m_frame->settings()->touchDragDropEnabled() && m_frame->view()) {
        PlatformMouseEvent mouseDownEvent(adjustedPoint, gestureEvent.globalPosition(), LeftButton, PlatformEvent::MousePressed, 1,
            gestureEvent.shiftKey(), gestureEvent.ctrlKey(), gestureEvent.altKey(), gestureEvent.metaKey(), WTF::currentTime());
        m_mouseDown = mouseDownEvent;

        PlatformMouseEvent mouseDragEvent(adjustedPoint, gestureEvent.globalPosition(), LeftButton, PlatformEvent::MouseMoved, 1,
            gestureEvent.shiftKey(), gestureEvent.ctrlKey(), gestureEvent.altKey(), gestureEvent.metaKey(), WTF::currentTime());
        HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::ConfusingAndOftenMisusedDisallowShadowContent);
        MouseEventWithHitTestResults mev = prepareMouseEvent(request, mouseDragEvent);
        m_didStartDrag = false;
        m_mouseDownMayStartDrag = true;
        dragState().m_dragSrc = nullptr;
        m_mouseDownPos = m_frame->view()->windowToContents(mouseDragEvent.position());
        RefPtr<FrameView> protector(m_frame->view());
        handleDrag(mev, DontCheckDragHysteresis);
        if (m_didStartDrag) {
            m_longTapShouldInvokeContextMenu = true;
            return true;
        }
    }
#if OS(ANDROID)
    bool shouldLongPressSelectWord = true;
#else
    bool shouldLongPressSelectWord = m_frame->settings() && m_frame->settings()->touchEditingEnabled();
#endif
    if (shouldLongPressSelectWord) {
        IntPoint hitTestPoint = m_frame->view()->windowToContents(gestureEvent.position());
        HitTestResult result = hitTestResultAtPoint(hitTestPoint);
        Node* innerNode = result.targetNode();
        if (!result.isLiveLink() && innerNode && (innerNode->isContentEditable() || innerNode->isTextNode())) {
            selectClosestWordFromHitTestResult(result, DontAppendTrailingWhitespace);
            if (m_frame->selection().isRange()) {
                focusDocumentView();
                return true;
            }
        }
    }
    return sendContextMenuEventForGesture(gestureEvent);
}

bool EventHandler::handleGestureLongTap(const PlatformGestureEvent& gestureEvent, const IntPoint& adjustedPoint)
{
#if !OS(ANDROID)
    if (m_longTapShouldInvokeContextMenu) {
        m_longTapShouldInvokeContextMenu = false;
        return sendContextMenuEventForGesture(gestureEvent);
    }
#endif
    return false;
}

bool EventHandler::handleScrollGestureOnResizer(Node* eventTarget, const PlatformGestureEvent& gestureEvent) {
    if (gestureEvent.type() == PlatformEvent::GestureScrollBegin) {
        RenderLayer* layer = eventTarget->renderer() ? eventTarget->renderer()->enclosingLayer() : 0;
        IntPoint p = m_frame->view()->windowToContents(gestureEvent.position());
        if (layer && layer->scrollableArea() && layer->scrollableArea()->isPointInResizeControl(p, ResizerForTouch)) {
            m_resizeScrollableArea = layer->scrollableArea();
            m_resizeScrollableArea->setInResizeMode(true);
            m_offsetFromResizeCorner = m_resizeScrollableArea->offsetFromResizeCorner(p);
            return true;
        }
    } else if (gestureEvent.type() == PlatformEvent::GestureScrollUpdate ||
               gestureEvent.type() == PlatformEvent::GestureScrollUpdateWithoutPropagation) {
        if (m_resizeScrollableArea && m_resizeScrollableArea->inResizeMode()) {
            m_resizeScrollableArea->resize(gestureEvent, m_offsetFromResizeCorner);
            return true;
        }
    } else if (gestureEvent.type() == PlatformEvent::GestureScrollEnd) {
        if (m_resizeScrollableArea && m_resizeScrollableArea->inResizeMode()) {
            m_resizeScrollableArea->setInResizeMode(false);
            m_resizeScrollableArea = 0;
            return false;
        }
    }

    return false;
}

bool EventHandler::handleGestureTwoFingerTap(const PlatformGestureEvent& gestureEvent, const IntPoint& adjustedPoint)
{
    return sendContextMenuEventForGesture(gestureEvent);
}

bool EventHandler::passGestureEventToWidget(const PlatformGestureEvent& gestureEvent, Widget* widget)
{
    if (!widget)
        return false;

    if (!widget->isFrameView())
        return false;

    return toFrameView(widget)->frame().eventHandler().handleGestureEvent(gestureEvent);
}

bool EventHandler::passGestureEventToWidgetIfPossible(const PlatformGestureEvent& gestureEvent, RenderObject* renderer)
{
    if (m_lastHitTestResultOverWidget && renderer && renderer->isWidget()) {
        Widget* widget = toRenderWidget(renderer)->widget();
        return widget && passGestureEventToWidget(gestureEvent, widget);
    }
    return false;
}

bool EventHandler::handleGestureScrollEnd(const PlatformGestureEvent& gestureEvent) {
    RefPtrWillBeRawPtr<Node> node = m_scrollGestureHandlingNode;
    clearGestureScrollNodes();

    if (node)
        passGestureEventToWidgetIfPossible(gestureEvent, node->renderer());

    return false;
}

bool EventHandler::handleGestureScrollBegin(const PlatformGestureEvent& gestureEvent)
{
    Document* document = m_frame->document();
    if (!document->renderView())
        return false;

    FrameView* view = m_frame->view();
    if (!view)
        return false;

    LayoutPoint viewPoint = view->windowToContents(gestureEvent.position());
    HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::ConfusingAndOftenMisusedDisallowShadowContent);
    HitTestResult result(viewPoint);
    document->renderView()->hitTest(request, result);

    m_lastHitTestResultOverWidget = result.isOverWidget();
    m_scrollGestureHandlingNode = result.innerNode();
    m_previousGestureScrolledNode = nullptr;

    // If there's no renderer on the node, send the event to the nearest ancestor with a renderer.
    // Needed for <option> and <optgroup> elements so we can touch scroll <select>s
    while (m_scrollGestureHandlingNode && !m_scrollGestureHandlingNode->renderer())
        m_scrollGestureHandlingNode = m_scrollGestureHandlingNode->parentOrShadowHostNode();

    if (!m_scrollGestureHandlingNode)
        return false;

    passGestureEventToWidgetIfPossible(gestureEvent, m_scrollGestureHandlingNode->renderer());

    return true;
}

bool EventHandler::handleGestureScrollUpdate(const PlatformGestureEvent& gestureEvent)
{
    FloatSize delta(gestureEvent.deltaX(), gestureEvent.deltaY());
    if (delta.isZero())
        return false;

    const float scaleFactor = m_frame->pageZoomFactor();
    delta.scale(1 / scaleFactor, 1 / scaleFactor);

    Node* node = m_scrollGestureHandlingNode.get();
    if (!node)
        return sendScrollEventToView(gestureEvent, delta);

    // Ignore this event if the targeted node does not have a valid renderer.
    RenderObject* renderer = node->renderer();
    if (!renderer)
        return false;

    RefPtr<FrameView> protector(m_frame->view());

    Node* stopNode = 0;
    bool scrollShouldNotPropagate = gestureEvent.type() == PlatformEvent::GestureScrollUpdateWithoutPropagation;

    // Try to send the event to the correct view.
    if (passGestureEventToWidgetIfPossible(gestureEvent, renderer)) {
        if(scrollShouldNotPropagate)
              m_previousGestureScrolledNode = m_scrollGestureHandlingNode;

        return true;
    }

    if (scrollShouldNotPropagate)
        stopNode = m_previousGestureScrolledNode.get();

    // First try to scroll the closest scrollable RenderBox ancestor of |node|.
    ScrollGranularity granularity = ScrollByPixel;
    bool horizontalScroll = scroll(ScrollLeft, granularity, node, &stopNode, delta.width());
    bool verticalScroll = scroll(ScrollUp, granularity, node, &stopNode, delta.height());

    if (scrollShouldNotPropagate)
        m_previousGestureScrolledNode = stopNode;

    if (horizontalScroll || verticalScroll) {
        setFrameWasScrolledByUser();
        return true;
    }

    // Otherwise try to scroll the view.
    return sendScrollEventToView(gestureEvent, delta);
}

bool EventHandler::sendScrollEventToView(const PlatformGestureEvent& gestureEvent, const FloatSize& scaledDelta)
{
    FrameView* view = m_frame->view();
    if (!view)
        return false;

    const float tickDivisor = static_cast<float>(WheelEvent::TickMultiplier);
    IntPoint point(gestureEvent.position().x(), gestureEvent.position().y());
    IntPoint globalPoint(gestureEvent.globalPosition().x(), gestureEvent.globalPosition().y());
    PlatformWheelEvent syntheticWheelEvent(point, globalPoint,
        scaledDelta.width(), scaledDelta.height(),
        scaledDelta.width() / tickDivisor, scaledDelta.height() / tickDivisor,
        ScrollByPixelWheelEvent,
        gestureEvent.shiftKey(), gestureEvent.ctrlKey(), gestureEvent.altKey(), gestureEvent.metaKey());
    syntheticWheelEvent.setHasPreciseScrollingDeltas(true);

    bool scrolledFrame = view->wheelEvent(syntheticWheelEvent);
    if (scrolledFrame)
        setFrameWasScrolledByUser();

    return scrolledFrame;
}

LocalFrame* EventHandler::getSubFrameForGestureEvent(const IntPoint& touchAdjustedPoint, const PlatformGestureEvent& gestureEvent)
{
    PlatformMouseEvent mouseDown(touchAdjustedPoint, gestureEvent.globalPosition(), LeftButton, PlatformEvent::MousePressed, 1,
        gestureEvent.shiftKey(), gestureEvent.ctrlKey(), gestureEvent.altKey(), gestureEvent.metaKey(), gestureEvent.timestamp());
    HitTestRequest request(HitTestRequest::ReadOnly);
    MouseEventWithHitTestResults mev = prepareMouseEvent(request, mouseDown);
    return subframeForHitTestResult(mev);
}

void EventHandler::clearGestureScrollNodes()
{
    m_scrollGestureHandlingNode = nullptr;
    m_previousGestureScrolledNode = nullptr;
}

bool EventHandler::isScrollbarHandlingGestures() const
{
    return m_scrollbarHandlingScrollGesture.get();
}

bool EventHandler::shouldApplyTouchAdjustment(const PlatformGestureEvent& event) const
{
    if (m_frame->settings() && !m_frame->settings()->touchAdjustmentEnabled())
        return false;
    return !event.area().isEmpty();
}

bool EventHandler::bestClickableNodeForTouchPoint(const IntPoint& touchCenter, const IntSize& touchRadius, IntPoint& targetPoint, Node*& targetNode)
{
    IntPoint hitTestPoint = m_frame->view()->windowToContents(touchCenter);
    HitTestResult result = hitTestResultAtPoint(hitTestPoint, HitTestRequest::ReadOnly | HitTestRequest::Active, touchRadius);

    // If the touch is over a scrollbar, don't adjust the touch point since touch adjustment only takes into account
    // DOM nodes so a touch over a scrollbar will be adjusted towards nearby nodes. This leads to things like textarea
    // scrollbars being untouchable.
    if (result.scrollbar())
        return false;

    IntRect touchRect(touchCenter - touchRadius, touchRadius + touchRadius);
    WillBeHeapVector<RefPtrWillBeMember<Node>, 11> nodes;
    copyToVector(result.rectBasedTestResult(), nodes);

    // FIXME: Should be able to handle targetNode being a shadow DOM node to avoid performing uncessary hit tests
    // in the case where further processing on the node is required. Returning the shadow ancestor prevents a
    // regression in touchadjustment/html-label.html. Some refinement is required to testing/internals to
    // handle targetNode being a shadow DOM node.

    // FIXME: the explicit Vector conversion copies into a temporary and is wasteful.
    // FIXME: targetNode and success are only used by Internals functions. We should
    // instead have dedicated test methods so we only do this work in tests.
    bool success = findBestClickableCandidate(targetNode, targetPoint, touchCenter, touchRect, WillBeHeapVector<RefPtrWillBeMember<Node> > (nodes));
    if (success && targetNode)
        targetNode = targetNode->deprecatedShadowAncestorNode();
    return success;
}

bool EventHandler::bestContextMenuNodeForTouchPoint(const IntPoint& touchCenter, const IntSize& touchRadius, IntPoint& targetPoint, Node*& targetNode)
{
    IntPoint hitTestPoint = m_frame->view()->windowToContents(touchCenter);
    HitTestResult result = hitTestResultAtPoint(hitTestPoint, HitTestRequest::ReadOnly | HitTestRequest::Active, touchRadius);

    IntRect touchRect(touchCenter - touchRadius, touchRadius + touchRadius);
    WillBeHeapVector<RefPtrWillBeMember<Node>, 11> nodes;
    copyToVector(result.rectBasedTestResult(), nodes);

    // FIXME: the explicit Vector conversion copies into a temporary and is wasteful.
    return findBestContextMenuCandidate(targetNode, targetPoint, touchCenter, touchRect, WillBeHeapVector<RefPtrWillBeMember<Node> >(nodes));
}

bool EventHandler::bestZoomableAreaForTouchPoint(const IntPoint& touchCenter, const IntSize& touchRadius, IntRect& targetArea, Node*& targetNode)
{
    IntPoint hitTestPoint = m_frame->view()->windowToContents(touchCenter);
    HitTestResult result = hitTestResultAtPoint(hitTestPoint, HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::ConfusingAndOftenMisusedDisallowShadowContent, touchRadius);

    IntRect touchRect(touchCenter - touchRadius, touchRadius + touchRadius);
    WillBeHeapVector<RefPtrWillBeMember<Node>, 11> nodes;
    copyToVector(result.rectBasedTestResult(), nodes);

    // FIXME: the explicit Vector conversion copies into a temporary and is wasteful.
    return findBestZoomableArea(targetNode, targetArea, touchCenter, touchRect, WillBeHeapVector<RefPtrWillBeMember<Node> >(nodes));
}

void EventHandler::adjustGesturePosition(const PlatformGestureEvent& gestureEvent, IntPoint& adjustedPoint)
{
    if (!shouldApplyTouchAdjustment(gestureEvent))
        return;

    Node* targetNode = 0;
    switch (gestureEvent.type()) {
    case PlatformEvent::GestureTap:
    case PlatformEvent::GestureTapUnconfirmed:
    case PlatformEvent::GestureTapDown:
    case PlatformEvent::GestureShowPress:
        bestClickableNodeForTouchPoint(gestureEvent.position(), IntSize(gestureEvent.area().width() / 2, gestureEvent.area().height() / 2), adjustedPoint, targetNode);
        break;
    case PlatformEvent::GestureLongPress:
    case PlatformEvent::GestureLongTap:
    case PlatformEvent::GestureTwoFingerTap:
        bestContextMenuNodeForTouchPoint(gestureEvent.position(), IntSize(gestureEvent.area().width() / 2, gestureEvent.area().height() / 2), adjustedPoint, targetNode);
        break;
    default:
        // FIXME: Implement handling for other types as needed.
        ASSERT_NOT_REACHED();
    }
}

bool EventHandler::sendContextMenuEvent(const PlatformMouseEvent& event)
{
    Document* doc = m_frame->document();
    FrameView* v = m_frame->view();
    if (!v)
        return false;

    // Clear mouse press state to avoid initiating a drag while context menu is up.
    m_mousePressed = false;
    LayoutPoint viewportPos = v->windowToContents(event.position());
    HitTestRequest request(HitTestRequest::Active | HitTestRequest::ConfusingAndOftenMisusedDisallowShadowContent);
    MouseEventWithHitTestResults mev = doc->prepareMouseEvent(request, viewportPos, event);

    if (!m_frame->selection().contains(viewportPos)
        && !mev.scrollbar()
        // FIXME: In the editable case, word selection sometimes selects content that isn't underneath the mouse.
        // If the selection is non-editable, we do word selection to make it easier to use the contextual menu items
        // available for text selections.  But only if we're above text.
        && (m_frame->selection().isContentEditable() || (mev.targetNode() && mev.targetNode()->isTextNode()))) {
        m_mouseDownMayStartSelect = true; // context menu events are always allowed to perform a selection

        if (mev.hitTestResult().isMisspelled())
            selectClosestMisspellingFromMouseEvent(mev);
        else if (m_frame->editor().behavior().shouldSelectOnContextualMenuClick())
            selectClosestWordOrLinkFromMouseEvent(mev);
    }

    return !dispatchMouseEvent(EventTypeNames::contextmenu, mev.targetNode(), 0, event, false);
}

bool EventHandler::sendContextMenuEventForKey()
{
    FrameView* view = m_frame->view();
    if (!view)
        return false;

    Document* doc = m_frame->document();
    if (!doc)
        return false;

    // Clear mouse press state to avoid initiating a drag while context menu is up.
    m_mousePressed = false;

    static const int kContextMenuMargin = 1;

#if OS(WIN)
    int rightAligned = ::GetSystemMetrics(SM_MENUDROPALIGNMENT);
#else
    int rightAligned = 0;
#endif
    IntPoint location;

    Element* focusedElement = doc->focusedElement();
    FrameSelection& selection = m_frame->selection();
    Position start = selection.selection().start();
    bool shouldTranslateToRootView = true;

    if (start.deprecatedNode() && (selection.rootEditableElement() || selection.isRange())) {
        RefPtrWillBeRawPtr<Range> selectionRange = selection.toNormalizedRange();
        IntRect firstRect = m_frame->editor().firstRectForRange(selectionRange.get());

        int x = rightAligned ? firstRect.maxX() : firstRect.x();
        // In a multiline edit, firstRect.maxY() would endup on the next line, so -1.
        int y = firstRect.maxY() ? firstRect.maxY() - 1 : 0;
        location = IntPoint(x, y);
    } else if (focusedElement) {
        IntRect clippedRect = focusedElement->boundsInRootViewSpace();
        location = IntPoint(clippedRect.center());
    } else {
        location = IntPoint(
            rightAligned ? view->contentsWidth() - kContextMenuMargin : kContextMenuMargin,
            kContextMenuMargin);
        shouldTranslateToRootView = false;
    }

    m_frame->view()->setCursor(pointerCursor());

    IntPoint position = shouldTranslateToRootView ? view->contentsToRootView(location) : location;
    IntPoint globalPosition = view->hostWindow()->rootViewToScreen(IntRect(position, IntSize())).location();

    Node* targetNode = doc->focusedElement();
    if (!targetNode)
        targetNode = doc;

    // Use the focused node as the target for hover and active.
    HitTestResult result(position);
    result.setInnerNode(targetNode);
    doc->updateHoverActiveState(HitTestRequest::Active | HitTestRequest::ConfusingAndOftenMisusedDisallowShadowContent, result.innerElement());

    // The contextmenu event is a mouse event even when invoked using the keyboard.
    // This is required for web compatibility.

#if OS(WIN)
    PlatformEvent::Type eventType = PlatformEvent::MouseReleased;
#else
    PlatformEvent::Type eventType = PlatformEvent::MousePressed;
#endif

    PlatformMouseEvent mouseEvent(position, globalPosition, RightButton, eventType, 1, false, false, false, false, WTF::currentTime());

    handleMousePressEvent(mouseEvent);
    return sendContextMenuEvent(mouseEvent);
}

bool EventHandler::sendContextMenuEventForGesture(const PlatformGestureEvent& event)
{
#if OS(WIN)
    PlatformEvent::Type eventType = PlatformEvent::MouseReleased;
#else
    PlatformEvent::Type eventType = PlatformEvent::MousePressed;
#endif

    IntPoint adjustedPoint = event.position();
    adjustGesturePosition(event, adjustedPoint);
    PlatformMouseEvent mouseEvent(adjustedPoint, event.globalPosition(), RightButton, eventType, 1, false, false, false, false, WTF::currentTime());
    // To simulate right-click behavior, we send a right mouse down and then
    // context menu event.
    handleMousePressEvent(mouseEvent);
    return sendContextMenuEvent(mouseEvent);
    // We do not need to send a corresponding mouse release because in case of
    // right-click, the context menu takes capture and consumes all events.
}

void EventHandler::scheduleHoverStateUpdate()
{
    if (!m_hoverTimer.isActive())
        m_hoverTimer.startOneShot(0, FROM_HERE);
}

void EventHandler::scheduleCursorUpdate()
{
    if (!m_cursorUpdateTimer.isActive())
        m_cursorUpdateTimer.startOneShot(cursorUpdateInterval, FROM_HERE);
}

void EventHandler::dispatchFakeMouseMoveEventSoon()
{
    if (m_mousePressed)
        return;

    if (m_mousePositionIsUnknown)
        return;

    Settings* settings = m_frame->settings();
    if (settings && !settings->deviceSupportsMouse())
        return;

    // If the content has ever taken longer than fakeMouseMoveShortInterval we
    // reschedule the timer and use a longer time. This will cause the content
    // to receive these moves only after the user is done scrolling, reducing
    // pauses during the scroll.
    if (m_maxMouseMovedDuration > fakeMouseMoveShortInterval) {
        if (m_fakeMouseMoveEventTimer.isActive())
            m_fakeMouseMoveEventTimer.stop();
        m_fakeMouseMoveEventTimer.startOneShot(fakeMouseMoveLongInterval, FROM_HERE);
    } else {
        if (!m_fakeMouseMoveEventTimer.isActive())
            m_fakeMouseMoveEventTimer.startOneShot(fakeMouseMoveShortInterval, FROM_HERE);
    }
}

void EventHandler::dispatchFakeMouseMoveEventSoonInQuad(const FloatQuad& quad)
{
    FrameView* view = m_frame->view();
    if (!view)
        return;

    if (!quad.containsPoint(view->windowToContents(m_lastKnownMousePosition)))
        return;

    dispatchFakeMouseMoveEventSoon();
}

void EventHandler::fakeMouseMoveEventTimerFired(Timer<EventHandler>* timer)
{
    ASSERT_UNUSED(timer, timer == &m_fakeMouseMoveEventTimer);
    ASSERT(!m_mousePressed);

    Settings* settings = m_frame->settings();
    if (settings && !settings->deviceSupportsMouse())
        return;

    FrameView* view = m_frame->view();
    if (!view)
        return;

    if (!m_frame->page() || !m_frame->page()->focusController().isActive())
        return;

    // Don't dispatch a synthetic mouse move event if the mouse cursor is not visible to the user.
    if (!isCursorVisible())
        return;

    bool shiftKey;
    bool ctrlKey;
    bool altKey;
    bool metaKey;
    PlatformKeyboardEvent::getCurrentModifierState(shiftKey, ctrlKey, altKey, metaKey);
    PlatformMouseEvent fakeMouseMoveEvent(m_lastKnownMousePosition, m_lastKnownMouseGlobalPosition, NoButton, PlatformEvent::MouseMoved, 0, shiftKey, ctrlKey, altKey, metaKey, currentTime());
    handleMouseMoveEvent(fakeMouseMoveEvent);
}

void EventHandler::cancelFakeMouseMoveEvent()
{
    m_fakeMouseMoveEventTimer.stop();
}

bool EventHandler::isCursorVisible() const
{
    return m_frame->page()->isCursorVisible();
}

void EventHandler::setResizingFrameSet(HTMLFrameSetElement* frameSet)
{
    m_frameSetBeingResized = frameSet;
}

void EventHandler::resizeScrollableAreaDestroyed()
{
    ASSERT(m_resizeScrollableArea);
    m_resizeScrollableArea = 0;
}

void EventHandler::hoverTimerFired(Timer<EventHandler>*)
{
    m_hoverTimer.stop();

    ASSERT(m_frame);
    ASSERT(m_frame->document());

    if (RenderView* renderer = m_frame->contentRenderer()) {
        if (FrameView* view = m_frame->view()) {
            HitTestRequest request(HitTestRequest::Move | HitTestRequest::ConfusingAndOftenMisusedDisallowShadowContent);
            HitTestResult result(view->windowToContents(m_lastKnownMousePosition));
            renderer->hitTest(request, result);
            m_frame->document()->updateHoverActiveState(request, result.innerElement());
        }
    }
}

void EventHandler::activeIntervalTimerFired(Timer<EventHandler>*)
{
    m_activeIntervalTimer.stop();

    if (m_frame
        && m_frame->document()
        && m_lastDeferredTapElement) {
        // FIXME: Enable condition when http://crbug.com/226842 lands
        // m_lastDeferredTapElement.get() == m_frame->document()->activeElement()
        HitTestRequest request(HitTestRequest::TouchEvent | HitTestRequest::Release);
        m_frame->document()->updateHoverActiveState(request, m_lastDeferredTapElement.get());
    }
    m_lastDeferredTapElement = nullptr;
}

void EventHandler::notifyElementActivated()
{
    // Since another element has been set to active, stop current timer and clear reference.
    if (m_activeIntervalTimer.isActive())
        m_activeIntervalTimer.stop();
    m_lastDeferredTapElement = nullptr;
}

bool EventHandler::handleAccessKey(const PlatformKeyboardEvent& evt)
{
    // FIXME: Ignoring the state of Shift key is what neither IE nor Firefox do.
    // IE matches lower and upper case access keys regardless of Shift key state - but if both upper and
    // lower case variants are present in a document, the correct element is matched based on Shift key state.
    // Firefox only matches an access key if Shift is not pressed, and does that case-insensitively.
    ASSERT(!(accessKeyModifiers() & PlatformEvent::ShiftKey));
    if ((evt.modifiers() & ~PlatformEvent::ShiftKey) != accessKeyModifiers())
        return false;
    String key = evt.unmodifiedText();
    Element* elem = m_frame->document()->getElementByAccessKey(key.lower());
    if (!elem)
        return false;
    elem->accessKeyAction(false);
    return true;
}

bool EventHandler::isKeyEventAllowedInFullScreen(FullscreenElementStack* fullscreen, const PlatformKeyboardEvent& keyEvent) const
{
    if (fullscreen->webkitFullScreenKeyboardInputAllowed())
        return true;

    if (keyEvent.type() == PlatformKeyboardEvent::Char) {
        if (keyEvent.text().length() != 1)
            return false;
        UChar character = keyEvent.text()[0];
        return character == ' ';
    }

    int keyCode = keyEvent.windowsVirtualKeyCode();
    return (keyCode >= VK_BACK && keyCode <= VK_CAPITAL)
        || (keyCode >= VK_SPACE && keyCode <= VK_DELETE)
        || (keyCode >= VK_OEM_1 && keyCode <= VK_OEM_PLUS)
        || (keyCode >= VK_MULTIPLY && keyCode <= VK_OEM_8);
}

bool EventHandler::keyEvent(const PlatformKeyboardEvent& initialKeyEvent)
{
    RefPtr<FrameView> protector(m_frame->view());

    ASSERT(m_frame->document());
    if (FullscreenElementStack* fullscreen = FullscreenElementStack::fromIfExists(*m_frame->document())) {
        if (fullscreen->webkitIsFullScreen() && !isKeyEventAllowedInFullScreen(fullscreen, initialKeyEvent))
            return false;
    }

    if (initialKeyEvent.windowsVirtualKeyCode() == VK_CAPITAL)
        capsLockStateMayHaveChanged();

#if OS(WIN)
    if (panScrollInProgress()) {
        // If a key is pressed while the panScroll is in progress then we want to stop
        if (initialKeyEvent.type() == PlatformEvent::KeyDown || initialKeyEvent.type() == PlatformEvent::RawKeyDown)
            stopAutoscroll();

        // If we were in panscroll mode, we swallow the key event
        return true;
    }
#endif

    // Check for cases where we are too early for events -- possible unmatched key up
    // from pressing return in the location bar.
    RefPtrWillBeRawPtr<Node> node = eventTargetNodeForDocument(m_frame->document());
    if (!node)
        return false;

    UserGestureIndicator gestureIndicator(DefinitelyProcessingUserGesture);

    // In IE, access keys are special, they are handled after default keydown processing, but cannot be canceled - this is hard to match.
    // On Mac OS X, we process them before dispatching keydown, as the default keydown handler implements Emacs key bindings, which may conflict
    // with access keys. Then we dispatch keydown, but suppress its default handling.
    // On Windows, WebKit explicitly calls handleAccessKey() instead of dispatching a keypress event for WM_SYSCHAR messages.
    // Other platforms currently match either Mac or Windows behavior, depending on whether they send combined KeyDown events.
    bool matchedAnAccessKey = false;
    if (initialKeyEvent.type() == PlatformEvent::KeyDown)
        matchedAnAccessKey = handleAccessKey(initialKeyEvent);

    // FIXME: it would be fair to let an input method handle KeyUp events before DOM dispatch.
    if (initialKeyEvent.type() == PlatformEvent::KeyUp || initialKeyEvent.type() == PlatformEvent::Char)
        return !node->dispatchKeyEvent(initialKeyEvent);

    PlatformKeyboardEvent keyDownEvent = initialKeyEvent;
    if (keyDownEvent.type() != PlatformEvent::RawKeyDown)
        keyDownEvent.disambiguateKeyDownEvent(PlatformEvent::RawKeyDown);
    RefPtrWillBeRawPtr<KeyboardEvent> keydown = KeyboardEvent::create(keyDownEvent, m_frame->document()->domWindow());
    if (matchedAnAccessKey)
        keydown->setDefaultPrevented(true);
    keydown->setTarget(node);

    if (initialKeyEvent.type() == PlatformEvent::RawKeyDown) {
        node->dispatchEvent(keydown, IGNORE_EXCEPTION);
        // If frame changed as a result of keydown dispatch, then return true to avoid sending a subsequent keypress message to the new frame.
        bool changedFocusedFrame = m_frame->page() && m_frame != m_frame->page()->focusController().focusedOrMainFrame();
        return keydown->defaultHandled() || keydown->defaultPrevented() || changedFocusedFrame;
    }

    node->dispatchEvent(keydown, IGNORE_EXCEPTION);
    // If frame changed as a result of keydown dispatch, then return early to avoid sending a subsequent keypress message to the new frame.
    bool changedFocusedFrame = m_frame->page() && m_frame != m_frame->page()->focusController().focusedOrMainFrame();
    bool keydownResult = keydown->defaultHandled() || keydown->defaultPrevented() || changedFocusedFrame;
    if (keydownResult)
        return keydownResult;

    // Focus may have changed during keydown handling, so refetch node.
    // But if we are dispatching a fake backward compatibility keypress, then we pretend that the keypress happened on the original node.
    node = eventTargetNodeForDocument(m_frame->document());
    if (!node)
        return false;

    PlatformKeyboardEvent keyPressEvent = initialKeyEvent;
    keyPressEvent.disambiguateKeyDownEvent(PlatformEvent::Char);
    if (keyPressEvent.text().isEmpty())
        return keydownResult;
    RefPtrWillBeRawPtr<KeyboardEvent> keypress = KeyboardEvent::create(keyPressEvent, m_frame->document()->domWindow());
    keypress->setTarget(node);
    if (keydownResult)
        keypress->setDefaultPrevented(true);
    node->dispatchEvent(keypress, IGNORE_EXCEPTION);

    return keydownResult || keypress->defaultPrevented() || keypress->defaultHandled();
}

static FocusType focusDirectionForKey(const AtomicString& keyIdentifier)
{
    DEFINE_STATIC_LOCAL(AtomicString, Down, ("Down", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, Up, ("Up", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, Left, ("Left", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, Right, ("Right", AtomicString::ConstructFromLiteral));

    FocusType retVal = FocusTypeNone;

    if (keyIdentifier == Down)
        retVal = FocusTypeDown;
    else if (keyIdentifier == Up)
        retVal = FocusTypeUp;
    else if (keyIdentifier == Left)
        retVal = FocusTypeLeft;
    else if (keyIdentifier == Right)
        retVal = FocusTypeRight;

    return retVal;
}

void EventHandler::defaultKeyboardEventHandler(KeyboardEvent* event)
{
    if (event->type() == EventTypeNames::keydown) {
        m_frame->editor().handleKeyboardEvent(event);
        if (event->defaultHandled())
            return;
        if (event->keyIdentifier() == "U+0009")
            defaultTabEventHandler(event);
        else if (event->keyIdentifier() == "U+0008")
            defaultBackspaceEventHandler(event);
        else if (event->keyIdentifier() == "U+001B")
            defaultEscapeEventHandler(event);
        else {
            FocusType type = focusDirectionForKey(AtomicString(event->keyIdentifier()));
            if (type != FocusTypeNone)
                defaultArrowEventHandler(type, event);
        }
    }
    if (event->type() == EventTypeNames::keypress) {
        m_frame->editor().handleKeyboardEvent(event);
        if (event->defaultHandled())
            return;
        if (event->charCode() == ' ')
            defaultSpaceEventHandler(event);
    }
}

bool EventHandler::dragHysteresisExceeded(const IntPoint& floatDragViewportLocation) const
{
    FloatPoint dragViewportLocation(floatDragViewportLocation.x(), floatDragViewportLocation.y());
    return dragHysteresisExceeded(dragViewportLocation);
}

bool EventHandler::dragHysteresisExceeded(const FloatPoint& dragViewportLocation) const
{
    FrameView* view = m_frame->view();
    if (!view)
        return false;
    IntPoint dragLocation = view->windowToContents(flooredIntPoint(dragViewportLocation));
    IntSize delta = dragLocation - m_mouseDownPos;

    int threshold = GeneralDragHysteresis;
    switch (dragState().m_dragType) {
    case DragSourceActionSelection:
        threshold = TextDragHysteresis;
        break;
    case DragSourceActionImage:
        threshold = ImageDragHysteresis;
        break;
    case DragSourceActionLink:
        threshold = LinkDragHysteresis;
        break;
    case DragSourceActionDHTML:
        break;
    case DragSourceActionNone:
        ASSERT_NOT_REACHED();
    }

    return abs(delta.width()) >= threshold || abs(delta.height()) >= threshold;
}

void EventHandler::freeClipboard()
{
    if (dragState().m_dragClipboard)
        dragState().m_dragClipboard->setAccessPolicy(ClipboardNumb);
}

void EventHandler::dragSourceEndedAt(const PlatformMouseEvent& event, DragOperation operation)
{
    // Send a hit test request so that RenderLayer gets a chance to update the :hover and :active pseudoclasses.
    HitTestRequest request(HitTestRequest::Release | HitTestRequest::ConfusingAndOftenMisusedDisallowShadowContent);
    prepareMouseEvent(request, event);

    if (dragState().m_dragSrc) {
        dragState().m_dragClipboard->setDestinationOperation(operation);
        // for now we don't care if event handler cancels default behavior, since there is none
        dispatchDragSrcEvent(EventTypeNames::dragend, event);
    }
    freeClipboard();
    dragState().m_dragSrc = nullptr;
    // In case the drag was ended due to an escape key press we need to ensure
    // that consecutive mousemove events don't reinitiate the drag and drop.
    m_mouseDownMayStartDrag = false;
}

void EventHandler::updateDragStateAfterEditDragIfNeeded(Element* rootEditableElement)
{
    // If inserting the dragged contents removed the drag source, we still want to fire dragend at the root editble element.
    if (dragState().m_dragSrc && !dragState().m_dragSrc->inDocument())
        dragState().m_dragSrc = rootEditableElement;
}

// returns if we should continue "default processing", i.e., whether eventhandler canceled
bool EventHandler::dispatchDragSrcEvent(const AtomicString& eventType, const PlatformMouseEvent& event)
{
    return !dispatchDragEvent(eventType, dragState().m_dragSrc.get(), event, dragState().m_dragClipboard.get());
}

bool EventHandler::handleDrag(const MouseEventWithHitTestResults& event, CheckDragHysteresis checkDragHysteresis)
{
    ASSERT(event.event().type() == PlatformEvent::MouseMoved);
    // Callers must protect the reference to FrameView, since this function may dispatch DOM
    // events, causing page/FrameView to go away.
    ASSERT(m_frame);
    ASSERT(m_frame->view());
    if (!m_frame->page())
        return false;

    if (event.event().button() != LeftButton || event.event().type() != PlatformEvent::MouseMoved) {
        // If we allowed the other side of the bridge to handle a drag
        // last time, then m_mousePressed might still be set. So we
        // clear it now to make sure the next move after a drag
        // doesn't look like a drag.
        m_mousePressed = false;
        return false;
    }

    if (m_mouseDownMayStartDrag) {
        HitTestRequest request(HitTestRequest::ReadOnly);
        HitTestResult result(m_mouseDownPos);
        m_frame->contentRenderer()->hitTest(request, result);
        Node* node = result.innerNode();
        if (node) {
            DragController::SelectionDragPolicy selectionDragPolicy = event.event().timestamp() - m_mouseDownTimestamp < TextDragDelay
                ? DragController::DelayedSelectionDragResolution
                : DragController::ImmediateSelectionDragResolution;
            dragState().m_dragSrc = m_frame->page()->dragController().draggableNode(m_frame, node, m_mouseDownPos, selectionDragPolicy, dragState().m_dragType);
        } else {
            dragState().m_dragSrc = nullptr;
        }

        if (!dragState().m_dragSrc)
            m_mouseDownMayStartDrag = false; // no element is draggable
    }

    if (!m_mouseDownMayStartDrag)
        return !mouseDownMayStartSelect() && !m_mouseDownMayStartAutoscroll;

    // We are starting a text/image/url drag, so the cursor should be an arrow
    // FIXME <rdar://7577595>: Custom cursors aren't supported during drag and drop (default to pointer).
    m_frame->view()->setCursor(pointerCursor());

    if (checkDragHysteresis == ShouldCheckDragHysteresis && !dragHysteresisExceeded(event.event().position()))
        return true;

    // Once we're past the hysteresis point, we don't want to treat this gesture as a click
    invalidateClick();

    if (!tryStartDrag(event)) {
        // Something failed to start the drag, clean up.
        freeClipboard();
        dragState().m_dragSrc = nullptr;
    }

    m_mouseDownMayStartDrag = false;
    // Whether or not the drag actually started, no more default handling (like selection).
    return true;
}

bool EventHandler::tryStartDrag(const MouseEventWithHitTestResults& event)
{
    freeClipboard(); // would only happen if we missed a dragEnd.  Do it anyway, just
                     // to make sure it gets numbified
    dragState().m_dragClipboard = createDraggingClipboard();

    // Check to see if this a DOM based drag, if it is get the DOM specified drag
    // image and offset
    if (dragState().m_dragType == DragSourceActionDHTML) {
        if (RenderObject* renderer = dragState().m_dragSrc->renderer()) {
            // FIXME: This doesn't work correctly with transforms.
            FloatPoint absPos = renderer->localToAbsolute();
            IntSize delta = m_mouseDownPos - roundedIntPoint(absPos);
            dragState().m_dragClipboard->setDragImageElement(dragState().m_dragSrc.get(), IntPoint(delta));
        } else {
            // The renderer has disappeared, this can happen if the onStartDrag handler has hidden
            // the element in some way. In this case we just kill the drag.
            return false;
        }
    }

    DragController& dragController = m_frame->page()->dragController();
    if (!dragController.populateDragClipboard(m_frame, dragState(), m_mouseDownPos))
        return false;
    m_mouseDownMayStartDrag = dispatchDragSrcEvent(EventTypeNames::dragstart, m_mouseDown)
        && !m_frame->selection().isInPasswordField();

    // Invalidate clipboard here against anymore pasteboard writing for security. The drag
    // image can still be changed as we drag, but not the pasteboard data.
    dragState().m_dragClipboard->setAccessPolicy(ClipboardImageWritable);

    if (m_mouseDownMayStartDrag) {
        // Dispatching the event could cause Page to go away. Make sure it's still valid before trying to use DragController.
        m_didStartDrag = m_frame->page() && dragController.startDrag(m_frame, dragState(), event.event(), m_mouseDownPos);
        // FIXME: This seems pretty useless now. The gesture code uses this as a signal for
        // whether or not the drag started, but perhaps it can simply use the return value from
        // handleDrag(), even though it doesn't mean exactly the same thing.
        if (m_didStartDrag)
            return true;
        // Drag was canned at the last minute - we owe m_dragSrc a DRAGEND event
        dispatchDragSrcEvent(EventTypeNames::dragend, event.event());
    }

    return false;
}

bool EventHandler::handleTextInputEvent(const String& text, Event* underlyingEvent, TextEventInputType inputType)
{
    // Platforms should differentiate real commands like selectAll from text input in disguise (like insertNewline),
    // and avoid dispatching text input events from keydown default handlers.
    ASSERT(!underlyingEvent || !underlyingEvent->isKeyboardEvent() || toKeyboardEvent(underlyingEvent)->type() == EventTypeNames::keypress);

    if (!m_frame)
        return false;

    EventTarget* target;
    if (underlyingEvent)
        target = underlyingEvent->target();
    else
        target = eventTargetNodeForDocument(m_frame->document());
    if (!target)
        return false;

    RefPtrWillBeRawPtr<TextEvent> event = TextEvent::create(m_frame->domWindow(), text, inputType);
    event->setUnderlyingEvent(underlyingEvent);

    target->dispatchEvent(event, IGNORE_EXCEPTION);
    return event->defaultHandled();
}

void EventHandler::defaultTextInputEventHandler(TextEvent* event)
{
    if (m_frame->editor().handleTextEvent(event))
        event->setDefaultHandled();
}

void EventHandler::defaultSpaceEventHandler(KeyboardEvent* event)
{
    ASSERT(event->type() == EventTypeNames::keypress);

    if (event->ctrlKey() || event->metaKey() || event->altKey() || event->altGraphKey())
        return;

    ScrollDirection direction = event->shiftKey() ? ScrollBlockDirectionBackward : ScrollBlockDirectionForward;
    if (scroll(direction, ScrollByPage)) {
        event->setDefaultHandled();
        return;
    }

    FrameView* view = m_frame->view();
    if (!view)
        return;

    if (view->scroll(direction, ScrollByPage))
        event->setDefaultHandled();
}

void EventHandler::defaultBackspaceEventHandler(KeyboardEvent* event)
{
    ASSERT(event->type() == EventTypeNames::keydown);

    if (event->ctrlKey() || event->metaKey() || event->altKey() || event->altGraphKey())
        return;

    if (!m_frame->editor().behavior().shouldNavigateBackOnBackspace())
        return;

    Page* page = m_frame->page();
    if (!page)
        return;
    bool handledEvent = page->mainFrame()->loader().client()->navigateBackForward(event->shiftKey() ? 1 : -1);
    if (handledEvent)
        event->setDefaultHandled();
}

void EventHandler::defaultArrowEventHandler(FocusType focusType, KeyboardEvent* event)
{
    ASSERT(event->type() == EventTypeNames::keydown);

    if (event->ctrlKey() || event->metaKey() || event->altGraphKey() || event->shiftKey())
        return;

    Page* page = m_frame->page();
    if (!page)
        return;

    if (!isSpatialNavigationEnabled(m_frame))
        return;

    // Arrows and other possible directional navigation keys can be used in design
    // mode editing.
    if (m_frame->document()->inDesignMode())
        return;

    if (page->focusController().advanceFocus(focusType))
        event->setDefaultHandled();
}

void EventHandler::defaultTabEventHandler(KeyboardEvent* event)
{
    ASSERT(event->type() == EventTypeNames::keydown);

    // We should only advance focus on tabs if no special modifier keys are held down.
    if (event->ctrlKey() || event->metaKey() || event->altGraphKey())
        return;

    Page* page = m_frame->page();
    if (!page)
        return;
    if (!page->tabKeyCyclesThroughElements())
        return;

    FocusType focusType = event->shiftKey() ? FocusTypeBackward : FocusTypeForward;

    // Tabs can be used in design mode editing.
    if (m_frame->document()->inDesignMode())
        return;

    if (page->focusController().advanceFocus(focusType))
        event->setDefaultHandled();
}

void EventHandler::defaultEscapeEventHandler(KeyboardEvent* event)
{
    if (HTMLDialogElement* dialog = m_frame->document()->activeModalDialog())
        dialog->dispatchEvent(Event::createCancelable(EventTypeNames::cancel));
}

void EventHandler::capsLockStateMayHaveChanged()
{
    if (Element* element = m_frame->document()->focusedElement()) {
        if (RenderObject* r = element->renderer()) {
            if (r->isTextField())
                toRenderTextControlSingleLine(r)->capsLockStateMayHaveChanged();
        }
    }
}

void EventHandler::setFrameWasScrolledByUser()
{
    if (FrameView* view = m_frame->view())
        view->setWasScrolledByUser(true);
}

bool EventHandler::passMousePressEventToScrollbar(MouseEventWithHitTestResults& mev, Scrollbar* scrollbar)
{
    if (!scrollbar || !scrollbar->enabled())
        return false;
    setFrameWasScrolledByUser();
    scrollbar->mouseDown(mev.event());
    return true;
}

// If scrollbar (under mouse) is different from last, send a mouse exited. Set
// last to scrollbar if setLast is true; else set last to 0.
void EventHandler::updateLastScrollbarUnderMouse(Scrollbar* scrollbar, bool setLast)
{
    if (m_lastScrollbarUnderMouse != scrollbar) {
        // Send mouse exited to the old scrollbar.
        if (m_lastScrollbarUnderMouse)
            m_lastScrollbarUnderMouse->mouseExited();

        // Send mouse entered if we're setting a new scrollbar.
        if (scrollbar && setLast)
            scrollbar->mouseEntered();

        m_lastScrollbarUnderMouse = setLast ? scrollbar : 0;
    }
}

static const AtomicString& eventNameForTouchPointState(PlatformTouchPoint::State state)
{
    switch (state) {
    case PlatformTouchPoint::TouchReleased:
        return EventTypeNames::touchend;
    case PlatformTouchPoint::TouchCancelled:
        return EventTypeNames::touchcancel;
    case PlatformTouchPoint::TouchPressed:
        return EventTypeNames::touchstart;
    case PlatformTouchPoint::TouchMoved:
        return EventTypeNames::touchmove;
    case PlatformTouchPoint::TouchStationary:
        // TouchStationary state is not converted to touch events, so fall through to assert.
    default:
        ASSERT_NOT_REACHED();
        return emptyAtom;
    }
}

HitTestResult EventHandler::hitTestResultInFrame(LocalFrame* frame, const LayoutPoint& point, HitTestRequest::HitTestRequestType hitType)
{
    HitTestResult result(point);

    if (!frame || !frame->contentRenderer())
        return result;
    if (frame->view()) {
        IntRect rect = frame->view()->visibleContentRect();
        if (!rect.contains(roundedIntPoint(point)))
            return result;
    }
    frame->contentRenderer()->hitTest(HitTestRequest(hitType), result);
    return result;
}

bool EventHandler::handleTouchEvent(const PlatformTouchEvent& event)
{
    TRACE_EVENT0("webkit", "EventHandler::handleTouchEvent");

    const Vector<PlatformTouchPoint>& points = event.touchPoints();

    UserGestureIndicator gestureIndicator(DefinitelyProcessingUserGesture);

    unsigned i;
    bool freshTouchEvents = true;
    bool allTouchReleased = true;
    for (i = 0; i < points.size(); ++i) {
        const PlatformTouchPoint& point = points[i];
        if (point.state() != PlatformTouchPoint::TouchPressed)
            freshTouchEvents = false;
        if (point.state() != PlatformTouchPoint::TouchReleased && point.state() != PlatformTouchPoint::TouchCancelled)
            allTouchReleased = false;
    }
    if (freshTouchEvents) {
        // Ideally we'd ASSERT !m_touchSequenceDocument here since we should
        // have cleared the active document when we saw the last release. But we
        // have some tests that violate this, ClusterFuzz could trigger it, and
        // there may be cases where the browser doesn't reliably release all
        // touches. http://crbug.com/345372 tracks this.
        m_touchSequenceDocument.clear();
    }

    // First do hit tests for any new touch points.
    for (i = 0; i < points.size(); ++i) {
        const PlatformTouchPoint& point = points[i];
        LayoutPoint pagePoint = documentPointForWindowPoint(m_frame, point.pos());

        // Touch events implicitly capture to the touched node, and don't change
        // active/hover states themselves (Gesture events do). So we only need
        // to hit-test on touchstart, and it can be read-only.
        if (point.state() == PlatformTouchPoint::TouchPressed) {
            HitTestRequest::HitTestRequestType hitType = HitTestRequest::TouchEvent | HitTestRequest::ReadOnly | HitTestRequest::Active;
            HitTestResult result;
            if (!m_touchSequenceDocument) {
                result = hitTestResultAtPoint(pagePoint, hitType);
            } else if (m_touchSequenceDocument->frame()) {
                LayoutPoint pagePointInOriginatingDocument = documentPointForWindowPoint(m_touchSequenceDocument->frame(), point.pos());
                result = hitTestResultInFrame(m_touchSequenceDocument->frame(), pagePointInOriginatingDocument, hitType);
            } else
                continue;

            Node* node = result.innerNode();
            if (!node)
                continue;

            // Touch events should not go to text nodes
            if (node->isTextNode())
                node = NodeRenderingTraversal::parent(node);

            if (!m_touchSequenceDocument) {
                // Keep track of which document should receive all touch events
                // in the active sequence. This must be a single document to
                // ensure we don't leak Nodes between documents.
                m_touchSequenceDocument = &(result.innerNode()->document());
            }

            // Ideally we'd ASSERT(!m_targetForTouchID.contains(point.id())
            // since we shouldn't get a touchstart for a touch that's already
            // down. However EventSender allows this to be violated and there's
            // some tests that take advantage of it. There may also be edge
            // cases in the browser where this happens.
            // See http://crbug.com/345372.
            m_targetForTouchID.set(point.id(), node);

            TouchAction effectiveTouchAction = computeEffectiveTouchAction(pagePoint);
            if (effectiveTouchAction != TouchActionAuto)
                m_frame->page()->chrome().client().setTouchAction(effectiveTouchAction);
        }
    }

    m_touchPressed = !allTouchReleased;

    // If there's no document receiving touch events, or no handlers on the
    // document set to receive the events, then we can skip all the rest of
    // this work.
    if (!m_touchSequenceDocument || !m_touchSequenceDocument->hasTouchEventHandlers() || !m_touchSequenceDocument->frame()) {
        if (allTouchReleased)
            m_touchSequenceDocument.clear();
        return false;
    }

    // Build up the lists to use for the 'touches', 'targetTouches' and
    // 'changedTouches' attributes in the JS event. See
    // http://www.w3.org/TR/touch-events/#touchevent-interface for how these
    // lists fit together.

    // Holds the complete set of touches on the screen.
    RefPtrWillBeRawPtr<TouchList> touches = TouchList::create();

    // A different view on the 'touches' list above, filtered and grouped by
    // event target. Used for the 'targetTouches' list in the JS event.
    typedef WillBeHeapHashMap<EventTarget*, RefPtrWillBeMember<TouchList> > TargetTouchesHeapMap;
    TargetTouchesHeapMap touchesByTarget;

    // Array of touches per state, used to assemble the 'changedTouches' list.
    typedef HashSet<RefPtr<EventTarget> > EventTargetSet;
    struct {
        // The touches corresponding to the particular change state this struct
        // instance represents.
        RefPtrWillBeMember<TouchList> m_touches;
        // Set of targets involved in m_touches.
        EventTargetSet m_targets;
    } changedTouches[PlatformTouchPoint::TouchStateEnd];

    for (i = 0; i < points.size(); ++i) {
        const PlatformTouchPoint& point = points[i];
        LayoutPoint pagePoint = documentPointForWindowPoint(m_frame, point.pos());
        PlatformTouchPoint::State pointState = point.state();
        RefPtr<EventTarget> touchTarget;

        if (pointState == PlatformTouchPoint::TouchReleased || pointState == PlatformTouchPoint::TouchCancelled) {
            // The target should be the original target for this touch, so get
            // it from the hashmap. As it's a release or cancel we also remove
            // it from the map.
            touchTarget = m_targetForTouchID.take(point.id());
        } else {
            // No hittest is performed on move or stationary, since the target
            // is not allowed to change anyway.
            touchTarget = m_targetForTouchID.get(point.id());
        }

        LocalFrame* targetFrame;
        bool knownTarget;
        if (touchTarget) {
            Document& doc = touchTarget->toNode()->document();
            ASSERT(&doc == m_touchSequenceDocument.get());
            targetFrame = doc.frame();
            knownTarget = true;
        } else {
            // If we don't have a target registered for the point it means we've
            // missed our opportunity to do a hit test for it (due to some
            // optimization that prevented blink from ever seeing the
            // touchstart), or that the touch started outside the active touch
            // sequence document. We should still include the touch in the
            // Touches list reported to the application (eg. so it can
            // differentiate between a one and two finger gesture), but we won't
            // actually dispatch any events for it. Set the target to the
            // Document so that there's some valid node here. Perhaps this
            // should really be DOMWindow, but in all other cases the target of
            // a Touch is a Node so using the window could be a breaking change.
            // Since we know there was no handler invoked, the specific target
            // should be completely irrelevant to the application.
            // FIXME: Oilpan: We can remove the following .get() if EventTarget
            // is on-heap.
            touchTarget = m_touchSequenceDocument.get();
            targetFrame = m_touchSequenceDocument->frame();
            knownTarget = false;
        }
        ASSERT(targetFrame);

        if (m_frame != targetFrame) {
            // pagePoint should always be relative to the target elements
            // containing frame.
            pagePoint = documentPointForWindowPoint(targetFrame, point.pos());
        }

        float scaleFactor = targetFrame->pageZoomFactor();

        int adjustedPageX = lroundf(pagePoint.x() / scaleFactor);
        int adjustedPageY = lroundf(pagePoint.y() / scaleFactor);
        int adjustedRadiusX = lroundf(point.radiusX() / scaleFactor);
        int adjustedRadiusY = lroundf(point.radiusY() / scaleFactor);

        RefPtrWillBeRawPtr<Touch> touch = Touch::create(targetFrame, touchTarget.get(), point.id(),
                                            point.screenPos().x(), point.screenPos().y(),
                                            adjustedPageX, adjustedPageY,
                                            adjustedRadiusX, adjustedRadiusY,
                                            point.rotationAngle(), point.force());

        // Ensure this target's touch list exists, even if it ends up empty, so
        // it can always be passed to TouchEvent::Create below.
        TargetTouchesHeapMap::iterator targetTouchesIterator = touchesByTarget.find(touchTarget.get());
        if (targetTouchesIterator == touchesByTarget.end()) {
            touchesByTarget.set(touchTarget.get(), TouchList::create());
            targetTouchesIterator = touchesByTarget.find(touchTarget.get());
        }

        // touches and targetTouches should only contain information about
        // touches still on the screen, so if this point is released or
        // cancelled it will only appear in the changedTouches list.
        if (pointState != PlatformTouchPoint::TouchReleased && pointState != PlatformTouchPoint::TouchCancelled) {
            touches->append(touch);
            targetTouchesIterator->value->append(touch);
        }

        // Now build up the correct list for changedTouches.
        // Note that  any touches that are in the TouchStationary state (e.g. if
        // the user had several points touched but did not move them all) should
        // never be in the changedTouches list so we do not handle them
        // explicitly here. See https://bugs.webkit.org/show_bug.cgi?id=37609
        // for further discussion about the TouchStationary state.
        if (pointState != PlatformTouchPoint::TouchStationary && knownTarget) {
            ASSERT(pointState < PlatformTouchPoint::TouchStateEnd);
            if (!changedTouches[pointState].m_touches)
                changedTouches[pointState].m_touches = TouchList::create();
            changedTouches[pointState].m_touches->append(touch);
            changedTouches[pointState].m_targets.add(touchTarget);
        }
    }
    if (allTouchReleased)
        m_touchSequenceDocument.clear();

    // Now iterate the changedTouches list and m_targets within it, sending
    // events to the targets as required.
    bool swallowedEvent = false;
    for (unsigned state = 0; state != PlatformTouchPoint::TouchStateEnd; ++state) {
        if (!changedTouches[state].m_touches)
            continue;

        const AtomicString& stateName(eventNameForTouchPointState(static_cast<PlatformTouchPoint::State>(state)));
        const EventTargetSet& targetsForState = changedTouches[state].m_targets;
        for (EventTargetSet::const_iterator it = targetsForState.begin(); it != targetsForState.end(); ++it) {
            EventTarget* touchEventTarget = it->get();
            RefPtrWillBeRawPtr<TouchEvent> touchEvent = TouchEvent::create(
                touches.get(), touchesByTarget.get(touchEventTarget), changedTouches[state].m_touches.get(),
                stateName, touchEventTarget->toNode()->document().domWindow(),
                event.ctrlKey(), event.altKey(), event.shiftKey(), event.metaKey(), event.cancelable());
            touchEventTarget->toNode()->dispatchTouchEvent(touchEvent.get());
            swallowedEvent = swallowedEvent || touchEvent->defaultPrevented() || touchEvent->defaultHandled();
        }
    }

    return swallowedEvent;
}

TouchAction EventHandler::intersectTouchAction(TouchAction action1, TouchAction action2)
{
    if (action1 == TouchActionNone || action2 == TouchActionNone)
        return TouchActionNone;
    if (action1 == TouchActionAuto)
        return action2;
    if (action2 == TouchActionAuto)
        return action1;
    if (!(action1 & action2))
        return TouchActionNone;
    return action1 & action2;
}

TouchAction EventHandler::computeEffectiveTouchAction(const LayoutPoint& point)
{
    // Optimization to minimize risk of this new feature (behavior should be identical
    // since there's no way to get non-default touch-action values).
    if (!RuntimeEnabledFeatures::cssTouchActionEnabled())
        return TouchActionAuto;

    HitTestResult taResult = hitTestResultAtPoint(point, HitTestRequest::TouchEvent | HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::TouchAction);
    Node* node = taResult.innerNode();
    if (!node)
        return TouchActionAuto;

    // Start by permitting all actions, then walk the elements supporting
    // touch-action from the target node up to the nearest scrollable ancestor
    // and exclude any prohibited actions.
    TouchAction effectiveTouchAction = TouchActionAuto;
    for (const Node* curNode = node; curNode; curNode = NodeRenderingTraversal::parent(curNode)) {
        if (RenderObject* renderer = curNode->renderer()) {
            if (renderer->visibleForTouchAction()) {
                TouchAction action = renderer->style()->touchAction();
                effectiveTouchAction = intersectTouchAction(action, effectiveTouchAction);
                if (effectiveTouchAction == TouchActionNone)
                    break;
            }

            // If we've reached an ancestor that supports a touch action, search no further.
            if (renderer->isBox() && toRenderBox(renderer)->scrollsOverflow())
                break;
        }
    }
    return effectiveTouchAction;
}

void EventHandler::setLastKnownMousePosition(const PlatformMouseEvent& event)
{
    m_mousePositionIsUnknown = false;
    m_lastKnownMousePosition = event.position();
    m_lastKnownMouseGlobalPosition = event.globalPosition();
}

bool EventHandler::passMousePressEventToSubframe(MouseEventWithHitTestResults& mev, LocalFrame* subframe)
{
    // If we're clicking into a frame that is selected, the frame will appear
    // greyed out even though we're clicking on the selection.  This looks
    // really strange (having the whole frame be greyed out), so we deselect the
    // selection.
    IntPoint p = m_frame->view()->windowToContents(mev.event().position());
    if (m_frame->selection().contains(p)) {
        VisiblePosition visiblePos(
            mev.targetNode()->renderer()->positionForPoint(mev.localPoint()));
        VisibleSelection newSelection(visiblePos);
        m_frame->selection().setSelection(newSelection);
    }

    subframe->eventHandler().handleMousePressEvent(mev.event());
    return true;
}

bool EventHandler::passMouseMoveEventToSubframe(MouseEventWithHitTestResults& mev, LocalFrame* subframe, HitTestResult* hoveredNode)
{
    if (m_mouseDownMayStartDrag && !m_mouseDownWasInSubframe)
        return false;
    subframe->eventHandler().handleMouseMoveOrLeaveEvent(mev.event(), hoveredNode);
    return true;
}

bool EventHandler::passMouseReleaseEventToSubframe(MouseEventWithHitTestResults& mev, LocalFrame* subframe)
{
    subframe->eventHandler().handleMouseReleaseEvent(mev.event());
    return true;
}

bool EventHandler::passWheelEventToWidget(const PlatformWheelEvent& wheelEvent, Widget* widget)
{
    // We can sometimes get a null widget!  EventHandlerMac handles a null
    // widget by returning false, so we do the same.
    if (!widget)
        return false;

    // If not a FrameView, then probably a plugin widget.  Those will receive
    // the event via an EventTargetNode dispatch when this returns false.
    if (!widget->isFrameView())
        return false;

    return toFrameView(widget)->frame().eventHandler().handleWheelEvent(wheelEvent);
}

bool EventHandler::passWidgetMouseDownEventToWidget(const MouseEventWithHitTestResults& event)
{
    // Figure out which view to send the event to.
    if (!event.targetNode() || !event.targetNode()->renderer() || !event.targetNode()->renderer()->isWidget())
        return false;
    return false;
}

PassRefPtrWillBeRawPtr<Clipboard> EventHandler::createDraggingClipboard() const
{
    return Clipboard::create(Clipboard::DragAndDrop, ClipboardWritable, DataObject::create());
}

void EventHandler::focusDocumentView()
{
    Page* page = m_frame->page();
    if (!page)
        return;
    page->focusController().focusDocumentView(m_frame);
}

unsigned EventHandler::accessKeyModifiers()
{
#if OS(MACOSX)
    return PlatformEvent::CtrlKey | PlatformEvent::AltKey;
#else
    return PlatformEvent::AltKey;
#endif
}

} // namespace WebCore
