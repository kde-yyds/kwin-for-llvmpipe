/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2022 Xaver Hugl <xaver.hugl@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "virtual_egl_gbm_layer.h"
#include "drm_abstract_output.h"
#include "drm_backend.h"
#include "drm_gpu.h"
#include "drm_output.h"
#include "drm_pipeline.h"
#include "drm_virtual_output.h"
#include "dumb_swapchain.h"
#include "egl_dmabuf.h"
#include "egl_gbm_backend.h"
#include "gbm_surface.h"
#include "kwineglutils_p.h"
#include "logging.h"
#include "shadowbuffer.h"
#include "surfaceitem_wayland.h"
#include "wayland/linuxdmabufv1clientbuffer.h"
#include "wayland/surface_interface.h"

#include <QRegion>
#include <drm_fourcc.h>
#include <errno.h>
#include <gbm.h>
#include <unistd.h>

namespace KWin
{

VirtualEglGbmLayer::VirtualEglGbmLayer(EglGbmBackend *eglBackend, DrmVirtualOutput *output)
    : m_output(output)
    , m_eglBackend(eglBackend)
{
}

void VirtualEglGbmLayer::aboutToStartPainting(const QRegion &damagedRegion)
{
    if (m_gbmSurface && m_gbmSurface->bufferAge() > 0 && !damagedRegion.isEmpty() && m_eglBackend->supportsPartialUpdate()) {
        const QRegion region = damagedRegion & m_output->geometry();

        QVector<EGLint> rects = m_output->regionToRects(region);
        const bool correct = eglSetDamageRegionKHR(m_eglBackend->eglDisplay(), m_gbmSurface->eglSurface(), rects.data(), rects.count() / 4);
        if (!correct) {
            qCWarning(KWIN_DRM) << "eglSetDamageRegionKHR failed:" << getEglErrorString();
        }
    }
}

OutputLayerBeginFrameInfo VirtualEglGbmLayer::beginFrame()
{
    // gbm surface
    if (doesGbmSurfaceFit(m_gbmSurface.get())) {
        m_oldGbmSurface.reset();
    } else {
        if (doesGbmSurfaceFit(m_oldGbmSurface.get())) {
            m_gbmSurface = m_oldGbmSurface;
        } else {
            if (!createGbmSurface()) {
                return {};
            }
        }
    }
    if (!m_gbmSurface->makeContextCurrent()) {
        return {};
    }
    GLFramebuffer::pushFramebuffer(m_gbmSurface->fbo());
    return OutputLayerBeginFrameInfo{
        .renderTarget = RenderTarget(m_gbmSurface->fbo()),
        .repaint = m_gbmSurface->repaintRegion(),
    };
}

bool VirtualEglGbmLayer::endFrame(const QRegion &renderedRegion, const QRegion &damagedRegion)
{
    Q_UNUSED(renderedRegion);
    GLFramebuffer::popFramebuffer();
    const auto buffer = m_gbmSurface->swapBuffers(damagedRegion);
    if (buffer) {
        m_currentBuffer = buffer;
        m_currentDamage = damagedRegion;
    }
    return buffer != nullptr;
}

QRegion VirtualEglGbmLayer::currentDamage() const
{
    return m_currentDamage;
}

bool VirtualEglGbmLayer::createGbmSurface()
{
    static bool modifiersEnvSet = false;
    static const bool modifiersEnv = qEnvironmentVariableIntValue("KWIN_DRM_USE_MODIFIERS", &modifiersEnvSet) != 0;
    const bool allowModifiers = !modifiersEnvSet || modifiersEnv;

    const auto tranches = m_eglBackend->dmabuf()->tranches();
    for (const auto &tranche : tranches) {
        for (auto it = tranche.formatTable.constBegin(); it != tranche.formatTable.constEnd(); it++) {
            const auto size = m_output->pixelSize();
            const auto config = m_eglBackend->config(it.key());
            const auto format = it.key();
            const auto modifiers = it.value();

            if (allowModifiers && !modifiers.isEmpty()) {
                const auto ret = GbmSurface::createSurface(m_eglBackend, size, format, modifiers, config);
                if (const auto surface = std::get_if<std::shared_ptr<GbmSurface>>(&ret)) {
                    m_oldGbmSurface = m_gbmSurface;
                    m_gbmSurface = *surface;
                    return true;
                } else if (std::get<GbmSurface::Error>(ret) != GbmSurface::Error::ModifiersUnsupported) {
                    continue;
                }
            }
            const auto ret = GbmSurface::createSurface(m_eglBackend, size, format, GBM_BO_USE_RENDERING, config);
            if (const auto surface = std::get_if<std::shared_ptr<GbmSurface>>(&ret)) {
                m_oldGbmSurface = m_gbmSurface;
                m_gbmSurface = *surface;
                return true;
            }
        }
    }
    return false;
}

bool VirtualEglGbmLayer::doesGbmSurfaceFit(GbmSurface *surf) const
{
    return surf && surf->size() == m_output->pixelSize();
}

QSharedPointer<GLTexture> VirtualEglGbmLayer::texture() const
{
    if (!m_currentBuffer) {
        qCWarning(KWIN_DRM) << "Failed to record frame: No gbm buffer!";
        return nullptr;
    }
    return m_eglBackend->importDmaBufAsTexture(m_currentBuffer->bo());
}

bool VirtualEglGbmLayer::scanout(SurfaceItem *surfaceItem)
{
    static bool valid;
    static const bool directScanoutDisabled = qEnvironmentVariableIntValue("KWIN_DRM_NO_DIRECT_SCANOUT", &valid) == 1 && valid;
    if (directScanoutDisabled) {
        return false;
    }

    SurfaceItemWayland *item = qobject_cast<SurfaceItemWayland *>(surfaceItem);
    if (!item || !item->surface()) {
        return false;
    }
    const auto buffer = qobject_cast<KWaylandServer::LinuxDmaBufV1ClientBuffer *>(item->surface()->buffer());
    if (!buffer || buffer->planes().isEmpty() || buffer->size() != m_output->pixelSize()) {
        return false;
    }
    const auto scanoutBuffer = GbmBuffer::importBuffer(m_output->gpu(), buffer);
    if (!scanoutBuffer) {
        return false;
    }
    // damage tracking for screen casting
    m_currentDamage = m_scanoutSurface == item->surface() ? surfaceItem->damage() : infiniteRegion();
    surfaceItem->resetDamage();
    m_scanoutSurface = item->surface();
    m_currentBuffer = scanoutBuffer;
    return true;
}

void VirtualEglGbmLayer::releaseBuffers()
{
    m_currentBuffer.reset();
    m_gbmSurface.reset();
    m_oldGbmSurface.reset();
}
}
