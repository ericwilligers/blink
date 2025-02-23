// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated by {{code_generator}}. DO NOT MODIFY!

#ifndef {{v8_class}}_h
#define {{v8_class}}_h

{% filter conditional(conditional_string) %}
{% for filename in header_includes %}
#include "{{filename}}"
{% endfor %}

namespace WebCore {

{% if has_event_constructor %}
class Dictionary;
{% endif %}
{% if named_constructor %}
class {{v8_class}}Constructor {
public:
    static v8::Handle<v8::FunctionTemplate> domTemplate(v8::Isolate*);
    static const WrapperTypeInfo wrapperTypeInfo;
};

{% endif %}
class {{v8_class}} {
public:
    static bool hasInstance(v8::Handle<v8::Value>, v8::Isolate*);
    static v8::Handle<v8::Object> findInstanceInPrototypeChain(v8::Handle<v8::Value>, v8::Isolate*);
    static v8::Handle<v8::FunctionTemplate> domTemplate(v8::Isolate*);
    static {{cpp_class}}* toNative(v8::Handle<v8::Object> object)
    {
        return fromInternalPointer(object->GetAlignedPointerFromInternalField(v8DOMWrapperObjectIndex));
    }
    static {{cpp_class}}* toNativeWithTypeCheck(v8::Isolate*, v8::Handle<v8::Value>);
    static const WrapperTypeInfo wrapperTypeInfo;
    static void derefObject(void*);
    {% if has_visit_dom_wrapper %}
    static void visitDOMWrapper(void*, const v8::Persistent<v8::Object>&, v8::Isolate*);
    {% endif %}
    {% if is_active_dom_object %}
    static ActiveDOMObject* toActiveDOMObject(v8::Handle<v8::Object>);
    {% endif %}
    {% if is_event_target %}
    static EventTarget* toEventTarget(v8::Handle<v8::Object>);
    {% endif %}
    {% if interface_name == 'Window' %}
    static v8::Handle<v8::ObjectTemplate> getShadowObjectTemplate(v8::Isolate*);
    {% endif %}
    {% for method in methods if method.is_custom %}
    {% filter conditional(method.conditional_string) %}
    static void {{method.name}}MethodCustom(const v8::FunctionCallbackInfo<v8::Value>&);
    {% endfilter %}
    {% endfor %}
    {% if constructors or has_custom_constructor or has_event_constructor %}
    static void constructorCallback(const v8::FunctionCallbackInfo<v8::Value>&);
    {% endif %}
    {% if has_custom_constructor %}
    static void constructorCustom(const v8::FunctionCallbackInfo<v8::Value>&);
    {% endif %}
    {% for attribute in attributes %}
    {% if attribute.has_custom_getter %}{# FIXME: and not attribute.implemented_by #}
    {% filter conditional(attribute.conditional_string) %}
    static void {{attribute.name}}AttributeGetterCustom(const v8::PropertyCallbackInfo<v8::Value>&);
    {% endfilter %}
    {% endif %}
    {% if attribute.has_custom_setter %}{# FIXME: and not attribute.implemented_by #}
    {% filter conditional(attribute.conditional_string) %}
    static void {{attribute.name}}AttributeSetterCustom(v8::Local<v8::Value>, const v8::PropertyCallbackInfo<void>&);
    {% endfilter %}
    {% endif %}
    {% endfor %}
    {# Custom special operations #}
    {% if indexed_property_getter and indexed_property_getter.is_custom %}
    static void indexedPropertyGetterCustom(uint32_t, const v8::PropertyCallbackInfo<v8::Value>&);
    {% endif %}
    {% if indexed_property_setter and indexed_property_setter.is_custom %}
    static void indexedPropertySetterCustom(uint32_t, v8::Local<v8::Value>, const v8::PropertyCallbackInfo<v8::Value>&);
    {% endif %}
    {% if indexed_property_deleter and indexed_property_deleter.is_custom %}
    static void indexedPropertyDeleterCustom(uint32_t, const v8::PropertyCallbackInfo<v8::Boolean>&);
    {% endif %}
    {% if named_property_getter and named_property_getter.is_custom %}
    static void namedPropertyGetterCustom(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>&);
    {% endif %}
    {% if named_property_setter and named_property_setter.is_custom %}
    static void namedPropertySetterCustom(v8::Local<v8::String>, v8::Local<v8::Value>, const v8::PropertyCallbackInfo<v8::Value>&);
    {% endif %}
    {% if named_property_getter and
          named_property_getter.is_custom_property_query %}
    static void namedPropertyQueryCustom(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Integer>&);
    {% endif %}
    {% if named_property_deleter and named_property_deleter.is_custom %}
    static void namedPropertyDeleterCustom(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Boolean>&);
    {% endif %}
    {% if named_property_getter and
          named_property_getter.is_custom_property_enumerator %}
    static void namedPropertyEnumeratorCustom(const v8::PropertyCallbackInfo<v8::Array>&);
    {% endif %}
    {# END custom special operations #}
    {% if has_custom_legacy_call_as_function %}
    static void legacyCallCustom(const v8::FunctionCallbackInfo<v8::Value>&);
    {% endif %}
    {# Custom internal fields #}
    {% set custom_internal_field_counter = 0 %}
    {% if is_event_target and not is_node %}
    {# Event listeners on DOM nodes are explicitly supported in the GC controller. #}
    static const int eventListenerCacheIndex = v8DefaultWrapperInternalFieldCount + {{custom_internal_field_counter}};
    {% set custom_internal_field_counter = custom_internal_field_counter + 1 %}
    {% endif %}
    {# persistentHandleIndex must be the last field, if it is present.
       Detailed explanation: https://codereview.chromium.org/139173012
       FIXME: Remove this internal field, and share one field for either:
       * a persistent handle (if the object is in oilpan) or
       * a C++ pointer to the DOM object (if the object is not in oilpan) #}
    {% if not gc_type == 'RefCountedObject' %}
    static const int persistentHandleIndex = v8DefaultWrapperInternalFieldCount + {{custom_internal_field_counter}};
    {% set custom_internal_field_counter = custom_internal_field_counter + 1 %}
    {% endif %}
    static const int internalFieldCount = v8DefaultWrapperInternalFieldCount + {{custom_internal_field_counter}};
    {# End custom internal fields #}
    static inline void* toInternalPointer({{cpp_class}}* impl)
    {
        {% if parent_interface %}
        return V8{{parent_interface}}::toInternalPointer(impl);
        {% else %}
        return impl;
        {% endif %}
    }

    static inline {{cpp_class}}* fromInternalPointer(void* object)
    {
        {% if parent_interface %}
        return static_cast<{{cpp_class}}*>(V8{{parent_interface}}::fromInternalPointer(object));
        {% else %}
        return static_cast<{{cpp_class}}*>(object);
        {% endif %}
    }
    {% if interface_name == 'Window' %}
    static bool namedSecurityCheckCustom(v8::Local<v8::Object> host, v8::Local<v8::Value> key, v8::AccessType, v8::Local<v8::Value> data);
    static bool indexedSecurityCheckCustom(v8::Local<v8::Object> host, uint32_t index, v8::AccessType, v8::Local<v8::Value> data);
    {% endif %}
    static void installPerContextEnabledProperties(v8::Handle<v8::Object>, {{cpp_class}}*, v8::Isolate*){% if has_per_context_enabled_attributes %};
    {% else %} { }
    {% endif %}
    static void installPerContextEnabledMethods(v8::Handle<v8::Object>, v8::Isolate*){% if has_per_context_enabled_methods %};
    {% else %} { }
    {% endif %}
    {# Element wrappers #}
    {% if interface_name == 'HTMLElement' %}
    friend v8::Handle<v8::Object> createV8HTMLWrapper(HTMLElement*, v8::Handle<v8::Object> creationContext, v8::Isolate*);
    friend v8::Handle<v8::Object> createV8HTMLDirectWrapper(HTMLElement*, v8::Handle<v8::Object> creationContext, v8::Isolate*);
    {% elif interface_name == 'SVGElement' %}
    friend v8::Handle<v8::Object> createV8SVGWrapper(SVGElement*, v8::Handle<v8::Object> creationContext, v8::Isolate*);
    friend v8::Handle<v8::Object> createV8SVGDirectWrapper(SVGElement*, v8::Handle<v8::Object> creationContext, v8::Isolate*);
    friend v8::Handle<v8::Object> createV8SVGFallbackWrapper(SVGElement*, v8::Handle<v8::Object> creationContext, v8::Isolate*);
    {% elif interface_name == 'HTMLUnknownElement' %}
    friend v8::Handle<v8::Object> createV8HTMLFallbackWrapper(HTMLUnknownElement*, v8::Handle<v8::Object> creationContext, v8::Isolate*);
    {% elif interface_name == 'Element' %}
    // This is a performance optimization hack. See V8Element::wrap.
    friend v8::Handle<v8::Object> wrap(Node*, v8::Handle<v8::Object> creationContext, v8::Isolate*);
    {% endif %}

private:
    {% if not has_custom_to_v8 %}
    friend v8::Handle<v8::Object> wrap({{cpp_class}}*, v8::Handle<v8::Object> creationContext, v8::Isolate*);
    static v8::Handle<v8::Object> createWrapper({{pass_cpp_type}}, v8::Handle<v8::Object> creationContext, v8::Isolate*);
    {% endif %}
};

{% if has_custom_to_v8 %}
class {{cpp_class}};
v8::Handle<v8::Value> toV8({{cpp_class}}*, v8::Handle<v8::Object> creationContext, v8::Isolate*);

template<class CallbackInfo>
inline void v8SetReturnValue(const CallbackInfo& callbackInfo, {{cpp_class}}* impl)
{
    v8SetReturnValue(callbackInfo, toV8(impl, callbackInfo.Holder(), callbackInfo.GetIsolate()));
}

template<class CallbackInfo>
inline void v8SetReturnValueForMainWorld(const CallbackInfo& callbackInfo, {{cpp_class}}* impl)
{
     v8SetReturnValue(callbackInfo, toV8(impl, callbackInfo.Holder(), callbackInfo.GetIsolate()));
}

template<class CallbackInfo, class Wrappable>
inline void v8SetReturnValueFast(const CallbackInfo& callbackInfo, {{cpp_class}}* impl, Wrappable*)
{
     v8SetReturnValue(callbackInfo, toV8(impl, callbackInfo.Holder(), callbackInfo.GetIsolate()));
}
{% else %}{# has_custom_to_v8 #}
{% if has_custom_wrap or special_wrap_for or is_document %}
v8::Handle<v8::Object> wrap({{cpp_class}}* impl, v8::Handle<v8::Object> creationContext, v8::Isolate*);
{% else %}
inline v8::Handle<v8::Object> wrap({{cpp_class}}* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl);
    ASSERT(!DOMDataStore::containsWrapper<{{v8_class}}>(impl, isolate));
    return {{v8_class}}::createWrapper(impl, creationContext, isolate);
}
{% endif %}

inline v8::Handle<v8::Value> toV8({{cpp_class}}* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    if (UNLIKELY(!impl))
        return v8::Null(isolate);
    v8::Handle<v8::Value> wrapper = DOMDataStore::getWrapper<{{v8_class}}>(impl, isolate);
    if (!wrapper.IsEmpty())
        return wrapper;
    return wrap(impl, creationContext, isolate);
}

template<typename CallbackInfo>
inline void v8SetReturnValue(const CallbackInfo& callbackInfo, {{cpp_class}}* impl)
{
    if (UNLIKELY(!impl)) {
        v8SetReturnValueNull(callbackInfo);
        return;
    }
    if (DOMDataStore::setReturnValueFromWrapper<{{v8_class}}>(callbackInfo.GetReturnValue(), impl))
        return;
    v8::Handle<v8::Object> wrapper = wrap(impl, callbackInfo.Holder(), callbackInfo.GetIsolate());
    v8SetReturnValue(callbackInfo, wrapper);
}

template<typename CallbackInfo>
inline void v8SetReturnValueForMainWorld(const CallbackInfo& callbackInfo, {{cpp_class}}* impl)
{
    ASSERT(DOMWrapperWorld::current(callbackInfo.GetIsolate()).isMainWorld());
    if (UNLIKELY(!impl)) {
        v8SetReturnValueNull(callbackInfo);
        return;
    }
    if (DOMDataStore::setReturnValueFromWrapperForMainWorld<{{v8_class}}>(callbackInfo.GetReturnValue(), impl))
        return;
    v8::Handle<v8::Value> wrapper = wrap(impl, callbackInfo.Holder(), callbackInfo.GetIsolate());
    v8SetReturnValue(callbackInfo, wrapper);
}

template<class CallbackInfo, class Wrappable>
inline void v8SetReturnValueFast(const CallbackInfo& callbackInfo, {{cpp_class}}* impl, Wrappable* wrappable)
{
    if (UNLIKELY(!impl)) {
        v8SetReturnValueNull(callbackInfo);
        return;
    }
    if (DOMDataStore::setReturnValueFromWrapperFast<{{v8_class}}>(callbackInfo.GetReturnValue(), impl, callbackInfo.Holder(), wrappable))
        return;
    v8::Handle<v8::Object> wrapper = wrap(impl, callbackInfo.Holder(), callbackInfo.GetIsolate());
    v8SetReturnValue(callbackInfo, wrapper);
}
{% endif %}{# has_custom_to_v8 #}

inline v8::Handle<v8::Value> toV8({{pass_cpp_type}} impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    return toV8(impl.get(), creationContext, isolate);
}

template<class CallbackInfo>
inline void v8SetReturnValue(const CallbackInfo& callbackInfo, {{pass_cpp_type}} impl)
{
    v8SetReturnValue(callbackInfo, impl.get());
}

template<class CallbackInfo>
inline void v8SetReturnValueForMainWorld(const CallbackInfo& callbackInfo, {{pass_cpp_type}} impl)
{
    v8SetReturnValueForMainWorld(callbackInfo, impl.get());
}

template<class CallbackInfo, class Wrappable>
inline void v8SetReturnValueFast(const CallbackInfo& callbackInfo, {{pass_cpp_type}} impl, Wrappable* wrappable)
{
    v8SetReturnValueFast(callbackInfo, impl.get(), wrappable);
}

{% if has_event_constructor %}
bool initialize{{cpp_class}}({{cpp_class}}Init&, const Dictionary&, ExceptionState&, const v8::FunctionCallbackInfo<v8::Value>& info, const String& = "");

{% endif %}
}
{% endfilter %}
#endif // {{v8_class}}_h
