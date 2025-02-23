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

#include "platform/graphics/gpu/DrawingBuffer.h"

#include "RuntimeEnabledFeatures.h"
#include "platform/graphics/ImageBuffer.h"
#include "platform/graphics/UnacceleratedImageBufferSurface.h"
#include "platform/graphics/gpu/Extensions3DUtil.h"
#include "platform/graphics/test/MockWebGraphicsContext3D.h"
#include "public/platform/Platform.h"
#include "public/platform/WebExternalTextureMailbox.h"
#include "wtf/RefPtr.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace WebCore;
using namespace blink;
using testing::Test;
using testing::_;

namespace {

class FakeContextEvictionManager : public ContextEvictionManager {
public:
    void forciblyLoseOldestContext(const String& reason) { }
    IntSize oldestContextSize() { return IntSize(); }
};

class WebGraphicsContext3DForTests : public MockWebGraphicsContext3D {
public:
    WebGraphicsContext3DForTests()
        : MockWebGraphicsContext3D()
        , m_boundTexture(0)
        , m_currentMailboxByte(0)
        , m_mostRecentlyWaitedSyncPoint(0)
        , m_currentImageId(1) { }

    virtual void bindTexture(WGC3Denum target, WebGLId texture)
    {
        if (target == GL_TEXTURE_2D) {
            m_boundTexture = texture;
        }
    }

    virtual void texImage2D(WGC3Denum target, WGC3Dint level, WGC3Denum internalformat, WGC3Dsizei width, WGC3Dsizei height, WGC3Dint border, WGC3Denum format, WGC3Denum type, const void* pixels)
    {
        if (target == GL_TEXTURE_2D && !level) {
            m_textureSizes.set(m_boundTexture, IntSize(width, height));
        }
    }

    virtual void genMailboxCHROMIUM(WGC3Dbyte* mailbox)
    {
        ++m_currentMailboxByte;
        WebExternalTextureMailbox temp;
        memset(mailbox, m_currentMailboxByte, sizeof(temp.name));
    }

    virtual void produceTextureCHROMIUM(WGC3Denum target, const WGC3Dbyte* mailbox)
    {
        ASSERT_EQ(target, static_cast<WGC3Denum>(GL_TEXTURE_2D));
        m_mostRecentlyProducedSize = m_textureSizes.get(m_boundTexture);
    }

    IntSize mostRecentlyProducedSize()
    {
        return m_mostRecentlyProducedSize;
    }

    virtual unsigned insertSyncPoint()
    {
        static unsigned syncPointGenerator = 0;
        return ++syncPointGenerator;
    }

    virtual void waitSyncPoint(unsigned syncPoint)
    {
        m_mostRecentlyWaitedSyncPoint = syncPoint;
    }

    virtual WGC3Duint createImageCHROMIUM(WGC3Dsizei width, WGC3Dsizei height, WGC3Denum internalformat, WGC3Denum usage)
    {
        m_imageSizes.set(m_currentImageId, IntSize(width, height));
        return m_currentImageId++;
    }

    MOCK_METHOD1(destroyImageMock, void(WGC3Duint imageId));
    void destroyImageCHROMIUM(WGC3Duint imageId)
    {
        m_imageSizes.remove(imageId);
        // No textures should be bound to this.
        ASSERT(m_imageToTextureMap.find(imageId) == m_imageToTextureMap.end());
        m_imageSizes.remove(imageId);
        destroyImageMock(imageId);
    }

    MOCK_METHOD1(bindTexImage2DMock, void(WGC3Dint imageId));
    void bindTexImage2DCHROMIUM(WGC3Denum target, WGC3Dint imageId)
    {
        if (target == GL_TEXTURE_2D) {
            m_textureSizes.set(m_boundTexture, m_imageSizes.find(imageId)->value);
            m_imageToTextureMap.set(imageId, m_boundTexture);
            bindTexImage2DMock(imageId);
        }
    }

    MOCK_METHOD1(releaseTexImage2DMock, void(WGC3Dint imageId));
    void releaseTexImage2DCHROMIUM(WGC3Denum target, WGC3Dint imageId)
    {
        if (target == GL_TEXTURE_2D) {
            m_imageSizes.set(m_currentImageId, IntSize());
            m_imageToTextureMap.remove(imageId);
            releaseTexImage2DMock(imageId);
        }
    }

    unsigned mostRecentlyWaitedSyncPoint()
    {
        return m_mostRecentlyWaitedSyncPoint;
    }

    WGC3Duint nextImageIdToBeCreated()
    {
        return m_currentImageId;
    }

private:
    WebGLId m_boundTexture;
    HashMap<WebGLId, IntSize> m_textureSizes;
    WGC3Dbyte m_currentMailboxByte;
    IntSize m_mostRecentlyProducedSize;
    unsigned m_mostRecentlyWaitedSyncPoint;
    WGC3Duint m_currentImageId;
    HashMap<WGC3Duint, IntSize> m_imageSizes;
    HashMap<WGC3Duint, WebGLId> m_imageToTextureMap;
};

static const int initialWidth = 100;
static const int initialHeight = 100;
static const int alternateHeight = 50;

class DrawingBufferForTests : public DrawingBuffer {
public:
    static PassRefPtr<DrawingBufferForTests> create(PassOwnPtr<blink::WebGraphicsContext3D> context,
        const IntSize& size, PreserveDrawingBuffer preserve, PassRefPtr<ContextEvictionManager> contextEvictionManager)
    {
        OwnPtr<Extensions3DUtil> extensionsUtil = Extensions3DUtil::create(context.get());
        RefPtr<DrawingBufferForTests> drawingBuffer =
            adoptRef(new DrawingBufferForTests(context, extensionsUtil.release(), preserve, contextEvictionManager));
        if (!drawingBuffer->initialize(size)) {
            drawingBuffer->beginDestruction();
            return PassRefPtr<DrawingBufferForTests>();
        }
        return drawingBuffer.release();
    }

    DrawingBufferForTests(PassOwnPtr<blink::WebGraphicsContext3D> context,
        PassOwnPtr<Extensions3DUtil> extensionsUtil,
        PreserveDrawingBuffer preserve,
        PassRefPtr<ContextEvictionManager> contextEvictionManager)
        : DrawingBuffer(context, extensionsUtil, false /* multisampleExtensionSupported */,
            false /* packedDepthStencilExtensionSupported */, preserve, contextEvictionManager)
        , m_live(0)
    { }

    virtual ~DrawingBufferForTests()
    {
        if (m_live)
            *m_live = false;
    }

    bool* m_live;
};

class DrawingBufferTest : public Test {
protected:
    virtual void SetUp()
    {
        RefPtr<FakeContextEvictionManager> contextEvictionManager = adoptRef(new FakeContextEvictionManager());
        OwnPtr<WebGraphicsContext3DForTests> context = adoptPtr(new WebGraphicsContext3DForTests);
        m_context = context.get();
        m_drawingBuffer = DrawingBufferForTests::create(context.release(),
            IntSize(initialWidth, initialHeight), DrawingBuffer::Preserve, contextEvictionManager.release());
    }

    WebGraphicsContext3DForTests* webContext()
    {
        return m_context;
    }

    WebGraphicsContext3DForTests* m_context;
    RefPtr<DrawingBufferForTests> m_drawingBuffer;
};

TEST_F(DrawingBufferTest, testPaintRenderingResultsToCanvas)
{
    OwnPtr<ImageBufferSurface> imageBufferSurface = adoptPtr(new UnacceleratedImageBufferSurface(IntSize(initialWidth, initialHeight)));
    EXPECT_FALSE(!imageBufferSurface);
    EXPECT_TRUE(imageBufferSurface->isValid());
    OwnPtr<ImageBuffer> imageBuffer = ImageBuffer::create(imageBufferSurface.release());
    EXPECT_FALSE(!imageBuffer);
    EXPECT_FALSE(imageBuffer->isAccelerated());
    EXPECT_FALSE(imageBuffer->bitmap().isNull());
    m_drawingBuffer->paintRenderingResultsToCanvas(imageBuffer.get());
    EXPECT_FALSE(imageBuffer->isAccelerated());
    EXPECT_FALSE(imageBuffer->bitmap().isNull());
    m_drawingBuffer->beginDestruction();
}

TEST_F(DrawingBufferTest, verifyResizingProperlyAffectsMailboxes)
{
    blink::WebExternalTextureMailbox mailbox;

    IntSize initialSize(initialWidth, initialHeight);
    IntSize alternateSize(initialWidth, alternateHeight);

    // Produce one mailbox at size 100x100.
    m_drawingBuffer->markContentsChanged();
    EXPECT_TRUE(m_drawingBuffer->prepareMailbox(&mailbox, 0));
    EXPECT_EQ(initialSize, webContext()->mostRecentlyProducedSize());

    // Resize to 100x50.
    m_drawingBuffer->reset(IntSize(initialWidth, alternateHeight));
    m_drawingBuffer->mailboxReleased(mailbox);

    // Produce a mailbox at this size.
    m_drawingBuffer->markContentsChanged();
    EXPECT_TRUE(m_drawingBuffer->prepareMailbox(&mailbox, 0));
    EXPECT_EQ(alternateSize, webContext()->mostRecentlyProducedSize());

    // Reset to initial size.
    m_drawingBuffer->reset(IntSize(initialWidth, initialHeight));
    m_drawingBuffer->mailboxReleased(mailbox);

    // Prepare another mailbox and verify that it's the correct size.
    m_drawingBuffer->markContentsChanged();
    EXPECT_TRUE(m_drawingBuffer->prepareMailbox(&mailbox, 0));
    EXPECT_EQ(initialSize, webContext()->mostRecentlyProducedSize());

    // Prepare one final mailbox and verify that it's the correct size.
    m_drawingBuffer->mailboxReleased(mailbox);
    m_drawingBuffer->markContentsChanged();
    EXPECT_TRUE(m_drawingBuffer->prepareMailbox(&mailbox, 0));
    EXPECT_EQ(initialSize, webContext()->mostRecentlyProducedSize());
    m_drawingBuffer->beginDestruction();
}

TEST_F(DrawingBufferTest, verifyDestructionCompleteAfterAllMailboxesReleased)
{
    bool live = true;
    m_drawingBuffer->m_live = &live;

    blink::WebExternalTextureMailbox mailbox1;
    blink::WebExternalTextureMailbox mailbox2;
    blink::WebExternalTextureMailbox mailbox3;

    IntSize initialSize(initialWidth, initialHeight);

    // Produce mailboxes.
    m_drawingBuffer->markContentsChanged();
    EXPECT_TRUE(m_drawingBuffer->prepareMailbox(&mailbox1, 0));
    m_drawingBuffer->markContentsChanged();
    EXPECT_TRUE(m_drawingBuffer->prepareMailbox(&mailbox2, 0));
    m_drawingBuffer->markContentsChanged();
    EXPECT_TRUE(m_drawingBuffer->prepareMailbox(&mailbox3, 0));

    m_drawingBuffer->markContentsChanged();
    m_drawingBuffer->mailboxReleased(mailbox1);

    m_drawingBuffer->beginDestruction();
    EXPECT_EQ(live, true);

    DrawingBufferForTests* weakPointer = m_drawingBuffer.get();
    m_drawingBuffer.clear();
    EXPECT_EQ(live, true);

    weakPointer->markContentsChanged();
    weakPointer->mailboxReleased(mailbox2);
    EXPECT_EQ(live, true);

    weakPointer->markContentsChanged();
    weakPointer->mailboxReleased(mailbox3);
    EXPECT_EQ(live, false);
}

class TextureMailboxWrapper {
public:
    explicit TextureMailboxWrapper(const blink::WebExternalTextureMailbox& mailbox)
        : m_mailbox(mailbox)
    { }

    bool operator==(const TextureMailboxWrapper& other) const
    {
        return !memcmp(m_mailbox.name, other.m_mailbox.name, sizeof(m_mailbox.name));
    }

private:
    blink::WebExternalTextureMailbox m_mailbox;
};

TEST_F(DrawingBufferTest, verifyRecyclingMailboxesByFIFO)
{
    blink::WebExternalTextureMailbox mailbox1;
    blink::WebExternalTextureMailbox mailbox2;
    blink::WebExternalTextureMailbox mailbox3;

    // Produce mailboxes.
    m_drawingBuffer->markContentsChanged();
    EXPECT_TRUE(m_drawingBuffer->prepareMailbox(&mailbox1, 0));
    m_drawingBuffer->markContentsChanged();
    EXPECT_TRUE(m_drawingBuffer->prepareMailbox(&mailbox2, 0));
    m_drawingBuffer->markContentsChanged();
    EXPECT_TRUE(m_drawingBuffer->prepareMailbox(&mailbox3, 0));

    // Release mailboxes by specific order; 2, 3, 1.
    m_drawingBuffer->markContentsChanged();
    m_drawingBuffer->mailboxReleased(mailbox2);
    m_drawingBuffer->markContentsChanged();
    m_drawingBuffer->mailboxReleased(mailbox3);
    m_drawingBuffer->markContentsChanged();
    m_drawingBuffer->mailboxReleased(mailbox1);

    // The first recycled mailbox must be 2.
    blink::WebExternalTextureMailbox recycledMailbox;
    m_drawingBuffer->markContentsChanged();
    EXPECT_TRUE(m_drawingBuffer->prepareMailbox(&recycledMailbox, 0));
    EXPECT_EQ(TextureMailboxWrapper(mailbox2), TextureMailboxWrapper(recycledMailbox));

    // The second recycled mailbox must be 3.
    m_drawingBuffer->markContentsChanged();
    EXPECT_TRUE(m_drawingBuffer->prepareMailbox(&recycledMailbox, 0));
    EXPECT_EQ(TextureMailboxWrapper(mailbox3), TextureMailboxWrapper(recycledMailbox));

    // The third recycled mailbox must be 1.
    m_drawingBuffer->markContentsChanged();
    EXPECT_TRUE(m_drawingBuffer->prepareMailbox(&recycledMailbox, 0));
    EXPECT_EQ(TextureMailboxWrapper(mailbox1), TextureMailboxWrapper(recycledMailbox));

    m_drawingBuffer->mailboxReleased(mailbox1);
    m_drawingBuffer->mailboxReleased(mailbox2);
    m_drawingBuffer->mailboxReleased(mailbox3);
    m_drawingBuffer->beginDestruction();
}

TEST_F(DrawingBufferTest, verifyInsertAndWaitSyncPointCorrectly)
{
    blink::WebExternalTextureMailbox mailbox;

    // Produce mailboxes.
    m_drawingBuffer->markContentsChanged();
    EXPECT_EQ(0u, webContext()->mostRecentlyWaitedSyncPoint());
    EXPECT_TRUE(m_drawingBuffer->prepareMailbox(&mailbox, 0));
    // prepareMailbox() does not wait for any sync point.
    EXPECT_EQ(0u, webContext()->mostRecentlyWaitedSyncPoint());

    unsigned waitSyncPoint = webContext()->insertSyncPoint();
    mailbox.syncPoint = waitSyncPoint;
    m_drawingBuffer->mailboxReleased(mailbox);
    // m_drawingBuffer will wait for the sync point when recycling.
    EXPECT_EQ(0u, webContext()->mostRecentlyWaitedSyncPoint());

    m_drawingBuffer->markContentsChanged();
    EXPECT_TRUE(m_drawingBuffer->prepareMailbox(&mailbox, 0));
    // m_drawingBuffer waits for the sync point when recycling in prepareMailbox().
    EXPECT_EQ(waitSyncPoint, webContext()->mostRecentlyWaitedSyncPoint());

    m_drawingBuffer->beginDestruction();
    waitSyncPoint = webContext()->insertSyncPoint();
    mailbox.syncPoint = waitSyncPoint;
    m_drawingBuffer->mailboxReleased(mailbox);
    // m_drawingBuffer waits for the sync point because the destruction is in progress.
    EXPECT_EQ(waitSyncPoint, webContext()->mostRecentlyWaitedSyncPoint());
}

class DrawingBufferImageChromiumTest : public DrawingBufferTest {
protected:
    virtual void SetUp()
    {
        RefPtr<FakeContextEvictionManager> contextEvictionManager = adoptRef(new FakeContextEvictionManager());
        OwnPtr<WebGraphicsContext3DForTests> context = adoptPtr(new WebGraphicsContext3DForTests);
        m_context = context.get();
        RuntimeEnabledFeatures::setWebGLImageChromiumEnabled(true);
        m_imageId0 = webContext()->nextImageIdToBeCreated();
        EXPECT_CALL(*webContext(), bindTexImage2DMock(m_imageId0)).Times(1);
        m_drawingBuffer = DrawingBufferForTests::create(context.release(),
            IntSize(initialWidth, initialHeight), DrawingBuffer::Preserve, contextEvictionManager.release());
        testing::Mock::VerifyAndClearExpectations(webContext());
    }

    virtual void TearDown()
    {
        RuntimeEnabledFeatures::setWebGLImageChromiumEnabled(false);
    }
    WGC3Duint m_imageId0;
};

TEST_F(DrawingBufferImageChromiumTest, verifyResizingReallocatesImages)
{
    blink::WebExternalTextureMailbox mailbox;

    IntSize initialSize(initialWidth, initialHeight);
    IntSize alternateSize(initialWidth, alternateHeight);

    WGC3Duint m_imageId1 = webContext()->nextImageIdToBeCreated();
    EXPECT_CALL(*webContext(), bindTexImage2DMock(m_imageId1)).Times(1);
    // Produce one mailbox at size 100x100.
    m_drawingBuffer->markContentsChanged();
    EXPECT_TRUE(m_drawingBuffer->prepareMailbox(&mailbox, 0));
    EXPECT_EQ(initialSize, webContext()->mostRecentlyProducedSize());
    EXPECT_TRUE(mailbox.allowOverlay);
    testing::Mock::VerifyAndClearExpectations(webContext());

    WGC3Duint m_imageId2 = webContext()->nextImageIdToBeCreated();
    EXPECT_CALL(*webContext(), bindTexImage2DMock(m_imageId2)).Times(1);
    EXPECT_CALL(*webContext(), destroyImageMock(m_imageId0)).Times(1);
    EXPECT_CALL(*webContext(), releaseTexImage2DMock(m_imageId0)).Times(1);
    // Resize to 100x50.
    m_drawingBuffer->reset(IntSize(initialWidth, alternateHeight));
    m_drawingBuffer->mailboxReleased(mailbox);
    testing::Mock::VerifyAndClearExpectations(webContext());

    WGC3Duint m_imageId3 = webContext()->nextImageIdToBeCreated();
    EXPECT_CALL(*webContext(), bindTexImage2DMock(m_imageId3)).Times(1);
    EXPECT_CALL(*webContext(), destroyImageMock(m_imageId1)).Times(1);
    EXPECT_CALL(*webContext(), releaseTexImage2DMock(m_imageId1)).Times(1);
    // Produce a mailbox at this size.
    m_drawingBuffer->markContentsChanged();
    EXPECT_TRUE(m_drawingBuffer->prepareMailbox(&mailbox, 0));
    EXPECT_EQ(alternateSize, webContext()->mostRecentlyProducedSize());
    EXPECT_TRUE(mailbox.allowOverlay);
    testing::Mock::VerifyAndClearExpectations(webContext());

    WGC3Duint m_imageId4 = webContext()->nextImageIdToBeCreated();
    EXPECT_CALL(*webContext(), bindTexImage2DMock(m_imageId4)).Times(1);
    EXPECT_CALL(*webContext(), destroyImageMock(m_imageId2)).Times(1);
    EXPECT_CALL(*webContext(), releaseTexImage2DMock(m_imageId2)).Times(1);
    // Reset to initial size.
    m_drawingBuffer->reset(IntSize(initialWidth, initialHeight));
    m_drawingBuffer->mailboxReleased(mailbox);
    testing::Mock::VerifyAndClearExpectations(webContext());

    WGC3Duint m_imageId5 = webContext()->nextImageIdToBeCreated();
    EXPECT_CALL(*webContext(), bindTexImage2DMock(m_imageId5)).Times(1);
    EXPECT_CALL(*webContext(), destroyImageMock(m_imageId3)).Times(1);
    EXPECT_CALL(*webContext(), releaseTexImage2DMock(m_imageId3)).Times(1);
    // Prepare another mailbox and verify that it's the correct size.
    m_drawingBuffer->markContentsChanged();
    EXPECT_TRUE(m_drawingBuffer->prepareMailbox(&mailbox, 0));
    EXPECT_EQ(initialSize, webContext()->mostRecentlyProducedSize());
    EXPECT_TRUE(mailbox.allowOverlay);
    testing::Mock::VerifyAndClearExpectations(webContext());

    // Prepare one final mailbox and verify that it's the correct size.
    m_drawingBuffer->mailboxReleased(mailbox);
    m_drawingBuffer->markContentsChanged();
    EXPECT_TRUE(m_drawingBuffer->prepareMailbox(&mailbox, 0));
    EXPECT_EQ(initialSize, webContext()->mostRecentlyProducedSize());
    EXPECT_TRUE(mailbox.allowOverlay);
    m_drawingBuffer->mailboxReleased(mailbox);

    EXPECT_CALL(*webContext(), destroyImageMock(m_imageId5)).Times(1);
    EXPECT_CALL(*webContext(), releaseTexImage2DMock(m_imageId5)).Times(1);
    EXPECT_CALL(*webContext(), destroyImageMock(m_imageId4)).Times(1);
    EXPECT_CALL(*webContext(), releaseTexImage2DMock(m_imageId4)).Times(1);
    m_drawingBuffer->beginDestruction();
    testing::Mock::VerifyAndClearExpectations(webContext());
}

} // namespace
