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

var Capabilities = {
    isMainFrontend: false,
    canProfilePower: false,
}

/**
 * @constructor
 */
WebInspector.Settings = function()
{
    this._eventSupport = new WebInspector.Object();
    this._registry = /** @type {!Object.<string, !WebInspector.Setting>} */ ({});

    this.colorFormat = this.createSetting("colorFormat", "original");
    this.consoleHistory = this.createSetting("consoleHistory", []);
    this.domWordWrap = this.createSetting("domWordWrap", true);
    this.eventListenersFilter = this.createSetting("eventListenersFilter", "all");
    this.lastViewedScriptFile = this.createSetting("lastViewedScriptFile", "application");
    this.monitoringXHREnabled = this.createSetting("monitoringXHREnabled", false);
    this.preserveConsoleLog = this.createSetting("preserveConsoleLog", false);
    this.consoleTimestampsEnabled = this.createSetting("consoleTimestampsEnabled", false);
    this.resourcesLargeRows = this.createSetting("resourcesLargeRows", true);
    this.resourcesSortOptions = this.createSetting("resourcesSortOptions", {timeOption: "responseTime", sizeOption: "transferSize"});
    this.resourceViewTab = this.createSetting("resourceViewTab", "preview");
    this.showInheritedComputedStyleProperties = this.createSetting("showInheritedComputedStyleProperties", false);
    this.showUserAgentStyles = this.createSetting("showUserAgentStyles", true);
    this.watchExpressions = this.createSetting("watchExpressions", []);
    this.breakpoints = this.createSetting("breakpoints", []);
    this.eventListenerBreakpoints = this.createSetting("eventListenerBreakpoints", []);
    this.domBreakpoints = this.createSetting("domBreakpoints", []);
    this.xhrBreakpoints = this.createSetting("xhrBreakpoints", []);
    this.jsSourceMapsEnabled = this.createSetting("sourceMapsEnabled", true);
    this.cssSourceMapsEnabled = this.createSetting("cssSourceMapsEnabled", true);
    this.cacheDisabled = this.createSetting("cacheDisabled", false);
    this.showUAShadowDOM = this.createSetting("showUAShadowDOM", false);
    this.savedURLs = this.createSetting("savedURLs", {});
    this.javaScriptDisabled = this.createSetting("javaScriptDisabled", false);
    this.showAdvancedHeapSnapshotProperties = this.createSetting("showAdvancedHeapSnapshotProperties", false);
    this.highResolutionCpuProfiling = this.createSetting("highResolutionCpuProfiling", false);
    this.searchInContentScripts = this.createSetting("searchInContentScripts", false);
    this.textEditorIndent = this.createSetting("textEditorIndent", "    ");
    this.textEditorAutoDetectIndent = this.createSetting("textEditorAutoIndentIndent", true);
    this.textEditorAutocompletion = this.createSetting("textEditorAutocompletion", true);
    this.textEditorBracketMatching = this.createSetting("textEditorBracketMatching", true);
    this.cssReloadEnabled = this.createSetting("cssReloadEnabled", false);
    this.timelineLiveUpdate = this.createSetting("timelineLiveUpdate", true);
    this.showMetricsRulers = this.createSetting("showMetricsRulers", false);
    this.workerInspectorWidth = this.createSetting("workerInspectorWidth", 600);
    this.workerInspectorHeight = this.createSetting("workerInspectorHeight", 600);
    this.messageURLFilters = this.createSetting("messageURLFilters", {});
    this.networkHideDataURL = this.createSetting("networkHideDataURL", false);
    this.networkResourceTypeFilters = this.createSetting("networkResourceTypeFilters", {});
    this.messageLevelFilters = this.createSetting("messageLevelFilters", {});
    this.splitVerticallyWhenDockedToRight = this.createSetting("splitVerticallyWhenDockedToRight", true);
    this.visiblePanels = this.createSetting("visiblePanels", {});
    this.shortcutPanelSwitch = this.createSetting("shortcutPanelSwitch", false);
    this.showWhitespacesInEditor = this.createSetting("showWhitespacesInEditor", false);
    this.skipStackFramesSwitch = this.createSetting("skipStackFramesSwitch", false);
    this.skipStackFramesPattern = this.createRegExpSetting("skipStackFramesPattern", "");
    this.pauseOnExceptionEnabled = this.createSetting("pauseOnExceptionEnabled", false);
    this.pauseOnCaughtException = this.createSetting("pauseOnCaughtException", false);
    this.enableAsyncStackTraces = this.createSetting("enableAsyncStackTraces", false);
    this.responsiveDesignMode = this.createSetting("responsiveDesignMode", false);
}

WebInspector.Settings.prototype = {
    /**
     * @param {string} key
     * @param {*} defaultValue
     * @return {!WebInspector.Setting}
     */
    createSetting: function(key, defaultValue)
    {
        if (!this._registry[key])
            this._registry[key] = new WebInspector.Setting(key, defaultValue, this._eventSupport, window.localStorage);
        return this._registry[key];
    },

    /**
     * @param {string} key
     * @param {string} defaultValue
     * @param {string=} regexFlags
     * @return {!WebInspector.Setting}
     */
    createRegExpSetting: function(key, defaultValue, regexFlags)
    {
        if (!this._registry[key])
            this._registry[key] = new WebInspector.RegExpSetting(key, defaultValue, this._eventSupport, window.localStorage, regexFlags);
        return this._registry[key];
    },

    /**
     * @param {string} key
     * @param {*} defaultValue
     * @param {function(*, function(string, ...))} setterCallback
     * @return {!WebInspector.Setting}
     */
    createBackendSetting: function(key, defaultValue, setterCallback)
    {
        if (!this._registry[key])
            this._registry[key] = new WebInspector.BackendSetting(key, defaultValue, this._eventSupport, window.localStorage, setterCallback);
        return this._registry[key];
    },

    initializeBackendSettings: function()
    {
        this.showPaintRects = WebInspector.settings.createBackendSetting("showPaintRects", false, PageAgent.setShowPaintRects.bind(PageAgent));
        this.showDebugBorders = WebInspector.settings.createBackendSetting("showDebugBorders", false, PageAgent.setShowDebugBorders.bind(PageAgent));
        this.continuousPainting = WebInspector.settings.createBackendSetting("continuousPainting", false, PageAgent.setContinuousPaintingEnabled.bind(PageAgent));
        this.showFPSCounter = WebInspector.settings.createBackendSetting("showFPSCounter", false, PageAgent.setShowFPSCounter.bind(PageAgent));
        this.showScrollBottleneckRects = WebInspector.settings.createBackendSetting("showScrollBottleneckRects", false, PageAgent.setShowScrollBottleneckRects.bind(PageAgent));
    }
}

/**
 * @constructor
 * @param {string} name
 * @param {V} defaultValue
 * @param {!WebInspector.Object} eventSupport
 * @param {?Storage} storage
 * @template V
 */
WebInspector.Setting = function(name, defaultValue, eventSupport, storage)
{
    this._name = name;
    this._defaultValue = defaultValue;
    this._eventSupport = eventSupport;
    this._storage = storage;
}

WebInspector.Setting.prototype = {
    /**
     * @param {function(!WebInspector.Event)} listener
     * @param {!Object=} thisObject
     */
    addChangeListener: function(listener, thisObject)
    {
        this._eventSupport.addEventListener(this._name, listener, thisObject);
    },

    /**
     * @param {function(!WebInspector.Event)} listener
     * @param {!Object=} thisObject
     */
    removeChangeListener: function(listener, thisObject)
    {
        this._eventSupport.removeEventListener(this._name, listener, thisObject);
    },

    get name()
    {
        return this._name;
    },

    /**
     * @return {V}
     */
    get: function()
    {
        if (typeof this._value !== "undefined")
            return this._value;

        this._value = this._defaultValue;
        if (this._storage && this._name in this._storage) {
            try {
                this._value = JSON.parse(this._storage[this._name]);
            } catch(e) {
                delete this._storage[this._name];
            }
        }
        return this._value;
    },

    set: function(value)
    {
        this._value = value;
        if (this._storage) {
            try {
                this._storage[this._name] = JSON.stringify(value);
            } catch(e) {
                console.error("Error saving setting with name:" + this._name);
            }
        }
        this._eventSupport.dispatchEventToListeners(this._name, value);
    }
}

/**
 * @constructor
 * @extends {WebInspector.Setting}
 * @param {string} name
 * @param {string} defaultValue
 * @param {!WebInspector.Object} eventSupport
 * @param {?Storage} storage
 * @param {string=} regexFlags
 */
WebInspector.RegExpSetting = function(name, defaultValue, eventSupport, storage, regexFlags)
{
    WebInspector.Setting.call(this, name, defaultValue, eventSupport, storage);
    this._regexFlags = regexFlags;
}

WebInspector.RegExpSetting.prototype = {
    set: function(value)
    {
        delete this._regex;
        WebInspector.Setting.prototype.set.call(this, value);
    },

    /**
     * @return {?RegExp}
     */
    asRegExp: function()
    {
        if (typeof this._regex !== "undefined")
            return this._regex;
        this._regex = null;
        try {
            this._regex = new RegExp(this.get(), this._regexFlags || "");
        } catch (e) {
        }
        return this._regex;
    },

    __proto__: WebInspector.Setting.prototype
}

/**
 * @constructor
 * @extends {WebInspector.Setting}
 * @param {string} name
 * @param {*} defaultValue
 * @param {!WebInspector.Object} eventSupport
 * @param {?Storage} storage
 * @param {function(*,function(string, ...))} setterCallback
 */
WebInspector.BackendSetting = function(name, defaultValue, eventSupport, storage, setterCallback)
{
    WebInspector.Setting.call(this, name, defaultValue, eventSupport, storage);
    this._setterCallback = setterCallback;
    var currentValue = this.get();
    if (currentValue !== defaultValue)
        this.set(currentValue);
}

WebInspector.BackendSetting.prototype = {
    set: function(value)
    {
        /**
         * @param {?Protocol.Error} error
         * @this {WebInspector.BackendSetting}
         */
        function callback(error)
        {
            if (error) {
                WebInspector.console.log("Error applying setting " + this._name + ": " + error);
                this._eventSupport.dispatchEventToListeners(this._name, this._value);
                return;
            }
            WebInspector.Setting.prototype.set.call(this, value);
        }
        this._setterCallback(value, callback.bind(this));
    },

    __proto__: WebInspector.Setting.prototype
}

/**
 * @constructor
 * @param {boolean} experimentsEnabled
 */
WebInspector.ExperimentsSettings = function(experimentsEnabled)
{
    this._experimentsEnabled = experimentsEnabled;
    this._setting = WebInspector.settings.createSetting("experiments", {});
    this._experiments = [];
    this._enabledForTest = {};

    // Add currently running experiments here.
    this.applyCustomStylesheet = this._createExperiment("applyCustomStylesheet", "Allow custom UI themes");
    this.canvasInspection = this._createExperiment("canvasInspection ", "Canvas inspection");
    this.devicesPanel = this._createExperiment("devicesPanel", "Devices panel", true);
    this.dockToLeft = this._createExperiment("dockToLeft", "Dock to left", true);
    this.editorInDrawer = this._createExperiment("showEditorInDrawer", "Editor in drawer", true);
    this.fileSystemInspection = this._createExperiment("fileSystemInspection", "FileSystem inspection");
    this.frameworksDebuggingSupport = this._createExperiment("frameworksDebuggingSupport", "JavaScript frameworks debugging");
    this.gpuTimeline = this._createExperiment("gpuTimeline", "GPU data on timeline", true);
    this.heapAllocationProfiler = this._createExperiment("allocationProfiler", "Heap allocation profiler");
    this.heapSnapshotStatistics = this._createExperiment("heapSnapshotStatistics", "Heap snapshot statistics", true);
    this.layersPanel = this._createExperiment("layersPanel", "Layers panel", true);
    this.responsiveDesign = this._createExperiment("responsiveDesign", "Responsive design", true);
    this.timelineFlameChart = this._createExperiment("timelineFlameChart", "Timeline flame chart");
    this.timelineOnTraceEvents = this._createExperiment("timelineOnTraceEvents", "Timeline on trace events", true);
    this.timelinePowerProfiler = this._createExperiment("timelinePowerProfiler", "Timeline power profiler");
    this.timelineTracingMode = this._createExperiment("timelineTracingMode", "Timeline tracing mode");
    this.timelineJSCPUProfile = this._createExperiment("timelineJSCPUProfile", "Timeline with JS sampling");
    this.timelineNoLiveUpdate = this._createExperiment("timelineNoLiveUpdate", "Timeline w/o live update", true);
    this.workersInMainWindow = this._createExperiment("workersInMainWindow", "Workers in main window", true);

    this._cleanUpSetting();
}

WebInspector.ExperimentsSettings.prototype = {
    /**
     * @return {!Array.<!WebInspector.Experiment>}
     */
    get experiments()
    {
        return this._experiments.slice();
    },

    /**
     * @return {boolean}
     */
    get experimentsEnabled()
    {
        return this._experimentsEnabled;
    },

    /**
     * @param {string} experimentName
     * @param {string} experimentTitle
     * @param {boolean=} hidden
     * @return {!WebInspector.Experiment}
     */
    _createExperiment: function(experimentName, experimentTitle, hidden)
    {
        var experiment = new WebInspector.Experiment(this, experimentName, experimentTitle, !!hidden);
        this._experiments.push(experiment);
        return experiment;
    },

    /**
     * @param {string} experimentName
     * @return {boolean}
     */
    isEnabled: function(experimentName)
    {
        if (this._enabledForTest[experimentName])
            return true;

        if (!this.experimentsEnabled)
            return false;

        var experimentsSetting = this._setting.get();
        return experimentsSetting[experimentName];
    },

    /**
     * @param {string} experimentName
     * @param {boolean} enabled
     */
    setEnabled: function(experimentName, enabled)
    {
        var experimentsSetting = this._setting.get();
        experimentsSetting[experimentName] = enabled;
        this._setting.set(experimentsSetting);
    },

    /**
     * @param {string} experimentName
     */
    _enableForTest: function(experimentName)
    {
        this._enabledForTest[experimentName] = true;
    },

    _cleanUpSetting: function()
    {
        var experimentsSetting = this._setting.get();
        var cleanedUpExperimentSetting = {};
        for (var i = 0; i < this._experiments.length; ++i) {
            var experimentName = this._experiments[i].name;
            if (experimentsSetting[experimentName])
                cleanedUpExperimentSetting[experimentName] = true;
        }
        this._setting.set(cleanedUpExperimentSetting);
    }
}

/**
 * @constructor
 * @param {!WebInspector.ExperimentsSettings} experimentsSettings
 * @param {string} name
 * @param {string} title
 * @param {boolean} hidden
 */
WebInspector.Experiment = function(experimentsSettings, name, title, hidden)
{
    this._name = name;
    this._title = title;
    this._hidden = hidden;
    this._experimentsSettings = experimentsSettings;
}

WebInspector.Experiment.prototype = {
    /**
     * @return {string}
     */
    get name()
    {
        return this._name;
    },

    /**
     * @return {string}
     */
    get title()
    {
        return this._title;
    },

    /**
     * @return {boolean}
     */
    get hidden()
    {
        return this._hidden;
    },

    /**
     * @return {boolean}
     */
    isEnabled: function()
    {
        return this._experimentsSettings.isEnabled(this._name);
    },

    /**
     * @param {boolean} enabled
     */
    setEnabled: function(enabled)
    {
        this._experimentsSettings.setEnabled(this._name, enabled);
    },

    enableForTest: function()
    {
        this._experimentsSettings._enableForTest(this._name);
    }
}

/**
 * @constructor
 */
WebInspector.VersionController = function()
{
}

WebInspector.VersionController.currentVersion = 8;

WebInspector.VersionController.prototype = {
    updateVersion: function()
    {
        var versionSetting = WebInspector.settings.createSetting("inspectorVersion", 0);
        var currentVersion = WebInspector.VersionController.currentVersion;
        var oldVersion = versionSetting.get();
        var methodsToRun = this._methodsToRunToUpdateVersion(oldVersion, currentVersion);
        for (var i = 0; i < methodsToRun.length; ++i)
            this[methodsToRun[i]].call(this);
        versionSetting.set(currentVersion);
    },

    /**
     * @param {number} oldVersion
     * @param {number} currentVersion
     */
    _methodsToRunToUpdateVersion: function(oldVersion, currentVersion)
    {
        var result = [];
        for (var i = oldVersion; i < currentVersion; ++i)
            result.push("_updateVersionFrom" + i + "To" + (i + 1));
        return result;
    },

    _updateVersionFrom0To1: function()
    {
        this._clearBreakpointsWhenTooMany(WebInspector.settings.breakpoints, 500000);
    },

    _updateVersionFrom1To2: function()
    {
        var versionSetting = WebInspector.settings.createSetting("previouslyViewedFiles", []);
        versionSetting.set([]);
    },

    _updateVersionFrom2To3: function()
    {
        var fileSystemMappingSetting = WebInspector.settings.createSetting("fileSystemMapping", {});
        fileSystemMappingSetting.set({});
        if (window.localStorage)
            delete window.localStorage["fileMappingEntries"];
    },

    _updateVersionFrom3To4: function()
    {
        var advancedMode = WebInspector.settings.createSetting("showHeaSnapshotObjectsHiddenProperties", false).get();
        WebInspector.settings.showAdvancedHeapSnapshotProperties.set(advancedMode);
    },

    _updateVersionFrom4To5: function()
    {
        if (!window.localStorage)
            return;
        var settingNames = {
            "FileSystemViewSidebarWidth": "fileSystemViewSplitViewState",
            "canvasProfileViewReplaySplitLocation": "canvasProfileViewReplaySplitViewState",
            "canvasProfileViewSplitLocation": "canvasProfileViewSplitViewState",
            "elementsSidebarWidth": "elementsPanelSplitViewState",
            "StylesPaneSplitRatio": "stylesPaneSplitViewState",
            "heapSnapshotRetainersViewSize": "heapSnapshotSplitViewState",
            "InspectorView.splitView": "InspectorView.splitViewState",
            "InspectorView.screencastSplitView": "InspectorView.screencastSplitViewState",
            "Inspector.drawerSplitView": "Inspector.drawerSplitViewState",
            "layerDetailsSplitView": "layerDetailsSplitViewState",
            "networkSidebarWidth": "networkPanelSplitViewState",
            "sourcesSidebarWidth": "sourcesPanelSplitViewState",
            "scriptsPanelNavigatorSidebarWidth": "sourcesPanelNavigatorSplitViewState",
            "sourcesPanelSplitSidebarRatio": "sourcesPanelDebuggerSidebarSplitViewState",
            "timeline-details": "timelinePanelDetailsSplitViewState",
            "timeline-split": "timelinePanelRecorsSplitViewState",
            "timeline-view": "timelinePanelTimelineStackSplitViewState",
            "auditsSidebarWidth": "auditsPanelSplitViewState",
            "layersSidebarWidth": "layersPanelSplitViewState",
            "profilesSidebarWidth": "profilesPanelSplitViewState",
            "resourcesSidebarWidth": "resourcesPanelSplitViewState"
        };
        for (var oldName in settingNames) {
            var newName = settingNames[oldName];
            var oldNameH = oldName + "H";

            var newValue = null;
            var oldSetting = WebInspector.settings.createSetting(oldName, undefined).get();
            if (oldSetting) {
                newValue = newValue || {};
                newValue.vertical = {};
                newValue.vertical.size = oldSetting;
                delete window.localStorage[oldName];
            }
            var oldSettingH = WebInspector.settings.createSetting(oldNameH, undefined).get();
            if (oldSettingH) {
                newValue = newValue || {};
                newValue.horizontal = {};
                newValue.horizontal.size = oldSettingH;
                delete window.localStorage[oldNameH];
            }
            var newSetting = WebInspector.settings.createSetting(newName, {});
            if (newValue)
                newSetting.set(newValue);
        }
    },

    _updateVersionFrom5To6: function()
    {
        if (!window.localStorage)
            return;

        var settingNames = {
            "debuggerSidebarHidden": "sourcesPanelSplitViewState",
            "navigatorHidden": "sourcesPanelNavigatorSplitViewState",
            "WebInspector.Drawer.showOnLoad": "Inspector.drawerSplitViewState"
        };

        for (var oldName in settingNames) {
            var newName = settingNames[oldName];

            var oldSetting = WebInspector.settings.createSetting(oldName, undefined).get();
            var invert = "WebInspector.Drawer.showOnLoad" === oldName;
            var hidden = !!oldSetting !== invert;
            delete window.localStorage[oldName];
            var showMode = hidden ? "OnlyMain" : "Both";

            var newSetting = WebInspector.settings.createSetting(newName, null);
            var newValue = newSetting.get() || {};
            newValue.vertical = newValue.vertical || {};
            newValue.vertical.showMode = showMode;
            newValue.horizontal = newValue.horizontal || {};
            newValue.horizontal.showMode = showMode;
            newSetting.set(newValue);
        }
    },

    _updateVersionFrom6To7: function()
    {
        if (!window.localStorage)
            return;

        var settingNames = {
            "sourcesPanelNavigatorSplitViewState": "sourcesPanelNavigatorSplitViewState",
            "elementsPanelSplitViewState": "elementsPanelSplitViewState",
            "canvasProfileViewReplaySplitViewState": "canvasProfileViewReplaySplitViewState",
            "editorInDrawerSplitViewState": "editorInDrawerSplitViewState",
            "stylesPaneSplitViewState": "stylesPaneSplitViewState",
            "sourcesPanelDebuggerSidebarSplitViewState": "sourcesPanelDebuggerSidebarSplitViewState"
        };

        for (var name in settingNames) {
            if (!(name in window.localStorage))
                continue;
            var setting = WebInspector.settings.createSetting(name, undefined);
            var value = setting.get();
            if (!value)
                continue;
            // Zero out saved percentage sizes, and they will be restored to defaults.
            if (value.vertical && value.vertical.size && value.vertical.size < 1)
                value.vertical.size = 0;
            if (value.horizontal && value.horizontal.size && value.horizontal.size < 1)
                value.horizontal.size = 0;
            setting.set(value);
        }
    },

    _updateVersionFrom7To8: function()
    {
        var settingName = "deviceMetrics";
        if (!window.localStorage || !(settingName in window.localStorage))
            return;
        var setting = WebInspector.settings.createSetting(settingName, undefined);
        var value = setting.get();
        if (!value)
            return;

        var components = value.split("x");
        if (components.length >= 3) {
            var width = parseInt(components[0], 10);
            var height = parseInt(components[1], 10);
            var deviceScaleFactor = parseFloat(components[2]);
            if (deviceScaleFactor) {
                components[0] = "" + Math.round(width / deviceScaleFactor);
                components[1] = "" + Math.round(height / deviceScaleFactor);
            }
        }
        value = components.join("x");
        setting.set(value);
    },

    /**
     * @param {!WebInspector.Setting} breakpointsSetting
     * @param {number} maxBreakpointsCount
     */
    _clearBreakpointsWhenTooMany: function(breakpointsSetting, maxBreakpointsCount)
    {
        // If there are too many breakpoints in a storage, it is likely due to a recent bug that caused
        // periodical breakpoints duplication leading to inspector slowness.
        if (breakpointsSetting.get().length > maxBreakpointsCount)
            breakpointsSetting.set([]);
    }
}

WebInspector.settings = new WebInspector.Settings();
WebInspector.experimentsSettings = new WebInspector.ExperimentsSettings(WebInspector.queryParam("experiments") !== null);

// These methods are added for backwards compatibility with Devtools CodeSchool extension.
// DO NOT REMOVE

/**
 * @constructor
 */
WebInspector.PauseOnExceptionStateSetting = function()
{
    WebInspector.settings.pauseOnExceptionEnabled.addChangeListener(this._enabledChanged, this);
    WebInspector.settings.pauseOnCaughtException.addChangeListener(this._pauseOnCaughtChanged, this);
    this._name = "pauseOnExceptionStateString";
    this._eventSupport = new WebInspector.Object();
    this._value = this._calculateValue();
}

WebInspector.PauseOnExceptionStateSetting.prototype = {
    /**
     * @param {function(!WebInspector.Event)} listener
     * @param {!Object=} thisObject
     */
    addChangeListener: function(listener, thisObject)
    {
        this._eventSupport.addEventListener(this._name, listener, thisObject);
    },

    /**
     * @param {function(!WebInspector.Event)} listener
     * @param {!Object=} thisObject
     */
    removeChangeListener: function(listener, thisObject)
    {
        this._eventSupport.removeEventListener(this._name, listener, thisObject);
    },

    /**
     * @return {string}
     */
    get: function()
    {
        return this._value;
    },

    /**
     * @return {string}
     */
    _calculateValue: function()
    {
        if (!WebInspector.settings.pauseOnExceptionEnabled.get())
            return "none";
        // The correct code here would be
        //     return WebInspector.settings.pauseOnCaughtException.get() ? "all" : "uncaught";
        // But the CodeSchool DevTools relies on the fact that we used to enable pausing on ALL extensions by default, so we trick it here.
        return "all";
    },

    _enabledChanged: function(event)
    {
        this._fireChangedIfNeeded();
    },

    _pauseOnCaughtChanged: function(event)
    {
        this._fireChangedIfNeeded();
    },

    _fireChangedIfNeeded: function()
    {
        var newValue = this._calculateValue();
        if (newValue === this._value)
            return;
        this._value = newValue;
        this._eventSupport.dispatchEventToListeners(this._name, this._value);
    }
}

WebInspector.settings.pauseOnExceptionStateString = new WebInspector.PauseOnExceptionStateSetting();
