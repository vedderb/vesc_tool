#ifndef LINENUMBERAREA_H
#define LINENUMBERAREA_H

#include <QDebug>
#include <QPainter>
#include <QScrollBar>
#include <QWidget>

#include "qmarkdowntextedit.h"

class LineNumArea final : public QWidget {
    Q_OBJECT

public:
    explicit LineNumArea(QMarkdownTextEdit *parent)
        : QWidget(parent),
          textEdit(parent)
    {
        Q_ASSERT(parent);

        _currentLineColor = QColor(QStringLiteral("#eef067"));
        _otherLinesColor = QColor(QStringLiteral("#a6a6a6"));
        setHidden(true);

        // We always use fixed font to avoid "width" issues
        setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    }

    void setCurrentLineColor(QColor color) {
        _currentLineColor = color;
    }

    void setOtherLineColor(QColor color) {
        _otherLinesColor = std::move(color);
    }

    int lineNumAreaWidth() const
    {
        if (!enabled) {
            return 0;
        }

        int digits = 2;
        int max = std::max(1, textEdit->blockCount());
        while (max >= 10) {
            max /= 10;
            ++digits;
        }

#if QT_VERSION >= 0x050B00
        int space = 13 + textEdit->fontMetrics().horizontalAdvance(u'9') * digits;
#else
        int space = 13 + textEdit->fontMetrics().width(QLatin1Char('9')) * digits;
#endif

        return space;
    }

    bool isLineNumAreaEnabled() const
    {
        return enabled;
    }

    void setLineNumAreaEnabled(bool e)
    {
        enabled = e;
        setHidden(!e);
    }

    QSize sizeHint() const override
    {
        return {lineNumAreaWidth(), 0};
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QPainter painter(this);

        painter.fillRect(event->rect(), palette().color(QPalette::Active, QPalette::Window));

        auto block = textEdit->firstVisibleBlock();
        int blockNumber = block.blockNumber();
        qreal top = textEdit->blockBoundingGeometry(block).translated(textEdit->contentOffset()).top();
        // Maybe the top is not 0?
        top += textEdit->viewportMargins().top();
        qreal bottom = top;

        const QPen currentLine = _currentLineColor;
        const QPen otherLines = _otherLinesColor;
        painter.setFont(font());

        while (block.isValid() && top <= event->rect().bottom()) {
            top = bottom;
            bottom = top + textEdit->blockBoundingRect(block).height();
            if (block.isVisible() && bottom >= event->rect().top()) {
                QString number = QString::number(blockNumber + 1);

                auto isCurrentLine = textEdit->textCursor().blockNumber() == blockNumber;
                painter.setPen(isCurrentLine ? currentLine : otherLines);

                painter.drawText(
                    -5,
                    top,
                    sizeHint().width(),
                    textEdit->fontMetrics().height(),
                    Qt::AlignRight,
                    number
                );
            }

            block = block.next();
            ++blockNumber;
        }
    }

private:
    bool enabled = false;
    QMarkdownTextEdit *textEdit;
    QColor _currentLineColor;
    QColor _otherLinesColor;
};

#endif // LINENUMBERAREA_H
