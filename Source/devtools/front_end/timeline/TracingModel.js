/*
 * Copyright 2014 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**
 * @constructor
 * @extends {WebInspector.TargetAwareObject}
 */
WebInspector.TracingModel = function(target)
{
    WebInspector.TargetAwareObject.call(this, target);
    this.reset();
    this._active = false;
    InspectorBackend.registerTracingDispatcher(new WebInspector.TracingDispatcher(this));
}

WebInspector.TracingModel.Events = {
    "BufferUsage": "BufferUsage"
}

/** @typedef {!{
        cat: string,
        pid: number,
        tid: number,
        ts: number,
        ph: string,
        name: string,
        args: !Object,
        dur: number,
        id: number,
        s: string
    }}
 */
WebInspector.TracingModel.EventPayload;

/**
 * @enum {string}
 */
WebInspector.TracingModel.Phase = {
    Begin: "B",
    End: "E",
    Complete: "X",
    Instant: "i",
    AsyncBegin: "S",
    AsyncStepInto: "T",
    AsyncStepPast: "p",
    AsyncEnd: "F",
    FlowBegin: "s",
    FlowStep: "t",
    FlowEnd: "f",
    Metadata: "M",
    Counter: "C",
    Sample: "P",
    CreateObject: "N",
    SnapshotObject: "O",
    DeleteObject: "D"
};

WebInspector.TracingModel.MetadataEvent = {
    ProcessSortIndex: "process_sort_index",
    ProcessName: "process_name",
    ThreadSortIndex: "thread_sort_index",
    ThreadName: "thread_name"
}

WebInspector.TracingModel.DevToolsMetadataEventCategory = "disabled-by-default-devtools.timeline";

WebInspector.TracingModel.FrameLifecycleEventCategory = "cc,devtools";

WebInspector.TracingModel.DevToolsMetadataEvent = {
    TracingStartedInPage: "TracingStartedInPage",
};

WebInspector.TracingModel.prototype = {
    /**
     * @return {!Array.<!WebInspector.TracingModel.Event>}
     */
    inspectedTargetEvents: function()
    {
        return this._inspectedTargetEvents;
    },

    /**
     * @param {string} categoryFilter
     * @param {string} options
     * @param {function(?string)=} callback
     */
    start: function(categoryFilter, options, callback)
    {
        this.reset();
        var bufferUsageReportingIntervalMs = 500;
        /**
         * @param {?string} error
         * @param {string} sessionId
         * @this {WebInspector.TracingModel}
         */
        function callbackWrapper(error, sessionId)
        {
            this._sessionId = sessionId;
            if (callback)
                callback(error);
        }
        TracingAgent.start(categoryFilter, options, bufferUsageReportingIntervalMs, callbackWrapper.bind(this));
        this._active = true;
    },

    /**
     * @param {function()} callback
     */
    stop: function(callback)
    {
        if (!this._active) {
            callback();
            return;
        }
        this._pendingStopCallback = callback;
        TracingAgent.end();
    },

    /**
     * @param {string} sessionId
     */
    setSessionIdForTest: function(sessionId)
    {
        this._sessionId = sessionId;
    },

    /**
     * @return {?string}
     */
    sessionId: function()
    {
        return this._sessionId;
    },

    /**
     * @param {number} usage
     */
    _bufferUsage: function(usage)
    {
        this.dispatchEventToListeners(WebInspector.TracingModel.Events.BufferUsage, usage);
    },

    /**
     * @param {!Array.<!WebInspector.TracingModel.EventPayload>} events
     */
    _eventsCollected: function(events)
    {
        for (var i = 0; i < events.length; ++i)
            this._addEvent(events[i]);
    },

    _tracingComplete: function()
    {
        this._active = false;

        /**
         * @param {!WebInspector.TracingModel.Event} a
         * @param {!WebInspector.TracingModel.Event} b
         */
        function compareStartTime(a, b)
        {
            return a.startTime - b.startTime;
        }

        this._inspectedTargetEvents.sort(compareStartTime);
        if (!this._pendingStopCallback)
            return;
        this._pendingStopCallback();
        this._pendingStopCallback = null;
    },

    reset: function()
    {
        this._processById = {};
        this._minimumRecordTime = null;
        this._maximumRecordTime = null;
        this._sessionId = null;
        this._inspectedTargetProcessId = null;
        this._inspectedTargetEvents = [];
    },

    /**
      * @param {!WebInspector.TracingModel.EventPayload} payload
      */
    _addEvent: function(payload)
    {
        var process = this._processById[payload.pid];
        if (!process) {
            process = new WebInspector.TracingModel.Process(payload.pid);
            this._processById[payload.pid] = process;
        }
        var thread = process.threadById(payload.tid);
        if (payload.ph === WebInspector.TracingModel.Phase.SnapshotObject) {
            var event = new WebInspector.TracingModel.Event(payload, 0, thread);
            process.addObject(event);
            if (payload.pid === this._inspectedTargetProcessId)
                this._inspectedTargetEvents.push(event);
            return;
        }
        if (payload.ph !== WebInspector.TracingModel.Phase.Metadata) {
            var timestamp = payload.ts;
            // We do allow records for unrelated threads to arrive out-of-order,
            // so there's a chance we're getting records from the past.
            if (timestamp && (!this._minimumRecordTime || timestamp < this._minimumRecordTime))
                this._minimumRecordTime = timestamp;
            if (!this._maximumRecordTime || timestamp > this._maximumRecordTime)
                this._maximumRecordTime = timestamp;
            if (payload.cat === WebInspector.TracingModel.DevToolsMetadataEventCategory)
                this._processDevToolsMetadataEvent(payload);
            var event = thread.addEvent(payload);
            if (event && payload.pid === this._inspectedTargetProcessId)
                this._inspectedTargetEvents.push(event);
            return;
        }
        switch (payload.name) {
        case WebInspector.TracingModel.MetadataEvent.ProcessSortIndex:
            process._setSortIndex(payload.args["sort_index"]);
            break;
        case WebInspector.TracingModel.MetadataEvent.ProcessName:
            process._setName(payload.args["name"]);
            break;
        case WebInspector.TracingModel.MetadataEvent.ThreadSortIndex:
            thread._setSortIndex(payload.args["sort_index"]);
            break;
        case WebInspector.TracingModel.MetadataEvent.ThreadName:
            thread._setName(payload.args["name"]);
            break;
        }
    },

     /**
      * @param {!WebInspector.TracingModel.EventPayload} payload
      */
    _processDevToolsMetadataEvent: function(payload)
    {
        if (payload.args["sessionId"] !== this._sessionId || payload.name !== WebInspector.TracingModel.DevToolsMetadataEvent.TracingStartedInPage)
            return;
        this._inspectedTargetProcessId = payload.pid;
    },

    /**
     * @return {?number}
     */
    minimumRecordTime: function()
    {
        return this._minimumRecordTime;
    },

    /**
     * @return {?number}
     */
    maximumRecordTime: function()
    {
        return this._maximumRecordTime;
    },

    /**
     * @return {!Array.<!WebInspector.TracingModel.Process>}
     */
    sortedProcesses: function()
    {
        return WebInspector.TracingModel.NamedObject._sort(Object.values(this._processById));
    },

    __proto__: WebInspector.TargetAwareObject.prototype
}

/**
 * @constructor
 * @param {!WebInspector.TracingModel.EventPayload} payload
 * @param {number} level
 * @param {?WebInspector.TracingModel.Thread} thread
 */
WebInspector.TracingModel.Event = function(payload, level, thread)
{
    this.name = payload.name;
    this.category = payload.cat;
    this.startTime = payload.ts;
    this.args = payload.args;
    this.phase = payload.ph;
    this.level = level;

    if (payload.id)
        this.id = payload.id;

    this.thread = thread;

    /** @type {?string} */
    this.warning = null;
    /** @type {?WebInspector.TracingModel.Event} */
    this.initiator = null;
    /** @type {?Array.<!ConsoleAgent.CallFrame>} */
    this.stackTrace = null;
    /** @type {?Element} */
    this.previewElement = null;
    /** @type {number} */
    this.selfTime = 0;
}

WebInspector.TracingModel.Event.prototype = {
    /**
     * @param {number} duration
     */
    _setDuration: function(duration)
    {
        this.endTime = this.startTime + duration;
        this.duration = duration;
    },

    /**
     * @param {!WebInspector.TracingModel.EventPayload} payload
     */
    _complete: function(payload)
    {
        if (this.name !== payload.name) {
            console.assert(false, "Open/close event mismatch: " + this.name + " vs. " + payload.name);
            return;
        }
        if (payload.args) {
            for (var name in payload.args) {
                if (name in this.args)
                    console.error("Same argument name (" + name +  ") is used for begin and end phases of " + this.name);
                this.args[name] = payload.args[name];
            }
        }
        var duration = payload.ts - this.startTime;
        if (duration < 0) {
            console.assert(false, "Event out of order: " + this.name);
            return;
        }
        this._setDuration(duration);
    }
}

/**
 * @constructor
 */
WebInspector.TracingModel.NamedObject = function()
{
}

WebInspector.TracingModel.NamedObject.prototype =
{
    /**
     * @param {string} name
     */
    _setName: function(name)
    {
        this._name = name;
    },

    /**
     * @return {string}
     */
    name: function()
    {
        return this._name;
    },

    /**
     * @param {number} sortIndex
     */
    _setSortIndex: function(sortIndex)
    {
        this._sortIndex = sortIndex;
    },
}

/**
 * @param {!Array.<!WebInspector.TracingModel.NamedObject>} array
 */
WebInspector.TracingModel.NamedObject._sort = function(array)
{
    /**
     * @param {!WebInspector.TracingModel.NamedObject} a
     * @param {!WebInspector.TracingModel.NamedObject} b
     */
    function comparator(a, b)
    {
        return a._sortIndex !== b._sortIndex ? a._sortIndex - b._sortIndex : a.name().localeCompare(b.name());
    }
    return array.sort(comparator);
}

/**
 * @constructor
 * @extends {WebInspector.TracingModel.NamedObject}
 * @param {number} id
 */
WebInspector.TracingModel.Process = function(id)
{
    WebInspector.TracingModel.NamedObject.call(this);
    this._setName("Process " + id);
    this._threads = {};
    this._objects = {};
}

WebInspector.TracingModel.Process.prototype = {
    /**
     * @param {number} id
     * @return {!WebInspector.TracingModel.Thread}
     */
    threadById: function(id)
    {
        var thread = this._threads[id];
        if (!thread) {
            thread = new WebInspector.TracingModel.Thread(id);
            this._threads[id] = thread;
        }
        return thread;
    },

    /**
     * @param {!WebInspector.TracingModel.Event} event
     */
    addObject: function(event)
    {
        this.objectsByName(event.name).push(event);
    },

    /**
     * @param {string} name
     * @return {!Array.<!WebInspector.TracingModel.Event>}
     */
    objectsByName: function(name)
    {
        var objects = this._objects[name];
        if (!objects) {
            objects = [];
            this._objects[name] = objects;
        }
        return objects;
    },

    /**
     * @return {!Array.<string>}
     */
    sortedObjectNames: function()
    {
        return Object.keys(this._objects).sort();
    },

    /**
     * @return {!Array.<!WebInspector.TracingModel.Thread>}
     */
    sortedThreads: function()
    {
        return WebInspector.TracingModel.NamedObject._sort(Object.values(this._threads));
    },

    __proto__: WebInspector.TracingModel.NamedObject.prototype
}

/**
 * @constructor
 * @extends {WebInspector.TracingModel.NamedObject}
 * @param {number} id
 */
WebInspector.TracingModel.Thread = function(id)
{
    WebInspector.TracingModel.NamedObject.call(this);
    this._setName("Thread " + id);
    this._events = [];
    this._stack = [];
    this._maxStackDepth = 0;
}

WebInspector.TracingModel.Thread.prototype = {
    /**
     * @param {!WebInspector.TracingModel.EventPayload} payload
     * @return {?WebInspector.TracingModel.Event} event
     */
    addEvent: function(payload)
    {
        for (var top = this._stack.peekLast(); top && top.endTime && top.endTime <= payload.ts;) {
            this._stack.pop();
            top = this._stack.peekLast();
        }
        if (payload.ph === WebInspector.TracingModel.Phase.End) {
            var openEvent = this._stack.pop();
            // Quietly ignore unbalanced close events, they're legit (we could have missed start one).
            if (openEvent)
                openEvent._complete(payload);
            return null;
        }

        var event = new WebInspector.TracingModel.Event(payload, this._stack.length, this);
        if (payload.ph === WebInspector.TracingModel.Phase.Begin || payload.ph === WebInspector.TracingModel.Phase.Complete) {
            if (payload.ph === WebInspector.TracingModel.Phase.Complete)
                event._setDuration(payload.dur);
            this._stack.push(event);
            if (this._maxStackDepth < this._stack.length)
                this._maxStackDepth = this._stack.length;
        }
        if (this._events.length && this._events.peekLast().startTime > event.startTime)
            console.assert(false, "Event is our of order: " + event.name);
        this._events.push(event);
        return event;
    },

    /**
     * @return {!Array.<!WebInspector.TracingModel.Event>}
     */
    events: function()
    {
        return this._events;
    },

    /**
     * @return {number}
     */
    maxStackDepth: function()
    {
        // Reserve one for non-container events.
        return this._maxStackDepth + 1;
    },

    __proto__: WebInspector.TracingModel.NamedObject.prototype
}


/**
 * @constructor
 * @implements {TracingAgent.Dispatcher}
 * @param {!WebInspector.TracingModel} tracingModel
 */
WebInspector.TracingDispatcher = function(tracingModel)
{
    this._tracingModel = tracingModel;
}

WebInspector.TracingDispatcher.prototype = {
    /**
     * @param {number} usage
     */
    bufferUsage: function(usage)
    {
        this._tracingModel._bufferUsage(usage);
    },

    /**
     * @param {!Array.<!WebInspector.TracingModel.EventPayload>} data
     */
    dataCollected: function(data)
    {
        this._tracingModel._eventsCollected(data);
    },

    tracingComplete: function()
    {
        this._tracingModel._tracingComplete();
    }
}
