/*
    Copyright 2016 - 2017 Benjamin Vedder	benjamin@vedder.se

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

#include "vtextbrowser.h"
#include <QDesktopServices>
#include <QMouseEvent>
#include <QScrollBar>

VTextBrowser::VTextBrowser(QWidget *parent)
    : QTextEdit(parent)
{
    setReadOnly(true);
    setTextInteractionFlags(Qt::TextSelectableByMouse |
                            Qt::LinksAccessibleByMouse |
                            Qt::LinksAccessibleByKeyboard);

    viewport()->setMouseTracking(true);
}

void VTextBrowser::mousePressEvent(QMouseEvent *e)
{
    mLastAnchor = (e->button() & Qt::LeftButton) ? anchorAt(e->pos()) : QString();
    QTextEdit::mousePressEvent(e);
}

void VTextBrowser::mouseReleaseEvent(QMouseEvent *e)
{
    if ((e->button() & Qt::LeftButton) &&
            !mLastAnchor.isEmpty() &&
            anchorAt(e->pos()) == mLastAnchor) {
        QDesktopServices::openUrl(mLastAnchor);
    }

    QTextEdit::mouseReleaseEvent(e);
}

void VTextBrowser::mouseMoveEvent(QMouseEvent *e)
{
    if (!anchorAt(e->pos()).isEmpty()) {
        viewport()->setCursor(Qt::PointingHandCursor);
    } else {
        viewport()->setCursor(Qt::ArrowCursor);
    }

    QTextEdit::mouseMoveEvent(e);
}

void VTextBrowser::trimKeepingLastLines(int maxAmount)
{
    int lines = document()->lineCount();
    int lines_to_remove = lines - maxAmount;
    if (lines_to_remove > 0) {
        int scroll_before = verticalScrollBar()->value();
        int scroll_max_before = verticalScrollBar()->maximum();

        auto cursor = textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, lines_to_remove);
        cursor.removeSelectedText();

        int scroll_max_delta = scroll_max_before - verticalScrollBar()->maximum();
        verticalScrollBar()->setValue(scroll_before - scroll_max_delta);        
    }
}
