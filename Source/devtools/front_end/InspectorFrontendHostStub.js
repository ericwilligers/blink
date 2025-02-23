/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @implements {InspectorFrontendHostAPI}
 */
WebInspector.InspectorFrontendHostStub = function()
{
    this.isStub = true;
}

WebInspector.InspectorFrontendHostStub.prototype = {
    /**
     * @return {string}
     */
    getSelectionBackgroundColor: function()
    {
        return "#6e86ff";
    },

    /**
     * @return {string}
     */
    getSelectionForegroundColor: function()
    {
        return "#ffffff";
    },

    /**
     * @return {string}
     */
    platform: function()
    {
        var match = navigator.userAgent.match(/Windows NT/);
        if (match)
            return "windows";
        match = navigator.userAgent.match(/Mac OS X/);
        if (match)
            return "mac";
        return "linux";
    },

    /**
     * @return {string}
     */
    port: function()
    {
        return "unknown";
    },

    bringToFront: function()
    {
        this._windowVisible = true;
    },

    closeWindow: function()
    {
        this._windowVisible = false;
    },

    setIsDocked: function(isDocked, callback)
    {
    },

    /**
     * Requests inspected page to be placed atop of the inspector frontend with specified bounds.
     * @param {{x: number, y: number, width: number, height: number}} bounds
     */
    setInspectedPageBounds: function(bounds)
    {
    },

    /**
     * Requests inspected page to be placed atop of the inspector frontend
     * with passed insets from the frontend sides, respecting minimum size passed.
     * @param {{top: number, left: number, right: number, bottom: number}} insets
     * @param {{width: number, height: number}} minSize
     */
    setContentsResizingStrategy: function(insets, minSize)
    {
    },

    inspectElementCompleted: function()
    {
    },

    moveWindowBy: function(x, y)
    {
    },

    setInjectedScriptForOrigin: function(origin, script)
    {
    },

    inspectedURLChanged: function(url)
    {
        document.title = WebInspector.UIString("Developer Tools - %s", url);
    },

    copyText: function(text)
    {
        WebInspector.console.log("Clipboard is not enabled in hosted mode. Please inspect using chrome://inspect", WebInspector.ConsoleMessage.MessageLevel.Error, true);
    },

    openInNewTab: function(url)
    {
        window.open(url, "_blank");
    },

    save: function(url, content, forceSaveAs)
    {
        WebInspector.console.log("Saving files is not enabled in hosted mode. Please inspect using chrome://inspect", WebInspector.ConsoleMessage.MessageLevel.Error, true);
        WebInspector.fileManager.canceledSaveURL(url);
    },

    append: function(url, content)
    {
        WebInspector.console.log("Saving files is not enabled in hosted mode. Please inspect using chrome://inspect", WebInspector.ConsoleMessage.MessageLevel.Error, true);
    },

    sendMessageToBackend: function(message)
    {
    },

    sendMessageToEmbedder: function(message)
    {
    },

    recordActionTaken: function(actionCode)
    {
    },

    recordPanelShown: function(panelCode)
    {
    },

    requestFileSystems: function()
    {
    },

    addFileSystem: function()
    {
    },

    removeFileSystem: function(fileSystemPath)
    {
    },

    /**
     * @param {string} fileSystemId
     * @param {string} registeredName
     * @return {?WebInspector.IsolatedFileSystem}
     */
    isolatedFileSystem: function(fileSystemId, registeredName)
    {
        return null;
    },

    upgradeDraggedFileSystemPermissions: function(domFileSystem)
    {
    },

    indexPath: function(requestId, fileSystemPath)
    {
    },

    stopIndexing: function(requestId)
    {
    },

    searchInPath: function(requestId, fileSystemPath, query)
    {
    },

    setZoomFactor: function(zoom)
    {
    },

    /**
     * @return {number}
     */
    zoomFactor: function()
    {
        return 1;
    },

    zoomIn: function()
    {
    },

    zoomOut: function()
    {
    },

    resetZoom: function()
    {
    },

    setWhitelistedShortcuts: function(shortcuts)
    {
    },

    /**
     * @return {boolean}
     */
    isUnderTest: function()
    {
        return false;
    },

    /**
     * @param {string} browserId
     * @param {string} url
     */
    openUrlOnRemoteDeviceAndInspect: function(browserId, url)
    {
    },

    /**
     * @param {string} eventType
     */
    subscribe: function(eventType)
    {
    },

    /**
     * @param {string} eventType
     */
    unsubscribe: function(eventType)
    {
    }
}

// Inherit bindings from the embedder.
if (top !== window) {
    window.InspectorFrontendHost = top.InspectorFrontendHost;
}

if (!window.InspectorFrontendHost) {
    InspectorFrontendHost = new WebInspector.InspectorFrontendHostStub();
} else {
    var proto = WebInspector.InspectorFrontendHostStub.prototype;
    for (var name in proto) {
        var value = proto[name];
        if (typeof value !== "function" || InspectorFrontendHost[name])
            continue;
        InspectorFrontendHost[name] = function(name) {
            var message = "Incompatible embedder: method InspectorFrontendHost." + name + " is missing. Using stub instead.";
            if (WebInspector.console)
                WebInspector.console.showErrorMessage(message);
            else
                console.error(message);
            var args = Array.prototype.slice.call(arguments, 1);
            return proto[name].apply(InspectorFrontendHost, args);
        }.bind(null, name);
    }
}
