/*
    Copyright 2018 Benjamin Vedder	benjamin@vedder.se

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

#include "historylineedit.h"
#include <QDebug>

HistoryLineEdit::HistoryLineEdit(QWidget *parent) :
    QLineEdit(parent)
{
    mIndexNow = 0;
    installEventFilter(this);
}

bool HistoryLineEdit::eventFilter(QObject *object, QEvent *e)
{
    Q_UNUSED(object);

    if (e->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);

        switch(keyEvent->key()) {
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Return:
        case Qt::Key_Enter:
            break;

        default:
            return false;
        }

        bool retval = true;

        switch(keyEvent->key()) {
        case Qt::Key_Up: {
            if (mIndexNow == 0) {
                mCurrentText = text();
            }

            mIndexNow--;

            int ind = mHistory.size() + mIndexNow;
            if (ind >= 0 && ind < mHistory.size()) {
                setText(mHistory.at(ind));
            } else {
                mIndexNow++;
            }
        } break;

        case Qt::Key_Down:
            mIndexNow++;

            if (mIndexNow >= 0) {
                if (mIndexNow == 0) {
                    setText(mCurrentText);
                }
                mIndexNow = 0;
            } else {
                int ind = mHistory.size() + mIndexNow;
                if (ind >= 0 && ind < mHistory.size()) {
                    setText(mHistory.at(ind));
                }
            }
            break;

        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (!text().isEmpty()) {
                if (mHistory.isEmpty() || mHistory.last() != text()) {
                    mHistory.append(text());
                }
            }
            mIndexNow = 0;
            mCurrentText.clear();
            retval = false;
            break;

        default:
            break;
        }

        return retval;
    }

    return false;
}
