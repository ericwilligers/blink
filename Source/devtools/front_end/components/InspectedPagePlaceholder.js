// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @constructor
 * @extends {WebInspector.View}
 */
WebInspector.InspectedPagePlaceholder = function()
{
    WebInspector.View.call(this);
    this.element.classList.add("white-background");
    WebInspector.zoomManager.addEventListener(WebInspector.ZoomManager.Events.ZoomChanged, this._onZoomChanged, this);
    this._margins = { top: 0, right: 0, bottom: 0, left: 0 };
    this.restoreMinimumSizeAndMargins();
    WebInspector.dockController.addEventListener(WebInspector.DockController.Events.BeforeDockSideChanged, this._beforeDockSideChange, this);
    WebInspector.dockController.addEventListener(WebInspector.DockController.Events.AfterDockSideChanged, this._afterDockSideChange, this);
};

WebInspector.InspectedPagePlaceholder.MarginValue = 3;

WebInspector.InspectedPagePlaceholder.prototype = {
    _findMargins: function()
    {
        var margins = { top: 0, right: 0, bottom: 0, left: 0 };

        if (this._useMargins) {
            var adjacent = { top: true, right: true, bottom: true, left: true };
            var view = this;
            while (view.parentView()) {
                var parent = view.parentView();
                // This view assumes it's always inside the main split view element, not a sidebar.
                // Every parent which is not a split view, must be of the same size as this view.
                if (parent instanceof WebInspector.SplitView) {
                    var side = parent.sidebarSide();
                    if (adjacent[side] && !parent.hasCustomResizer() && parent.isResizable())
                        margins[side] = WebInspector.InspectedPagePlaceholder.MarginValue;
                    adjacent[side] = false;
                }
                view = parent;
            }
        }

        if (this._margins.top !== margins.top || this._margins.left !== margins.left || this._margins.right !== margins.right || this._margins.bottom !== margins.bottom) {
            this._margins = margins;
            this._scheduleUpdate();
        }
    },

    _onZoomChanged: function()
    {
        this._scheduleUpdate();
    },

    onResize: function()
    {
        this._findMargins();
        this._scheduleUpdate();
    },

    _scheduleUpdate: function()
    {
        if (this._noUpdates)
            return;

        var dockSide = WebInspector.dockController.dockSide();
        if (dockSide !== WebInspector.DockController.State.Undocked) {
            if (this._updateId)
                window.cancelAnimationFrame(this._updateId);
            this._updateId = window.requestAnimationFrame(this._update.bind(this));
        }
    },

    /**
     * @return {!Size}
     */
    dipPageSize: function()
    {
        var rect = this._dipPageRect();
        return new Size(Math.round(rect.width), Math.round(rect.height));
    },

    /**
     * @return {!Size}
     */
    cssElementSize: function()
    {
        var zoomFactor = WebInspector.zoomManager.zoomFactor();
        var rect = this.element.getBoundingClientRect();
        var width = rect.width - (this._margins.left + this._margins.right) / zoomFactor;
        var height = rect.height - (this._margins.top + this._margins.bottom) / zoomFactor;
        return new Size(width, height);
    },

    restoreMinimumSizeAndMargins: function()
    {
        this._useMargins = true;
        this.setMinimumSize(50, 50);
        this._findMargins();
    },

    clearMinimumSizeAndMargins: function()
    {
        this._useMargins = false;
        this.setMinimumSize(1, 1);
        this._findMargins();
    },

    _dipPageRect: function()
    {
        var zoomFactor = WebInspector.zoomManager.zoomFactor();
        var rect = this.element.getBoundingClientRect();
        var bodyRect = document.body.getBoundingClientRect();

        var left = Math.max(rect.left * zoomFactor + this._margins.left, bodyRect.left * zoomFactor);
        var top = Math.max(rect.top * zoomFactor + this._margins.top, bodyRect.top * zoomFactor);
        var bottom = Math.min(rect.bottom * zoomFactor - this._margins.bottom, bodyRect.bottom * zoomFactor);
        var right = Math.min(rect.right * zoomFactor - this._margins.right, bodyRect.right * zoomFactor);

        return { x: left, y: top, width: right - left, height: bottom - top };
    },

    _update: function()
    {
        delete this._updateId;
        if (this._noUpdates)
            return;

        var rect = this._dipPageRect();
        var bounds = { x: Math.round(rect.x), y: Math.round(rect.y), height: Math.round(rect.height), width: Math.round(rect.width) };
        InspectorFrontendHost.setInspectedPageBounds(bounds);
    },

    _beforeDockSideChange: function()
    {
        this._noUpdates = true;
    },

    _afterDockSideChange: function()
    {
        this._noUpdates = false;
        this._scheduleUpdate();
    },

    __proto__: WebInspector.View.prototype
};
