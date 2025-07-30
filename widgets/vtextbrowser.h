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

#ifndef VTEXTBROWSER_H
#define VTEXTBROWSER_H

#include <QTextEdit>
#include <QScrollBar>

class VTextBrowser : public QTextEdit
{
    Q_OBJECT

public:
    explicit VTextBrowser(QWidget *parent = 0);

    void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    // Run `operation` with a cursor moved to the end of the document, making
    // sure to scroll to the bottom if the operation changed the content's
    // length and the view was at the bottom before.
    // `operation` should be a callable taking a single `QTextCursor`.
    template<typename F>
    void doAtEndWithScroll(const F operation) {
        // How far in pixels the scroll bar is allowed to be from the bottom
        // while still being snapped to the bottom.
        static const int bottom_margin = 30;

        int scroll_before = verticalScrollBar()->value();
        bool was_at_end = scroll_before >= verticalScrollBar()->maximum() - bottom_margin;

        auto cursor = textCursor();
        cursor.movePosition(QTextCursor::End);
        operation(cursor);

        if (was_at_end) {
            verticalScrollBar()->setValue(verticalScrollBar()->maximum());
        } else {
            verticalScrollBar()->setValue(scroll_before);
        }
    };
    // Remove the first lines so that only the last `maxAmount` of lines remain.
    // The scroll position is maintained so that the user doesn't perceive the
    // change.
    void trimKeepingLastLines(int maxAmount);

private:
    QString mLastAnchor;

};

#endif // VTEXTBROWSER_H
