/*
    Copyright 2026 Benjamin Vedder	benjamin@vedder.se

    This file is part of VESC Tool.

    VESC Tool is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VESC Tool is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

// Precision touchpad gestures through DirectManipulation, handed to Qt as the macOS plugin does. Before Qt 6.13
// they only arrive as plain wheel events. Follows qwindowsdirectmanipulation.cpp from qtbase dev (503e9cf2d2e4),
// Copyright (C) 2026 The Qt Company Ltd., used here under GPL-3.0-only.

#include "wintouchpad.h"

#include <QtGlobal>

#if QT_VERSION < QT_VERSION_CHECK(6, 13, 0)

#include <QAbstractNativeEventFilter>
#include <QApplication>
#include <QBasicTimer>
#include <QElapsedTimer>
#include <QHash>
#include <QLibraryInfo>
#include <QPlatformSurfaceEvent>
#include <QPointer>
#include <QPointingDevice>
#include <QQuickWidget>
#include <QQuickWindow>
#include <QVersionNumber>
#include <qpa/qwindowsysteminterface.h>

#include <atomic>
#include <cstdint>
#include <utility>

#include <windows.h>
#include <objbase.h>
#include <directmanipulation.h>

namespace {

// DirectManipulation only reports while pumped. Stops after 25 idle ticks.
const int pumpMs = 8;
const int maxIdleTicks = 25;

QPointingDevice *device = nullptr;
// Event times. GetTickCount is too coarse, and Flickable drops events with repeated times.
QElapsedTimer eventTime;

void startPump();

// One per window. Changes of the transform of an undrawn viewport become wheel and pinch events.
class Viewport final : public IDirectManipulationViewportEventHandler, public IDirectManipulationInteractionEventHandler
{
public:
    Viewport(QWindow *w, HWND h) : window(w), hwnd(h) {}

    QPointer<QWindow> window;
    const HWND hwnd;
    IDirectManipulationViewport *viewport = nullptr;
    DWORD cookie = 0;

    bool interacting() const
    {
        return status == DIRECTMANIPULATION_RUNNING || status == DIRECTMANIPULATION_INERTIA;
    }

    // Also called from other threads
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        const ULONG r = --refs;
        if (r == 0) {
            delete this;
        }
        return r;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **out) override
    {
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IDirectManipulationViewportEventHandler)) {
            *out = static_cast<IDirectManipulationViewportEventHandler*>(this);
            AddRef();
            return S_OK;
        }
        if (riid == __uuidof(IDirectManipulationInteractionEventHandler)) {
            *out = static_cast<IDirectManipulationInteractionEventHandler*>(this);
            AddRef();
            return S_OK;
        }
        *out = nullptr;
        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE OnViewportStatusChanged(IDirectManipulationViewport *vp,
                                                      DIRECTMANIPULATION_STATUS current,
                                                      DIRECTMANIPULATION_STATUS previous) override
    {
        // Caused by the reset below
        if (resetting) {
            return S_OK;
        }

        status = current;
        if (current == DIRECTMANIPULATION_RUNNING) {
            startPump();
        }
        if (current == DIRECTMANIPULATION_RUNNING && previous == DIRECTMANIPULATION_INERTIA) {
            // Fingers down while coasting start a new gesture
            end();
        } else if (current == DIRECTMANIPULATION_INERTIA && gesture == Pinch) {
            // Lifting ends a pinch, its coasting moves nothing
            end();
            gesture = Done;
        } else if (current == DIRECTMANIPULATION_READY) {
            end();
            // Back to the identity
            IDirectManipulationContent *content = nullptr;
            if (haveRef && (refScale != 1.0f || !refPos.isNull()) &&
                    SUCCEEDED(vp->GetPrimaryContent(IID_PPV_ARGS(&content)))) {
                static const float identity[6] = {1, 0, 0, 1, 0, 0};
                resetting = true;
                content->SyncContentTransform(identity, 6);
                resetting = false;
                content->Release();
            }
            haveRef = false;
        } else if (current != DIRECTMANIPULATION_RUNNING && current != DIRECTMANIPULATION_INERTIA) {
            end();
            haveRef = false;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnViewportUpdated(IDirectManipulationViewport *) override { return S_OK; }

    // Restarts the pump for fingers that rested before moving
    HRESULT STDMETHODCALLTYPE OnInteraction(IDirectManipulationViewport2 *,
                                            DIRECTMANIPULATION_INTERACTION_TYPE type) override
    {
        if (type == DIRECTMANIPULATION_INTERACTION_BEGIN) {
            startPump();
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnContentUpdated(IDirectManipulationViewport *,
                                               IDirectManipulationContent *content) override
    {
        // Only a uniform scale and a translation, in native pixels
        float m[6];
        if (resetting || !window || FAILED(content->GetContentTransform(m, 6))) {
            return S_OK;
        }
        const float scale = m[0];
        const QPointF pos(m[4], m[5]);

        // The first transform after a reset is only the reference
        if (!haveRef) {
            haveRef = true;
            refScale = scale;
            refPos = pos;
            return S_OK;
        }

        const qreal ratio = scale / refScale;
        const QPointF moved = pos - refPos;
        refScale = scale;
        refPos = pos;
        if (gesture == Done) {
            return S_OK;
        }

        // A pinch also translates, so the scale decides. It stays a pinch until the fingers lift.
        if (qAbs(ratio - 1.0) > 1e-5) {
            if (gesture == Scroll) {
                end();
            }
            if (gesture == None) {
                gesture = Pinch;
                pinch(Qt::BeginNativeGesture, 0);
            }
            // Relative, as PinchHandler applies it
            pinch(Qt::ZoomNativeGesture, ratio - 1.0);
        } else if (gesture != Pinch && !moved.isNull()) {
            // In device independent pixels, keeping the fraction
            carry += moved / window->devicePixelRatio();
            const QPoint pixels = carry.toPoint();
            if (pixels.isNull()) {
                return S_OK;
            }
            carry -= pixels;

            Qt::ScrollPhase phase = Qt::ScrollUpdate;
            if (gesture == None) {
                gesture = Scroll;
                // Kept from the start, as on macOS
                mods = QGuiApplication::queryKeyboardModifiers();
                // An empty begin, then the motion as updates, as from macOS
                scroll(Qt::ScrollBegin, QPoint());
            } else if (status == DIRECTMANIPULATION_INERTIA) {
                phase = Qt::ScrollMomentum;
            }
            scroll(phase, pixels);
        }
        return S_OK;
    }

    void end()
    {
        if (window && gesture == Scroll) {
            scroll(Qt::ScrollEnd, QPoint());
        } else if (window && gesture == Pinch) {
            pinch(Qt::EndNativeGesture, 0);
        }
        gesture = None;
        carry = QPointF();
    }

private:
    enum Gesture { None, Scroll, Pinch, Done };

    std::atomic<ULONG> refs {1};
    DIRECTMANIPULATION_STATUS status = DIRECTMANIPULATION_BUILDING;
    Gesture gesture = None;
    bool resetting = false;
    bool haveRef = false;
    float refScale = 1;
    QPointF refPos;
    QPointF carry;
    Qt::KeyboardModifiers mods;

    // Native pixels
    void cursor(QPointF *local, QPointF *global) const
    {
        POINT p;
        GetCursorPos(&p);
        *global = QPointF(p.x, p.y);
        ScreenToClient(hwnd, &p);
        *local = QPointF(p.x, p.y);
    }

    // angleDelta as from macOS, Qt drops updates without one
    void scroll(Qt::ScrollPhase phase, QPoint pixels)
    {
        QPointF local, global;
        cursor(&local, &global);
        QWindowSystemInterface::handleWheelEvent(window.data(), ulong(eventTime.elapsed()), device, local, global,
                                                 pixels, pixels * 2, mods, phase, Qt::MouseEventSynthesizedBySystem);
    }

    void pinch(Qt::NativeGestureType type, qreal value)
    {
        QPointF local, global;
        cursor(&local, &global);
        QWindowSystemInterface::handleGestureEventWithRealValue(window.data(), ulong(eventTime.elapsed()), device,
                                                                type, value, local, global, 2);
    }
};

class Touchpad : public QObject, public QAbstractNativeEventFilter
{
public:
    bool init()
    {
        if (FAILED(CoCreateInstance(__uuidof(DirectManipulationManager), nullptr, CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(&manager)))) {
            return false;
        }
        if (FAILED(manager->GetUpdateManager(IID_PPV_ARGS(&updater)))) {
            manager->Release();
            return false;
        }

        // First, or the touchpad would become the primary device
        QPointingDevice::primaryPointingDevice();
        device = new QPointingDevice(QStringLiteral("touchpad"), INT64_MAX, QInputDevice::DeviceType::TouchPad,
                                     QPointingDevice::PointerType::Finger,
                                     QInputDevice::Capability::Position | QInputDevice::Capability::Scroll |
                                     QInputDevice::Capability::PixelScroll, 2, 3);
        QWindowSystemInterface::registerInputDevice(device);
        eventTime.start();
        return true;
    }

    void restartPump()
    {
        idleTicks = 0;
        if (!pump.isActive()) {
            pump.start(pumpMs, Qt::PreciseTimer, this);
        }
    }

protected:
    bool eventFilter(QObject *obj, QEvent *e) override
    {
        if (e->type() != QEvent::PlatformSurface || !obj->isWindowType()) {
            return false;
        }

        QWindow *w = static_cast<QWindow*>(obj);
        if (static_cast<QPlatformSurfaceEvent*>(e)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated) {
            QPointer<QWindow> p(w);
            QMetaObject::invokeMethod(this, [this, p]() { addWindow(p); }, Qt::QueuedConnection);
        } else {
            for (auto it = viewports.begin(); it != viewports.end(); ++it) {
                if (it.value()->window == w) {
                    destroy(it.value());
                    viewports.erase(it);
                    break;
                }
            }
        }
        return false;
    }

    bool nativeEventFilter(const QByteArray &type, void *message, qintptr *) override
    {
        const MSG *msg = static_cast<MSG*>(message);
        if (type != "windows_generic_MSG" || msg->message != DM_POINTERHITTEST) {
            return false;
        }

        // Only touchpad contacts. The pump starts here, as transforms come before RUNNING.
        Viewport *v = viewports.value(msg->hwnd);
        const UINT32 id = GET_POINTERID_WPARAM(msg->wParam);
        POINTER_INPUT_TYPE pointerType;
        if (v && GetPointerType(id, &pointerType) && pointerType == PT_TOUCHPAD &&
                SUCCEEDED(v->viewport->SetContact(id))) {
            restartPump();
        }
        return false;
    }

    void timerEvent(QTimerEvent *) override
    {
        // Reports the changes
        updater->Update(nullptr);

        bool busy = false;
        for (Viewport *v: std::as_const(viewports)) {
            busy = busy || v->interacting();
        }
        if (busy) {
            idleTicks = 0;
        } else if (++idleTicks > maxIdleTicks) {
            // Ends a gesture whose end was not reported
            pump.stop();
            for (Viewport *v: std::as_const(viewports)) {
                v->end();
            }
        }
    }

private:
    IDirectManipulationManager *manager = nullptr;
    IDirectManipulationUpdateManager *updater = nullptr;
    QHash<HWND, Viewport*> viewports;
    QBasicTimer pump;
    int idleTicks = 0;

    void addWindow(QWindow *w)
    {
        // Only the Qt Quick windows: the mobile UI and the welcome panel's QQuickWidget. The main window keeps
        // pinch as Ctrl+wheel, which its plots zoom on. Queued, as the widget is only known once created.
        if (!w || !w->handle() ||
                (!qobject_cast<QQuickWindow*>(w) && !qobject_cast<QQuickWidget*>(QWidget::find(w->winId())))) {
            return;
        }

        // For DM_POINTERHITTEST
        const HWND hwnd = HWND(w->winId());
        if (viewports.contains(hwnd) || FAILED(manager->Activate(hwnd))) {
            return;
        }

        // Never shown, only covers the window. MANUALUPDATE, or it never becomes READY. No pinch inertia.
        Viewport *v = new Viewport(w, hwnd);
        const RECT rect = {0, 0, 1 << 20, 1 << 20};
        const auto config = DIRECTMANIPULATION_CONFIGURATION(DIRECTMANIPULATION_CONFIGURATION_INTERACTION |
                                                             DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_X |
                                                             DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_Y |
                                                             DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_INERTIA |
                                                             DIRECTMANIPULATION_CONFIGURATION_SCALING);
        if (FAILED(manager->CreateViewport(nullptr, hwnd, IID_PPV_ARGS(&v->viewport))) ||
                FAILED(v->viewport->ActivateConfiguration(config)) ||
                FAILED(v->viewport->SetViewportOptions(DIRECTMANIPULATION_VIEWPORT_OPTIONS_MANUALUPDATE)) ||
                FAILED(v->viewport->SetViewportRect(&rect)) ||
                FAILED(v->viewport->AddEventHandler(hwnd, v, &v->cookie)) ||
                FAILED(v->viewport->Enable())) {
            destroy(v);
            return;
        }
        viewports.insert(hwnd, v);
    }

    void destroy(Viewport *v)
    {
        if (v->viewport) {
            v->viewport->Stop();
            if (v->cookie) {
                v->viewport->RemoveEventHandler(v->cookie);
            }
            v->viewport->Abandon();
            v->viewport->Release();
        }
        manager->Deactivate(v->hwnd);
        v->Release();
    }
};

Touchpad *touchpad = nullptr;

void startPump()
{
    touchpad->restartPump();
}

}

#endif

void WinTouchpad::install()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 13, 0)
    // Qt 6.13 does this itself. Never deleted, as COM shuts down before QApplication.
    if (touchpad || QLibraryInfo::version() >= QVersionNumber(6, 13)) {
        return;
    }

    touchpad = new Touchpad;
    if (!touchpad->init()) {
        delete touchpad;
        touchpad = nullptr;
        return;
    }
    qApp->installEventFilter(touchpad);
    qApp->installNativeEventFilter(touchpad);
#endif
}
