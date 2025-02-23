// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "web/WebRemoteFrameImpl.h"

#include "core/frame/RemoteFrame.h"
#include "public/platform/WebFloatRect.h"
#include "public/platform/WebRect.h"
#include "public/web/WebDocument.h"
#include "public/web/WebPerformance.h"
#include "public/web/WebRange.h"
#include <v8/include/v8.h>

namespace blink {

WebRemoteFrame* WebRemoteFrame::create(WebFrameClient*)
{
    return adoptRef(new WebRemoteFrameImpl()).leakRef();
}

WebRemoteFrameImpl::WebRemoteFrameImpl()
    : m_frameClient(this)
{
}

WebRemoteFrameImpl::~WebRemoteFrameImpl()
{
}

bool WebRemoteFrameImpl::isWebLocalFrame() const
{
    return false;
}

WebLocalFrame* WebRemoteFrameImpl::toWebLocalFrame()
{
    ASSERT_NOT_REACHED();
    return 0;
}

bool WebRemoteFrameImpl::isWebRemoteFrame() const
{
    return true;
}

WebRemoteFrame* WebRemoteFrameImpl::toWebRemoteFrame()
{
    return this;
}

void WebRemoteFrameImpl::close()
{
    ASSERT_NOT_REACHED();
}

WebString WebRemoteFrameImpl::uniqueName() const
{
    ASSERT_NOT_REACHED();
    return WebString();
}

WebString WebRemoteFrameImpl::assignedName() const
{
    ASSERT_NOT_REACHED();
    return WebString();
}

void WebRemoteFrameImpl::setName(const WebString&)
{
    ASSERT_NOT_REACHED();
}

WebVector<WebIconURL> WebRemoteFrameImpl::iconURLs(int iconTypesMask) const
{
    ASSERT_NOT_REACHED();
    return WebVector<WebIconURL>();
}

void WebRemoteFrameImpl::setIsRemote(bool)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::setRemoteWebLayer(WebLayer*)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::setPermissionClient(WebPermissionClient*)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::setSharedWorkerRepositoryClient(WebSharedWorkerRepositoryClient*)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::setCanHaveScrollbars(bool)
{
    ASSERT_NOT_REACHED();
}

WebSize WebRemoteFrameImpl::scrollOffset() const
{
    ASSERT_NOT_REACHED();
    return WebSize();
}

void WebRemoteFrameImpl::setScrollOffset(const WebSize&)
{
    ASSERT_NOT_REACHED();
}

WebSize WebRemoteFrameImpl::minimumScrollOffset() const
{
    ASSERT_NOT_REACHED();
    return WebSize();
}

WebSize WebRemoteFrameImpl::maximumScrollOffset() const
{
    ASSERT_NOT_REACHED();
    return WebSize();
}

WebSize WebRemoteFrameImpl::contentsSize() const
{
    ASSERT_NOT_REACHED();
    return WebSize();
}

bool WebRemoteFrameImpl::hasVisibleContent() const
{
    ASSERT_NOT_REACHED();
    return false;
}

WebRect WebRemoteFrameImpl::visibleContentRect() const
{
    ASSERT_NOT_REACHED();
    return WebRect();
}

bool WebRemoteFrameImpl::hasHorizontalScrollbar() const
{
    ASSERT_NOT_REACHED();
    return false;
}

bool WebRemoteFrameImpl::hasVerticalScrollbar() const
{
    ASSERT_NOT_REACHED();
    return false;
}

WebView* WebRemoteFrameImpl::view() const
{
    ASSERT_NOT_REACHED();
    return 0;
}

WebFrame* WebRemoteFrameImpl::traversePrevious(bool wrap) const
{
    ASSERT_NOT_REACHED();
    return 0;
}

WebFrame* WebRemoteFrameImpl::traverseNext(bool wrap) const
{
    ASSERT_NOT_REACHED();
    return 0;
}

WebFrame* WebRemoteFrameImpl::findChildByName(const WebString&) const
{
    ASSERT_NOT_REACHED();
    return 0;
}

WebDocument WebRemoteFrameImpl::document() const
{
    ASSERT_NOT_REACHED();
    return WebDocument();
}

WebPerformance WebRemoteFrameImpl::performance() const
{
    ASSERT_NOT_REACHED();
    return WebPerformance();
}

bool WebRemoteFrameImpl::dispatchBeforeUnloadEvent()
{
    ASSERT_NOT_REACHED();
    return false;
}

void WebRemoteFrameImpl::dispatchUnloadEvent()
{
    ASSERT_NOT_REACHED();
}

NPObject* WebRemoteFrameImpl::windowObject() const
{
    ASSERT_NOT_REACHED();
    return 0;
}

void WebRemoteFrameImpl::bindToWindowObject(const WebString& name, NPObject*)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::bindToWindowObject(const WebString& name, NPObject*, void*)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::executeScript(const WebScriptSource&)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::executeScriptInIsolatedWorld(
    int worldID, const WebScriptSource* sources, unsigned numSources,
    int extensionGroup)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::setIsolatedWorldSecurityOrigin(int worldID, const WebSecurityOrigin&)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::setIsolatedWorldContentSecurityPolicy(int worldID, const WebString&)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::addMessageToConsole(const WebConsoleMessage&)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::collectGarbage()
{
    ASSERT_NOT_REACHED();
}

bool WebRemoteFrameImpl::checkIfRunInsecureContent(const WebURL&) const
{
    ASSERT_NOT_REACHED();
    return false;
}

v8::Handle<v8::Value> WebRemoteFrameImpl::executeScriptAndReturnValue(
    const WebScriptSource&)
{
    ASSERT_NOT_REACHED();
    return v8::Handle<v8::Value>();
}

void WebRemoteFrameImpl::executeScriptInIsolatedWorld(
    int worldID, const WebScriptSource* sourcesIn, unsigned numSources,
    int extensionGroup, WebVector<v8::Local<v8::Value> >* results)
{
    ASSERT_NOT_REACHED();
}

v8::Handle<v8::Value> WebRemoteFrameImpl::callFunctionEvenIfScriptDisabled(
    v8::Handle<v8::Function>,
    v8::Handle<v8::Value>,
    int argc,
    v8::Handle<v8::Value> argv[])
{
    ASSERT_NOT_REACHED();
    return v8::Handle<v8::Value>();
}

v8::Local<v8::Context> WebRemoteFrameImpl::mainWorldScriptContext() const
{
    ASSERT_NOT_REACHED();
    return v8::Local<v8::Context>();
}

void WebRemoteFrameImpl::reload(bool ignoreCache)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::reloadWithOverrideURL(const WebURL& overrideUrl, bool ignoreCache)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::loadRequest(const WebURLRequest&)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::loadHistoryItem(const WebHistoryItem&, WebHistoryLoadType, WebURLRequest::CachePolicy)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::loadData(
    const WebData&, const WebString& mimeType, const WebString& textEncoding,
    const WebURL& baseURL, const WebURL& unreachableURL, bool replace)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::loadHTMLString(
    const WebData& html, const WebURL& baseURL, const WebURL& unreachableURL,
    bool replace)
{
    ASSERT_NOT_REACHED();
}

bool WebRemoteFrameImpl::isLoading() const
{
    ASSERT_NOT_REACHED();
    return false;
}

void WebRemoteFrameImpl::stopLoading()
{
    ASSERT_NOT_REACHED();
}

WebDataSource* WebRemoteFrameImpl::provisionalDataSource() const
{
    ASSERT_NOT_REACHED();
    return 0;
}

WebDataSource* WebRemoteFrameImpl::dataSource() const
{
    ASSERT_NOT_REACHED();
    return 0;
}

void WebRemoteFrameImpl::enableViewSourceMode(bool enable)
{
    ASSERT_NOT_REACHED();
}

bool WebRemoteFrameImpl::isViewSourceModeEnabled() const
{
    ASSERT_NOT_REACHED();
    return false;
}

void WebRemoteFrameImpl::setReferrerForRequest(WebURLRequest&, const WebURL& referrer)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::dispatchWillSendRequest(WebURLRequest&)
{
    ASSERT_NOT_REACHED();
}

WebURLLoader* WebRemoteFrameImpl::createAssociatedURLLoader(const WebURLLoaderOptions&)
{
    ASSERT_NOT_REACHED();
    return 0;
}

unsigned WebRemoteFrameImpl::unloadListenerCount() const
{
    ASSERT_NOT_REACHED();
    return 0;
}

void WebRemoteFrameImpl::replaceSelection(const WebString&)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::insertText(const WebString&)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::setMarkedText(const WebString&, unsigned location, unsigned length)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::unmarkText()
{
    ASSERT_NOT_REACHED();
}

bool WebRemoteFrameImpl::hasMarkedText() const
{
    ASSERT_NOT_REACHED();
    return false;
}

WebRange WebRemoteFrameImpl::markedRange() const
{
    ASSERT_NOT_REACHED();
    return WebRange();
}

bool WebRemoteFrameImpl::firstRectForCharacterRange(unsigned location, unsigned length, WebRect&) const
{
    ASSERT_NOT_REACHED();
    return false;
}

size_t WebRemoteFrameImpl::characterIndexForPoint(const WebPoint&) const
{
    ASSERT_NOT_REACHED();
    return 0;
}

bool WebRemoteFrameImpl::executeCommand(const WebString&, const WebNode&)
{
    ASSERT_NOT_REACHED();
    return false;
}

bool WebRemoteFrameImpl::executeCommand(const WebString&, const WebString& value, const WebNode&)
{
    ASSERT_NOT_REACHED();
    return false;
}

bool WebRemoteFrameImpl::isCommandEnabled(const WebString&) const
{
    ASSERT_NOT_REACHED();
    return false;
}

void WebRemoteFrameImpl::enableContinuousSpellChecking(bool)
{
    ASSERT_NOT_REACHED();
}

bool WebRemoteFrameImpl::isContinuousSpellCheckingEnabled() const
{
    ASSERT_NOT_REACHED();
    return false;
}

void WebRemoteFrameImpl::requestTextChecking(const WebElement&)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::replaceMisspelledRange(const WebString&)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::removeSpellingMarkers()
{
    ASSERT_NOT_REACHED();
}

bool WebRemoteFrameImpl::hasSelection() const
{
    ASSERT_NOT_REACHED();
    return false;
}

WebRange WebRemoteFrameImpl::selectionRange() const
{
    ASSERT_NOT_REACHED();
    return WebRange();
}

WebString WebRemoteFrameImpl::selectionAsText() const
{
    ASSERT_NOT_REACHED();
    return WebString();
}

WebString WebRemoteFrameImpl::selectionAsMarkup() const
{
    ASSERT_NOT_REACHED();
    return WebString();
}

bool WebRemoteFrameImpl::selectWordAroundCaret()
{
    ASSERT_NOT_REACHED();
    return false;
}

void WebRemoteFrameImpl::selectRange(const WebPoint& base, const WebPoint& extent)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::selectRange(const WebRange&)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::moveRangeSelection(const WebPoint& base, const WebPoint& extent)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::moveCaretSelection(const WebPoint&)
{
    ASSERT_NOT_REACHED();
}

bool WebRemoteFrameImpl::setEditableSelectionOffsets(int start, int end)
{
    ASSERT_NOT_REACHED();
    return false;
}

bool WebRemoteFrameImpl::setCompositionFromExistingText(int compositionStart, int compositionEnd, const WebVector<WebCompositionUnderline>& underlines)
{
    ASSERT_NOT_REACHED();
    return false;
}

void WebRemoteFrameImpl::extendSelectionAndDelete(int before, int after)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::addStyleSheetByURL(const WebString& url)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::setCaretVisible(bool)
{
    ASSERT_NOT_REACHED();
}

int WebRemoteFrameImpl::printBegin(const WebPrintParams&, const WebNode& constrainToNode)
{
    ASSERT_NOT_REACHED();
    return 0;
}

float WebRemoteFrameImpl::printPage(int pageToPrint, WebCanvas*)
{
    ASSERT_NOT_REACHED();
    return 0.0;
}

float WebRemoteFrameImpl::getPrintPageShrink(int page)
{
    ASSERT_NOT_REACHED();
    return 0.0;
}

void WebRemoteFrameImpl::printEnd()
{
    ASSERT_NOT_REACHED();
}

bool WebRemoteFrameImpl::isPrintScalingDisabledForPlugin(const WebNode&)
{
    ASSERT_NOT_REACHED();
    return false;
}

bool WebRemoteFrameImpl::hasCustomPageSizeStyle(int pageIndex)
{
    ASSERT_NOT_REACHED();
    return false;
}

bool WebRemoteFrameImpl::isPageBoxVisible(int pageIndex)
{
    ASSERT_NOT_REACHED();
    return false;
}

void WebRemoteFrameImpl::pageSizeAndMarginsInPixels(
    int pageIndex,
    WebSize& pageSize,
    int& marginTop,
    int& marginRight,
    int& marginBottom,
    int& marginLeft)
{
    ASSERT_NOT_REACHED();
}

WebString WebRemoteFrameImpl::pageProperty(const WebString& propertyName, int pageIndex)
{
    ASSERT_NOT_REACHED();
    return WebString();
}

void WebRemoteFrameImpl::printPagesWithBoundaries(WebCanvas*, const WebSize&)
{
    ASSERT_NOT_REACHED();
}

bool WebRemoteFrameImpl::find(
    int identifier, const WebString& searchText, const WebFindOptions&,
    bool wrapWithinFrame, WebRect* selectionRect)
{
    ASSERT_NOT_REACHED();
    return false;
}

void WebRemoteFrameImpl::stopFinding(bool clearSelection)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::scopeStringMatches(
    int identifier, const WebString& searchText, const WebFindOptions&,
    bool reset)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::cancelPendingScopingEffort()
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::increaseMatchCount(int count, int identifier)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::resetMatchCount()
{
    ASSERT_NOT_REACHED();
}

int WebRemoteFrameImpl::findMatchMarkersVersion() const
{
    ASSERT_NOT_REACHED();
    return 0;
}

WebFloatRect WebRemoteFrameImpl::activeFindMatchRect()
{
    ASSERT_NOT_REACHED();
    return WebFloatRect();
}

void WebRemoteFrameImpl::findMatchRects(WebVector<WebFloatRect>&)
{
    ASSERT_NOT_REACHED();
}

int WebRemoteFrameImpl::selectNearestFindMatch(const WebFloatPoint&, WebRect* selectionRect)
{
    ASSERT_NOT_REACHED();
    return 0;
}

void WebRemoteFrameImpl::setTickmarks(const WebVector<WebRect>&)
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::sendOrientationChangeEvent()
{
    ASSERT_NOT_REACHED();
}

void WebRemoteFrameImpl::dispatchMessageEventWithOriginCheck(
    const WebSecurityOrigin& intendedTargetOrigin,
    const WebDOMEvent&)
{
    ASSERT_NOT_REACHED();
}

WebString WebRemoteFrameImpl::contentAsText(size_t maxChars) const
{
    ASSERT_NOT_REACHED();
    return WebString();
}

WebString WebRemoteFrameImpl::contentAsMarkup() const
{
    ASSERT_NOT_REACHED();
    return WebString();
}

WebString WebRemoteFrameImpl::renderTreeAsText(RenderAsTextControls toShow) const
{
    ASSERT_NOT_REACHED();
    return WebString();
}

WebString WebRemoteFrameImpl::markerTextForListItem(const WebElement&) const
{
    ASSERT_NOT_REACHED();
    return WebString();
}

WebRect WebRemoteFrameImpl::selectionBoundsRect() const
{
    ASSERT_NOT_REACHED();
    return WebRect();
}

bool WebRemoteFrameImpl::selectionStartHasSpellingMarkerFor(int from, int length) const
{
    ASSERT_NOT_REACHED();
    return false;
}

WebString WebRemoteFrameImpl::layerTreeAsText(bool showDebugInfo) const
{
    ASSERT_NOT_REACHED();
    return WebString();
}

} // namespace blink

