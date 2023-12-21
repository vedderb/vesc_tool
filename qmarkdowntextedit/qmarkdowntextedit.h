/*
 * MIT License
 *
 * Copyright (c) 2014-2023 Patrizio Bekerle -- <patrizio@bekerle.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <QEvent>
#include <QPlainTextEdit>

#include "markdownhighlighter.h"
#include "qplaintexteditsearchwidget.h"

class LineNumArea;

class QMarkdownTextEdit : public QPlainTextEdit {
    Q_OBJECT
    Q_PROPERTY(
        bool highlighting READ highlightingEnabled WRITE setHighlightingEnabled)

    friend class LineNumArea;

   public:
    enum AutoTextOption {
        None = 0x0000,

        // inserts closing characters for brackets and markdown characters
        BracketClosing = 0x0001,

        // removes matching brackets and markdown characters
        BracketRemoval = 0x0002
    };

    Q_DECLARE_FLAGS(AutoTextOptions, AutoTextOption)

    explicit QMarkdownTextEdit(QWidget *parent = nullptr,
                               bool initHighlighter = true);
    MarkdownHighlighter *highlighter();
    QPlainTextEditSearchWidget *searchWidget();
    void setIgnoredClickUrlSchemata(QStringList ignoredUrlSchemata);
    virtual void openUrl(const QString &urlString);
    QString getMarkdownUrlAtPosition(const QString &text, int position);
    void initSearchFrame(QWidget *searchFrame, bool darkMode = false);
    void setAutoTextOptions(AutoTextOptions options);
    static bool isValidUrl(const QString &urlString);
    void resetMouseCursor() const;
    void setReadOnly(bool ro);
    void doSearch(QString &searchText,
                  QPlainTextEditSearchWidget::SearchMode searchMode =
                      QPlainTextEditSearchWidget::SearchMode::PlainTextMode);
    void hideSearchWidget(bool reset);
    void updateSettings();
    void setLineNumbersCurrentLineColor(QColor color);
    void setLineNumbersOtherLineColor(QColor color);
    void setSearchWidgetDebounceDelay(uint debounceDelay);

    void setHighlightingEnabled(bool enabled);
    [[nodiscard]] bool highlightingEnabled() const;

    void setHighlightCurrentLine(bool set);
    bool highlightCurrentLine();

    void setCurrentLineHighlightColor(const QColor &c);
    QColor currentLineHighlightColor();

   public Q_SLOTS:
    void duplicateText();
    void setText(const QString &text);
    void setPlainText(const QString &text);
    void adjustRightMargin();
    void hide();
    bool openLinkAtCursorPosition();
    bool handleBackspaceEntered();
    void centerTheCursor();
    void undo();
    void moveTextUpDown(bool up);
    void setLineNumberEnabled(bool enabled);

   protected:
    QTextCursor _textCursor;
    MarkdownHighlighter *_highlighter = nullptr;
    bool _highlightingEnabled;
    QStringList _ignoredClickUrlSchemata;
    QPlainTextEditSearchWidget *_searchWidget;
    QWidget *_searchFrame;
    AutoTextOptions _autoTextOptions;
    bool _mouseButtonDown = false;
    bool _centerCursor = false;
    bool _highlightCurrentLine = false;
    QColor _currentLineHighlightColor = QColor();
    uint _debounceDelay = 0;

    bool eventFilter(QObject *obj, QEvent *event) override;
    QMargins viewportMargins();
    bool increaseSelectedTextIndention(
        bool reverse,
        const QString &indentCharacters = QChar::fromLatin1('\t'));
    bool handleTabEntered(bool reverse, const QString &indentCharacters =
                                            QChar::fromLatin1('\t'));
    QMap<QString, QString> parseMarkdownUrlsFromText(const QString &text);
    bool handleReturnEntered();
    bool handleBracketClosing(const QChar openingCharacter,
                              QChar closingCharacter = QChar());
    bool bracketClosingCheck(const QChar openingCharacter,
                             QChar closingCharacter);
    bool quotationMarkCheck(const QChar quotationCharacter);
    void focusOutEvent(QFocusEvent *event) override;
    void paintEvent(QPaintEvent *e) override;
    bool handleCharRemoval(MarkdownHighlighter::RangeType type, int block, int position);
    void resizeEvent(QResizeEvent *event) override;
    void setLineNumberLeftMarginOffset(int offset);
    int _lineNumberLeftMarginOffset = 0;
    LineNumArea *lineNumberArea()
    {
        return _lineNumArea;
    }
    void updateLineNumAreaGeometry();
    void updateLineNumberArea(const QRect rect, int dy);
    Q_SLOT void updateLineNumberAreaWidth(int);
    bool _handleBracketClosingUsed;
    LineNumArea *_lineNumArea;

   Q_SIGNALS:
    void urlClicked(QString url);
    void zoomIn();
    void zoomOut();
};
