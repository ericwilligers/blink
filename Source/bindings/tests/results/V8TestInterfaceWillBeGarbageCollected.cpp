// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated by code_generator_v8.py. DO NOT MODIFY!

#include "config.h"
#include "V8TestInterfaceWillBeGarbageCollected.h"

#include "RuntimeEnabledFeatures.h"
#include "bindings/tests/v8/V8TestInterfaceWillBeGarbageCollected.h"
#include "bindings/v8/ExceptionState.h"
#include "bindings/v8/V8DOMConfiguration.h"
#include "bindings/v8/V8HiddenValue.h"
#include "bindings/v8/V8ObjectConstructor.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/Document.h"
#include "core/frame/DOMWindow.h"
#include "platform/TraceEvent.h"
#include "wtf/GetPtr.h"
#include "wtf/RefPtr.h"

namespace WebCore {

static void initializeScriptWrappableForInterface(TestInterfaceWillBeGarbageCollected* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::fromObject(object)->setTypeInfo(&V8TestInterfaceWillBeGarbageCollected::wrapperTypeInfo);
    else
        ASSERT_NOT_REACHED();
}

} // namespace WebCore

void webCoreInitializeScriptWrappableForInterface(WebCore::TestInterfaceWillBeGarbageCollected* object)
{
    WebCore::initializeScriptWrappableForInterface(object);
}

namespace WebCore {
const WrapperTypeInfo V8TestInterfaceWillBeGarbageCollected::wrapperTypeInfo = { gin::kEmbedderBlink, V8TestInterfaceWillBeGarbageCollected::domTemplate, V8TestInterfaceWillBeGarbageCollected::derefObject, 0, V8TestInterfaceWillBeGarbageCollected::toEventTarget, 0, V8TestInterfaceWillBeGarbageCollected::installPerContextEnabledMethods, &V8EventTarget::wrapperTypeInfo, WrapperTypeObjectPrototype, WillBeGarbageCollectedObject };

namespace TestInterfaceWillBeGarbageCollectedV8Internal {

template <typename T> void V8_USE(T) { }

static void attr1AttributeGetter(const v8::PropertyCallbackInfo<v8::Value>& info)
{
    v8::Handle<v8::Object> holder = info.Holder();
    TestInterfaceWillBeGarbageCollected* impl = V8TestInterfaceWillBeGarbageCollected::toNative(holder);
    v8SetReturnValueFast(info, WTF::getPtr(impl->attr1()), impl);
}

static void attr1AttributeGetterCallback(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestInterfaceWillBeGarbageCollectedV8Internal::attr1AttributeGetter(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void attr1AttributeSetter(v8::Local<v8::Value> v8Value, const v8::PropertyCallbackInfo<void>& info)
{
    v8::Handle<v8::Object> holder = info.Holder();
    TestInterfaceWillBeGarbageCollected* impl = V8TestInterfaceWillBeGarbageCollected::toNative(holder);
    TONATIVE_VOID(TestInterfaceWillBeGarbageCollected*, cppValue, V8TestInterfaceWillBeGarbageCollected::toNativeWithTypeCheck(info.GetIsolate(), v8Value));
    impl->setAttr1(WTF::getPtr(cppValue));
}

static void attr1AttributeSetterCallback(v8::Local<v8::String>, v8::Local<v8::Value> v8Value, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    TestInterfaceWillBeGarbageCollectedV8Internal::attr1AttributeSetter(v8Value, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void funcMethod(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    if (UNLIKELY(info.Length() < 1)) {
        throwMinimumArityTypeErrorForMethod("func", "TestInterfaceWillBeGarbageCollected", 1, info.Length(), info.GetIsolate());
        return;
    }
    TestInterfaceWillBeGarbageCollected* impl = V8TestInterfaceWillBeGarbageCollected::toNative(info.Holder());
    TestInterfaceWillBeGarbageCollected* arg;
    {
        v8::TryCatch block;
        TONATIVE_VOID_INTERNAL(arg, V8TestInterfaceWillBeGarbageCollected::toNativeWithTypeCheck(info.GetIsolate(), info[0]));
    }
    impl->func(arg);
}

static void funcMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestInterfaceWillBeGarbageCollectedV8Internal::funcMethod(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    v8::Isolate* isolate = info.GetIsolate();
    if (UNLIKELY(info.Length() < 1)) {
        throwMinimumArityTypeErrorForConstructor("TestInterfaceWillBeGarbageCollected", 1, info.Length(), info.GetIsolate());
        return;
    }
    V8StringResource<> str;
    {
        TOSTRING_VOID_INTERNAL_NOTRYCATCH(str, info[0]);
    }
    RefPtrWillBeRawPtr<TestInterfaceWillBeGarbageCollected> impl = TestInterfaceWillBeGarbageCollected::create(str);

    v8::Handle<v8::Object> wrapper = info.Holder();
    V8DOMWrapper::associateObjectWithWrapper<V8TestInterfaceWillBeGarbageCollected>(impl.release(), &V8TestInterfaceWillBeGarbageCollected::wrapperTypeInfo, wrapper, isolate, WrapperConfiguration::Independent);
    v8SetReturnValue(info, wrapper);
}

} // namespace TestInterfaceWillBeGarbageCollectedV8Internal

static const V8DOMConfiguration::AttributeConfiguration V8TestInterfaceWillBeGarbageCollectedAttributes[] = {
    {"attr1", TestInterfaceWillBeGarbageCollectedV8Internal::attr1AttributeGetterCallback, TestInterfaceWillBeGarbageCollectedV8Internal::attr1AttributeSetterCallback, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
};

static const V8DOMConfiguration::MethodConfiguration V8TestInterfaceWillBeGarbageCollectedMethods[] = {
    {"func", TestInterfaceWillBeGarbageCollectedV8Internal::funcMethodCallback, 0, 1},
};

const WrapperTypeInfo V8TestInterfaceWillBeGarbageCollectedConstructor::wrapperTypeInfo = { gin::kEmbedderBlink, V8TestInterfaceWillBeGarbageCollectedConstructor::domTemplate, V8TestInterfaceWillBeGarbageCollected::derefObject, 0, V8TestInterfaceWillBeGarbageCollected::toEventTarget, 0, V8TestInterfaceWillBeGarbageCollected::installPerContextEnabledMethods, 0, WrapperTypeObjectPrototype, WillBeGarbageCollectedObject };

static void V8TestInterfaceWillBeGarbageCollectedConstructorCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    v8::Isolate* isolate = info.GetIsolate();
    if (!info.IsConstructCall()) {
        throwTypeError(ExceptionMessages::constructorNotCallableAsFunction("TestInterface"), isolate);
        return;
    }

    if (ConstructorMode::current(isolate) == ConstructorMode::WrapExistingObject) {
        v8SetReturnValue(info, info.Holder());
        return;
    }

    Document* document = currentDOMWindow(isolate)->document();
    ASSERT(document);

    // Make sure the document is added to the DOM Node map. Otherwise, the TestInterfaceWillBeGarbageCollected instance
    // may end up being the only node in the map and get garbage-collected prematurely.
    toV8(document, info.Holder(), isolate);

    if (UNLIKELY(info.Length() < 1)) {
        throwMinimumArityTypeErrorForConstructor("TestInterfaceWillBeGarbageCollected", 1, info.Length(), info.GetIsolate());
        return;
    }
    V8StringResource<> str;
    {
        TOSTRING_VOID_INTERNAL_NOTRYCATCH(str, info[0]);
    }
    RefPtrWillBeRawPtr<TestInterfaceWillBeGarbageCollected> impl = TestInterfaceWillBeGarbageCollected::createForJSConstructor(*document, str);

    v8::Handle<v8::Object> wrapper = info.Holder();
    V8DOMWrapper::associateObjectWithWrapper<V8TestInterfaceWillBeGarbageCollected>(impl.release(), &V8TestInterfaceWillBeGarbageCollectedConstructor::wrapperTypeInfo, wrapper, isolate, WrapperConfiguration::Independent);
    v8SetReturnValue(info, wrapper);
}

v8::Handle<v8::FunctionTemplate> V8TestInterfaceWillBeGarbageCollectedConstructor::domTemplate(v8::Isolate* isolate)
{
    static int domTemplateKey; // This address is used for a key to look up the dom template.
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    v8::Local<v8::FunctionTemplate> result = data->existingDOMTemplate(&domTemplateKey);
    if (!result.IsEmpty())
        return result;

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    result = v8::FunctionTemplate::New(isolate, V8TestInterfaceWillBeGarbageCollectedConstructorCallback);
    v8::Local<v8::ObjectTemplate> instanceTemplate = result->InstanceTemplate();
    instanceTemplate->SetInternalFieldCount(V8TestInterfaceWillBeGarbageCollected::internalFieldCount);
    result->SetClassName(v8AtomicString(isolate, "TestInterfaceWillBeGarbageCollected"));
    result->Inherit(V8TestInterfaceWillBeGarbageCollected::domTemplate(isolate));
    data->setDOMTemplate(&domTemplateKey, result);
    return result;
}

void V8TestInterfaceWillBeGarbageCollected::constructorCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "DOMConstructor");
    if (!info.IsConstructCall()) {
        throwTypeError(ExceptionMessages::constructorNotCallableAsFunction("TestInterfaceWillBeGarbageCollected"), info.GetIsolate());
        return;
    }

    if (ConstructorMode::current(info.GetIsolate()) == ConstructorMode::WrapExistingObject) {
        v8SetReturnValue(info, info.Holder());
        return;
    }

    TestInterfaceWillBeGarbageCollectedV8Internal::constructor(info);
}

static void configureV8TestInterfaceWillBeGarbageCollectedTemplate(v8::Handle<v8::FunctionTemplate> functionTemplate, v8::Isolate* isolate)
{
    functionTemplate->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::installDOMClassTemplate(functionTemplate, "TestInterfaceWillBeGarbageCollected", V8EventTarget::domTemplate(isolate), V8TestInterfaceWillBeGarbageCollected::internalFieldCount,
        V8TestInterfaceWillBeGarbageCollectedAttributes, WTF_ARRAY_LENGTH(V8TestInterfaceWillBeGarbageCollectedAttributes),
        0, 0,
        V8TestInterfaceWillBeGarbageCollectedMethods, WTF_ARRAY_LENGTH(V8TestInterfaceWillBeGarbageCollectedMethods),
        isolate);
    functionTemplate->SetCallHandler(V8TestInterfaceWillBeGarbageCollected::constructorCallback);
    functionTemplate->SetLength(1);
    v8::Local<v8::ObjectTemplate> instanceTemplate ALLOW_UNUSED = functionTemplate->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> prototypeTemplate ALLOW_UNUSED = functionTemplate->PrototypeTemplate();

    // Custom toString template
    functionTemplate->Set(v8AtomicString(isolate, "toString"), V8PerIsolateData::from(isolate)->toStringTemplate());
}

v8::Handle<v8::FunctionTemplate> V8TestInterfaceWillBeGarbageCollected::domTemplate(v8::Isolate* isolate)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    v8::Local<v8::FunctionTemplate> result = data->existingDOMTemplate(const_cast<WrapperTypeInfo*>(&wrapperTypeInfo));
    if (!result.IsEmpty())
        return result;

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    result = v8::FunctionTemplate::New(isolate, V8ObjectConstructor::isValidConstructorMode);
    configureV8TestInterfaceWillBeGarbageCollectedTemplate(result, isolate);
    data->setDOMTemplate(const_cast<WrapperTypeInfo*>(&wrapperTypeInfo), result);
    return result;
}

bool V8TestInterfaceWillBeGarbageCollected::hasInstance(v8::Handle<v8::Value> v8Value, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, v8Value);
}

v8::Handle<v8::Object> V8TestInterfaceWillBeGarbageCollected::findInstanceInPrototypeChain(v8::Handle<v8::Value> v8Value, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->findInstanceInPrototypeChain(&wrapperTypeInfo, v8Value);
}

TestInterfaceWillBeGarbageCollected* V8TestInterfaceWillBeGarbageCollected::toNativeWithTypeCheck(v8::Isolate* isolate, v8::Handle<v8::Value> value)
{
    return hasInstance(value, isolate) ? fromInternalPointer(v8::Handle<v8::Object>::Cast(value)->GetAlignedPointerFromInternalField(v8DOMWrapperObjectIndex)) : 0;
}

EventTarget* V8TestInterfaceWillBeGarbageCollected::toEventTarget(v8::Handle<v8::Object> object)
{
    return toNative(object);
}

v8::Handle<v8::Object> V8TestInterfaceWillBeGarbageCollected::createWrapper(PassRefPtrWillBeRawPtr<TestInterfaceWillBeGarbageCollected> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl);
    ASSERT(!DOMDataStore::containsWrapper<V8TestInterfaceWillBeGarbageCollected>(impl.get(), isolate));
    if (ScriptWrappable::wrapperCanBeStoredInObject(impl.get())) {
        const WrapperTypeInfo* actualInfo = ScriptWrappable::fromObject(impl.get())->typeInfo();
        // Might be a XXXConstructor::wrapperTypeInfo instead of an XXX::wrapperTypeInfo. These will both have
        // the same object de-ref functions, though, so use that as the basis of the check.
        RELEASE_ASSERT_WITH_SECURITY_IMPLICATION(actualInfo->derefObjectFunction == wrapperTypeInfo.derefObjectFunction);
    }

    v8::Handle<v8::Object> wrapper = V8DOMWrapper::createWrapper(creationContext, &wrapperTypeInfo, toInternalPointer(impl.get()), isolate);
    if (UNLIKELY(wrapper.IsEmpty()))
        return wrapper;

    installPerContextEnabledProperties(wrapper, impl.get(), isolate);
    V8DOMWrapper::associateObjectWithWrapper<V8TestInterfaceWillBeGarbageCollected>(impl, &wrapperTypeInfo, wrapper, isolate, WrapperConfiguration::Independent);
    return wrapper;
}

void V8TestInterfaceWillBeGarbageCollected::derefObject(void* object)
{
#if !ENABLE(OILPAN)
    fromInternalPointer(object)->deref();
#endif // !ENABLE(OILPAN)
}

template<>
v8::Handle<v8::Value> toV8NoInline(TestInterfaceWillBeGarbageCollected* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    return toV8(impl, creationContext, isolate);
}

} // namespace WebCore
