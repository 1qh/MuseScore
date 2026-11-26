/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "notationnavigator.h"

#include <algorithm>
#include <cmath>

#include <QQuickWindow>
#include <QScreen>

#include "draw/painter.h"
#include "notation/inotationpainting.h"

#include "log.h"

using namespace muse;
using namespace mu::notation;

NotationNavigatorCursorView::NotationNavigatorCursorView(QQuickItem* parent)
    : QQuickPaintedItem(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

void NotationNavigatorCursorView::paint(QPainter* painter)
{
    TRACEFUNC;

    QColor color(configuration()->selectionColor());
    QPen pen(color, configuration()->borderWidth());
    painter->setPen(pen);
    painter->setBrush(QColor(color.red(), color.green(), color.blue(), configuration()->cursorOpacity()));

    painter->drawRect(m_cursorRect.toQRectF());
}

void NotationNavigatorCursorView::setRect(const RectF& cursorRect)
{
    m_cursorRect = cursorRect;
}

NotationNavigator::NotationNavigator(QQuickItem* parent)
    : AbstractNotationPaintView(parent), m_cursorRectView(new NotationNavigatorCursorView(this))
{
    setReadonly(true);
    connect(this, &QQuickItem::windowChanged, this, &NotationNavigator::observeWindow);
}

void NotationNavigator::load()
{
    TRACEFUNC;

    initOrientation();
    initVisible();
    observeWindow(window());

    uiConfiguration()->currentThemeChanged().onNotify(this, [this]() {
        update();
        m_cursorRectView->update();
    });

    AbstractNotationPaintView::load();

    //! NOTE: avoid double-handling zoom/page-navigation actions already handled by the main view
    if (inputController()->dispatcher()) {
        inputController()->dispatcher()->unReg(inputController());
    }

    rescaleIfVisible();
}

void NotationNavigator::onLoadNotation(INotationPtr notation)
{
    AbstractNotationPaintView::onLoadNotation(notation);
    if (m_fitFullContent) {
        notation->notationChanged().onNotify(&m_contentReceiver, [this]() {
            polish();
        });
        notation->viewModeChanged().onNotify(&m_contentReceiver, [this]() {
            polish();
        });
    }
    rescaleIfVisible();
}

void NotationNavigator::onUnloadNotation(INotationPtr notation)
{
    notation->notationChanged().disconnect(&m_contentReceiver);
    notation->viewModeChanged().disconnect(&m_contentReceiver);
    AbstractNotationPaintView::onUnloadNotation(notation);
}

bool NotationNavigator::isVerticalOrientation() const
{
    return configuration()->canvasOrientation().val == muse::Orientation::Vertical;
}

const PageList& NotationNavigator::pages() const
{
    if (!notation()) {
        static const PageList dummyPages;
        return dummyPages;
    }

    return notation()->elements()->pages();
}

void NotationNavigator::rescaleIfVisible()
{
    TRACEFUNC;

    if (m_fitFullContent) {
        polish();
        return;
    }
    if (!isVisible() || size().isEmpty()) {
        return;
    }

    const PageList& pages = this->pages();
    if (pages.empty()) {
        return;
    }

    const Page* lastPage = pages.back();

    qreal scaling = 1.0;

    if (isVerticalOrientation()) {
        qreal scoreWidth = lastPage->width();
        scaling = width() / scoreWidth;
    } else {
        qreal scoreHeight = lastPage->height();
        scaling = height() / scoreHeight;
    }

    if (qFuzzyIsNull(scaling)) {
        return;
    }

    setScaling(scaling, PointF());
}

void NotationNavigator::wheelEvent(QWheelEvent*)
{
}

void NotationNavigator::mousePressEvent(QMouseEvent* event)
{
    TRACEFUNC;

    PointF logicPos = toLogical(event->pos());
    m_startMove = logicPos;
    if (m_cursorRect.contains(logicPos)) {
        return;
    }

    double dx = logicPos.x() - (m_cursorRect.x() + (m_cursorRect.width() / 2));
    double dy = logicPos.y() - (m_cursorRect.y() + (m_cursorRect.height() / 2));
    emit moveNotationRequested(-dx, -dy);
}

void NotationNavigator::mouseMoveEvent(QMouseEvent* event)
{
    TRACEFUNC;

    PointF logicPos = toLogical(event->pos());
    PointF delta = logicPos - m_startMove;
    emit moveNotationRequested(-delta.x(), -delta.y());

    m_startMove = logicPos;
}

bool NotationNavigator::moveCanvasToRect(const RectF& viewRect)
{
    TRACEFUNC;

    RectF newViewRect = viewRect;
    if (m_fitFullContent) {
        return false;
    }
    RectF viewport = this->viewport();
    RectF notationContentRect = this->notationContentRect();

    qreal dx = 0;
    qreal dy = 0;

    if (isVerticalOrientation()) {
        newViewRect.setHeight(std::min(viewport.height(), newViewRect.height()));

        PointF top = newViewRect.topLeft();
        PointF bottom = newViewRect.bottomRight();

        if (!notationContentRect.contains(top) && !notationContentRect.contains(bottom)) {
            return false;
        }

        if (viewport.top() > top.y()) {
            dy = top.y() - viewport.top();
        } else if (viewport.bottom() < bottom.y()) {
            dy = bottom.y() - viewport.bottom();
        }
    } else {
        newViewRect.setWidth(std::min(viewport.width(), newViewRect.width()));

        PointF left = newViewRect.topLeft();
        PointF right = newViewRect.bottomRight();

        if (!notationContentRect.contains(left) && !notationContentRect.contains(right)) {
            return false;
        }

        if (viewport.left() > left.x()) {
            dx = left.x() - viewport.left();
        } else if (viewport.right() < right.x()) {
            dx = right.x() - viewport.right();
        }
    }

    return moveCanvas(-dx, -dy);
}

void NotationNavigator::setCursorRect(const QRectF& rect)
{
    if (!rect.isValid()) {
        return;
    }

    TRACEFUNC;

    //! NOTE The view's viewport arrives before the score is laid out, when the content is still empty, so it is kept
    //! and intersected again on every refit
    m_viewportRect = rect;
    RectF newCursorRect = notationContentRect().intersected(RectF::fromQRectF(rect));

    bool moved = moveCanvasToRect(newCursorRect);
    m_cursorRect = newCursorRect;

    m_cursorRectView->setSize(this->size());
    m_cursorRectView->setRect(fromLogical(newCursorRect));

    if (moved) {
        update();
    }
    m_cursorRectView->update();
}

int NotationNavigator::orientation() const
{
    return static_cast<int>(configuration()->canvasOrientation().val);
}

void NotationNavigator::initOrientation()
{
    ValCh<muse::Orientation> orientation = configuration()->canvasOrientation();
    orientation.ch.onReceive(&m_contentReceiver, [this](muse::Orientation) {
        moveCanvasToPosition(PointF(0, 0));
        rescaleIfVisible();
        emit orientationChanged();
    });

    emit orientationChanged();
}

void NotationNavigator::initVisible()
{
    connect(this, &NotationNavigator::visibleChanged, [this]() {
        rescaleIfVisible();
    });
}

void NotationNavigator::observeWindow(QQuickWindow* viewWindow)
{
    for (const QMetaObject::Connection& connection : m_windowConnections) {
        disconnect(connection);
    }
    m_windowConnections.clear();
    observeScreen(m_fitFullContent && viewWindow ? viewWindow->screen() : nullptr);
    if (!m_fitFullContent || !viewWindow) {
        return;
    }
    m_windowConnections << connect(viewWindow, &QWindow::screenChanged, this, &NotationNavigator::observeScreen);
    const auto refit = [this]() { polish(); };
    m_windowConnections << connect(viewWindow, &QWindow::widthChanged, this, refit);
    m_windowConnections << connect(viewWindow, &QWindow::heightChanged, this, refit);
    m_windowConnections << connect(viewWindow, &QWindow::xChanged, this, refit);
    m_windowConnections << connect(viewWindow, &QWindow::yChanged, this, refit);
    m_windowConnections << connect(viewWindow, &QWindow::visibleChanged, this, refit);
}

void NotationNavigator::observeScreen(QScreen* screen)
{
    disconnect(m_screenConnection);
    if (screen) {
        m_screenConnection = connect(screen, &QScreen::availableGeometryChanged, this, [this]() {
            polish();
        });
    }
    polish();
}

ViewMode NotationNavigator::notationViewMode() const
{
    if (!notation()) {
        return ViewMode::PAGE;
    }

    return notation()->viewMode();
}

void NotationNavigator::paint(QPainter* painter)
{
    if (!isVisible()) {
        return;
    }

    TRACEFUNC;

    if (m_fitFullContent) {
        if (!notation()) {
            return;
        }
        painter->fillRect(boundingRect(), configuration()->backgroundColor());
        //! NOTE As in the base paint, the matrix is in GUI-scaled units, so painting multiplies the GUI scaling back in
        const qreal guiScaling = configuration()->guiScaling();
        const QTransform fittedMatrix = matrix().value<QTransform>() * QTransform::fromScale(guiScaling, guiScaling);
        muse::draw::Painter scorePainter(painter, objectName().toStdString());
        scorePainter.setWorldTransform(muse::draw::Transform::fromQTransform(fittedMatrix));
        notation()->painting()->paintView(&scorePainter, viewport(), true);
        painter->setWorldTransform(fittedMatrix);
    } else {
        AbstractNotationPaintView::paint(painter);
    }

    paintPageNumbers(painter);
}

void NotationNavigator::updatePolish()
{
    if (!m_fitFullContent || !isVisible()) {
        return;
    }
    const RectF content = notationContentRect();
    if (!content.isValid() || !std::isfinite(content.width()) || !std::isfinite(content.height())) {
        return;
    }
    qreal scaling = 1.0;
    if (const QQuickWindow* viewWindow = window()) {
        if (const QScreen* screen = viewWindow->screen()) {
            const QMargins margins = viewWindow->frameMargins();
            const QSize available = screen->availableGeometry().size();
            const qreal extraWidth = std::max(qreal(0.0), viewWindow->width() - width()) + margins.left() + margins.right();
            const qreal extraHeight = std::max(qreal(0.0), viewWindow->height() - height()) + margins.top() + margins.bottom();
            const qreal availableWidth = std::max(qreal(1.0), std::floor(available.width() - extraWidth));
            const qreal availableHeight = std::max(qreal(1.0), std::floor(available.height() - extraHeight));
            scaling = std::min({ scaling, availableWidth / content.width(), availableHeight / content.height() });
        }
    }
    const qreal fittedWidth = content.width() * scaling;
    const qreal fittedHeight = content.height() * scaling;
    setImplicitSize(std::ceil(fittedWidth), std::ceil(fittedHeight));
    if (size().isEmpty()) {
        return;
    }
    muse::draw::Transform matrix;
    matrix.translate((width() - fittedWidth) / 2, (height() - fittedHeight) / 2);
    matrix.scale(scaling, scaling);
    matrix.translate(-content.left(), -content.top());
    setMatrix(matrix);
    setCursorRect(m_viewportRect);
    update();
}

void NotationNavigator::onViewSizeChanged()
{
    rescaleIfVisible();
}

void NotationNavigator::paintPageNumbers(QPainter* painter)
{
    if (notationViewMode() != ViewMode::PAGE) {
        return;
    }

    TRACEFUNC;

    constexpr int PAGE_NUMBER_FONT_SIZE = 2000;
    QFont font(QString::fromStdString(configuration()->fontFamily()), PAGE_NUMBER_FONT_SIZE);

    painter->setClipping(false);
    painter->setFont(font);
    painter->setPen(engravingConfiguration()->scoreGreyColor().toQColor());

    for (const Page* page : pages()) {
        painter->translate(page->pos().toQPointF());
        painter->drawText(page->ldata()->bbox().toQRectF(), Qt::AlignCenter, QString("%1").arg(page->pageNumber() + 1));
        painter->translate(-page->pos().toQPointF());
    }
}
