/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#include "config.h"
#include "modules/indexeddb/IDBTransaction.h"

#include "core/dom/DOMError.h"
#include "core/dom/Document.h"
#include "modules/indexeddb/IDBDatabase.h"
#include "modules/indexeddb/IDBDatabaseCallbacks.h"
#include "modules/indexeddb/IDBPendingTransactionMonitor.h"
#include "platform/SharedBuffer.h"
#include "public/platform/WebIDBDatabase.h"

#include <gtest/gtest.h>

using namespace WebCore;

using blink::WebIDBDatabase;

namespace {

class IDBTransactionTest : public testing::Test {
public:
    IDBTransactionTest()
        : m_scope(V8TestingScope::create(v8::Isolate::GetCurrent()))
    {
        m_scope->scriptState()->setExecutionContext(Document::create());
    }

    v8::Isolate* isolate() const { return m_scope->isolate(); }
    ScriptState* scriptState() const { return m_scope->scriptState(); }
    ExecutionContext* executionContext() { return m_scope->scriptState()->executionContext(); }

private:
    OwnPtr<V8TestingScope> m_scope;
};

class FakeWebIDBDatabase FINAL : public blink::WebIDBDatabase {
public:
    static PassOwnPtr<FakeWebIDBDatabase> create() { return adoptPtr(new FakeWebIDBDatabase()); }

    virtual void commit(long long transactionId) OVERRIDE { }
    virtual void abort(long long transactionId) OVERRIDE { }
    virtual void close() OVERRIDE { }

private:
    FakeWebIDBDatabase() { }
};

class FakeIDBDatabaseCallbacks FINAL : public IDBDatabaseCallbacks {
public:
    static FakeIDBDatabaseCallbacks* create() { return new FakeIDBDatabaseCallbacks(); }
    virtual void onVersionChange(int64_t oldVersion, int64_t newVersion) OVERRIDE { }
    virtual void onForcedClose() OVERRIDE { }
    virtual void onAbort(int64_t transactionId, PassRefPtrWillBeRawPtr<DOMError> error) OVERRIDE { }
    virtual void onComplete(int64_t transactionId) OVERRIDE { }
private:
    FakeIDBDatabaseCallbacks() { }
};

TEST_F(IDBTransactionTest, EnsureLifetime)
{
    OwnPtr<FakeWebIDBDatabase> backend = FakeWebIDBDatabase::create();
    Persistent<IDBDatabase> db = IDBDatabase::create(executionContext(), backend.release(), FakeIDBDatabaseCallbacks::create());

    const int64_t transactionId = 1234;
    const Vector<String> transactionScope;
    Persistent<IDBTransaction> transaction = IDBTransaction::create(executionContext(), transactionId, transactionScope, blink::WebIDBDatabase::TransactionReadOnly, db.get());
    PersistentHeapHashSet<WeakMember<IDBTransaction> > set;
    set.add(transaction);

    Heap::collectAllGarbage();
    EXPECT_EQ(1u, set.size());

    Persistent<IDBRequest> request = IDBRequest::create(scriptState(), IDBAny::createUndefined(), transaction.get());
    IDBPendingTransactionMonitor::from(*executionContext()).deactivateNewTransactions();

    Heap::collectAllGarbage();
    EXPECT_EQ(1u, set.size());

    // This will generate an abort() call to the back end which is dropped by the fake proxy,
    // so an explicit onAbort call is made.
    executionContext()->stopActiveDOMObjects();
    transaction->onAbort(DOMError::create(AbortError, "Aborted"));
    transaction.clear();

    Heap::collectAllGarbage();
    EXPECT_EQ(0u, set.size());
}

TEST_F(IDBTransactionTest, TransactionFinish)
{
    OwnPtr<FakeWebIDBDatabase> backend = FakeWebIDBDatabase::create();
    Persistent<IDBDatabase> db = IDBDatabase::create(executionContext(), backend.release(), FakeIDBDatabaseCallbacks::create());

    const int64_t transactionId = 1234;
    const Vector<String> transactionScope;
    Persistent<IDBTransaction> transaction = IDBTransaction::create(executionContext(), transactionId, transactionScope, blink::WebIDBDatabase::TransactionReadOnly, db.get());
    PersistentHeapHashSet<WeakMember<IDBTransaction> > set;
    set.add(transaction);

    Heap::collectAllGarbage();
    EXPECT_EQ(1u, set.size());

    IDBPendingTransactionMonitor::from(*executionContext()).deactivateNewTransactions();

    Heap::collectAllGarbage();
    EXPECT_EQ(1u, set.size());

    transaction.clear();

    Heap::collectAllGarbage();
    EXPECT_EQ(1u, set.size());

    // Stop the context, so events don't get queued (which would keep the transaction alive).
    executionContext()->stopActiveDOMObjects();

    // Fire an abort to make sure this doesn't free the transaction during use. The test
    // will not fail if it is, but ASAN would notice the error.
    db->onAbort(transactionId, DOMError::create(AbortError, "Aborted"));

    // onAbort() should have cleared the transaction's reference to the database.
    Heap::collectAllGarbage();
    EXPECT_EQ(0u, set.size());
}

} // namespace
