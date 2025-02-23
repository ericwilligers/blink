/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

var base = base || {};

(function(){

base.endsWith = function(string, suffix)
{
    if (suffix.length > string.length)
        return false;
    var expectedIndex = string.length - suffix.length;
    return string.lastIndexOf(suffix) == expectedIndex;
};

base.joinPath = function(parent, child)
{
    if (parent.length == 0)
        return child;
    return parent + '/' + child;
};

base.dirName = function(path)
{
    var directoryIndex = path.lastIndexOf('/');
    if (directoryIndex == -1)
        return path;
    return path.substr(0, directoryIndex);
};

base.trimExtension = function(url)
{
    var index = url.lastIndexOf('.');
    if (index == -1)
        return url;
    return url.substr(0, index);
}

base.uniquifyArray = function(array)
{
    var seen = {};
    var result = [];
    $.each(array, function(index, value) {
        if (seen[value])
            return;
        seen[value] = true;
        result.push(value);
    });
    return result;
};

base.flattenArray = function(arrayOfArrays)
{
    if (!arrayOfArrays.length)
        return [];
    return arrayOfArrays.reduce(function(left, right) {
        return left.concat(right);
    });
};

base.filterDictionary = function(dictionary, predicate)
{
    var result = {};

    for (var key in dictionary) {
        if (predicate(key))
            result[key] = dictionary[key];
    }

    return result;
};

base.mapDictionary = function(dictionary, functor)
{
    var result = {};

    for (var key in dictionary) {
        var value = functor(dictionary[key]);
        if (typeof value !== 'undefined')
            result[key] = value;
    }

    return result;
};

base.filterTree = function(tree, isLeaf, predicate)
{
    var filteredTree = {};

    function walkSubtree(subtree, directory)
    {
        for (var childName in subtree) {
            var child = subtree[childName];
            var childPath = base.joinPath(directory, childName);
            if (isLeaf(child)) {
                if (predicate(child))
                    filteredTree[childPath] = child;
                continue;
            }
            walkSubtree(child, childPath);
        }
    }

    walkSubtree(tree, '');
    return filteredTree;
};

base.forEachDirectory = function(pathList, callback)
{
    var pathsByDirectory = {};
    pathList.forEach(function(path) {
        var directory = base.dirName(path);
        pathsByDirectory[directory] = pathsByDirectory[directory] || [];
        pathsByDirectory[directory].push(path);
    });
    Object.keys(pathsByDirectory).sort().forEach(function(directory) {
        var paths = pathsByDirectory[directory];
        callback(directory + ' (' + paths.length + ' tests)', paths);
    });
};

base.parseJSONP = function(jsonp)
{
    if (!jsonp)
        return {};

    if (!jsonp.match(/^[^{[]*\(/))
        return JSON.parse(jsonp);

    var startIndex = jsonp.indexOf('(') + 1;
    var endIndex = jsonp.lastIndexOf(')');
    if (startIndex == 0 || endIndex == -1)
        return {};
    return JSON.parse(jsonp.substr(startIndex, endIndex - startIndex));
};

// This is effectively a cache of possibly-resolved promises.
base.AsynchronousCache = function(fetch)
{
    this._fetch = fetch;
    this._promiseCache = {};
};

base.AsynchronousCache._sentinel = new Object();
base.AsynchronousCache.prototype.get = function(key)
{
    if (!(key in this._promiseCache)) {
        this._promiseCache[key] = base.AsynchronousCache._sentinel;
        this._promiseCache[key] = this._fetch.call(null, key);
    }
    if (this._promiseCache[key] === base.AsynchronousCache._sentinel)
        return Promise.reject(Error("Reentrant request for ", key));

    return this._promiseCache[key];
};

base.AsynchronousCache.prototype.clear = function()
{
    this._promiseCache = {};
};

/*
    Maintains a dictionary of items, tracking their updates and removing items that haven't been updated.
    An "update" is a call to the "update" method.
    To remove stale items, call the "remove" method. It will remove all
    items that have not been been updated since the last call of "remove".
*/
base.UpdateTracker = function()
{
    this._items = {};
    this._updated = {};
}

base.UpdateTracker.prototype = {
    /*
        Update an {key}/{item} pair. You can make the dictionary act as a set and
        skip the {item}, in which case the {key} is also the {item}.
    */
    update: function(key, object)
    {
        object = object || key;
        this._items[key] = object;
        this._updated[key] = 1;
    },
    exists: function(key)
    {
        return !!this.get(key);
    },
    get: function(key)
    {
        return this._items[key];
    },
    length: function()
    {
        return Object.keys(this._items).length;
    },
    /*
        Callback parameters are:
        - item
        - key
        - updated, which is true if the item was updated after last purge() call.
    */
    forEach: function(callback, thisObject)
    {
        if (!callback)
            return;

        Object.keys(this._items).sort().forEach(function(key) {
            var item = this._items[key];
            callback.call(thisObject || item, item, key, !!this._updated[key]);
        }, this);
    },
    purge: function(removeCallback, thisObject) {
        removeCallback = removeCallback || function() {};
        this.forEach(function(item, key, updated) {
            if (updated)
                return;
            removeCallback.call(thisObject || item, item);
            delete this._items[key];
        }, this);
        this._updated = {};
    }
}

// Based on http://src.chromium.org/viewvc/chrome/trunk/src/chrome/browser/resources/shared/js/cr/ui.js
base.extends = function(base, prototype)
{
    var extended = function() {
        var element = typeof base == 'string' ? document.createElement(base) : base.call(this);
        extended.prototype.__proto__ = element.__proto__;
        element.__proto__ = extended.prototype;
        var singleton = element.init && element.init.apply(element, arguments);
        if (singleton)
            return singleton;
        return element;
    }

    extended.prototype = prototype;
    return extended;
}

function createRelativeTimeDescriptor(divisorInMilliseconds, unit)
{
    return function(delta) {
        var deltaInUnits = delta / divisorInMilliseconds;
        return (deltaInUnits).toFixed(0) + ' ' + unit + (deltaInUnits >= 1.5 ? 's' : '') + ' ago';
    }
}

var kMinuteInMilliseconds = 60 * 1000;
var kRelativeTimeSlots = [
    {
        maxMilliseconds: kMinuteInMilliseconds,
        describe: function(delta) { return 'Just now'; }
    },
    {
        maxMilliseconds: 60 * kMinuteInMilliseconds,
        describe: createRelativeTimeDescriptor(kMinuteInMilliseconds, 'minute')
    },
    {
        maxMilliseconds: 24 * 60 * kMinuteInMilliseconds,
        describe: createRelativeTimeDescriptor(60 * kMinuteInMilliseconds, 'hour')
    },
    {
        maxMilliseconds: Number.MAX_VALUE,
        describe: createRelativeTimeDescriptor(24 * 60 * kMinuteInMilliseconds, 'day')
    }
];

/*
    Represent time as descriptive text, relative to now and gradually decreasing in precision:
        delta < 1 minutes => Just Now
        delta < 60 minutes => X minute[s] ago
        delta < 24 hours => X hour[s] ago
        delta < inf => X day[s] ago
*/
base.relativizeTime = function(time)
{
    var result;
    var delta = new Date().getTime() - time;
    kRelativeTimeSlots.some(function(slot) {
        if (delta >= slot.maxMilliseconds)
            return false;

        result = slot.describe(delta);
        return true;
    });
    return result;
}

base.getURLParameter = function(name)
{
    var match = RegExp(name + '=' + '(.+?)(&|$)').exec(location.search);
    if (!match)
        return null;
    return decodeURI(match[1])
}

base.underscoredBuilderName = function(builderName)
{
    return builderName.replace(/[ .()]/g, '_');
}

})();
