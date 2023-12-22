// QCodeEditor
#include <QLineNumberArea>
#include <QSyntaxStyle>
#include <QCodeEditor>
#include <QStyleSyntaxHighlighter>
#include <QCXXHighlighter>

// Qt
#include <QTextBlock>
#include <QPaintEvent>
#include <QFontDatabase>
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>
#include <QTextCharFormat>
#include <QCursor>
#include <QCompleter>
#include <QAbstractItemView>
#include <QShortcut>
#include <QMimeData>
#include <QDebug>
#include "utility.h"

static QVector<QPair<QString, QString>> parentheses = {
    {"(", ")"},
    {"{", "}"},
    {"[", "]"},
};

QCodeEditor::QCodeEditor(QWidget* widget) :
    QTextEdit(widget),
    m_highlighter(nullptr),
    m_syntaxStyle(nullptr),
    m_lineNumberArea(new QLineNumberArea(this)),
    m_completer(nullptr),
    m_searchIsCaseSensitive(false),
    m_autoIndentation(true),
    m_autoParentheses(true),
    m_replaceTab(true),
    m_tabReplace(QString(4, ' ')),
    m_commentStr("//"),
    m_indentStartStr("{"),
    m_indentEndStr("}"),
    m_separateMinus(true),
    m_highlightBlocks(false)
{

    initFont();
    performConnections();

    if (Utility::isDarkMode()) {
        setSyntaxStyle(QSyntaxStyle::darkStyle());
    } else {
        setSyntaxStyle(QSyntaxStyle::defaultStyle());
    }
}

void QCodeEditor::initFont()
{    
    QFont font;
    font.setFamily("DejaVu Sans Mono");
    font.setPointSize(11);

    setFont(font);
}

void QCodeEditor::performConnections()
{
    connect(
        document(),
        &QTextDocument::blockCountChanged,
        this,
        &QCodeEditor::updateLineNumberAreaWidth
    );

    connect(
        verticalScrollBar(),
        &QScrollBar::valueChanged,
        [this](int){ m_lineNumberArea->update(); }
    );

    connect(
        this,
        &QTextEdit::cursorPositionChanged,
        this,
        &QCodeEditor::updateExtraSelection
    );
}

void QCodeEditor::setHighlighter(QStyleSyntaxHighlighter* highlighter)
{
    if (m_highlighter) {
        m_highlighter->setDocument(nullptr);
    }

    m_highlighter = highlighter;

    if (m_highlighter) {
        m_highlighter->setSyntaxStyle(m_syntaxStyle);
        m_highlighter->setDocument(document());
    }
}

void QCodeEditor::setSyntaxStyle(QSyntaxStyle* style)
{
    m_syntaxStyle = style;
    m_lineNumberArea->setSyntaxStyle(m_syntaxStyle);

    if (m_highlighter) {
        m_highlighter->setSyntaxStyle(m_syntaxStyle);
    }

    updateStyle();
}

void QCodeEditor::updateStyle()
{
    if (m_highlighter) {
        m_highlighter->rehighlight();
    }

    if (m_syntaxStyle) {
        auto currentPalette = palette();

        // Setting text format/color
        currentPalette.setColor(
            QPalette::ColorRole::Text,
            m_syntaxStyle->getFormat("Text").foreground().color()
        );

        // Setting common background
        currentPalette.setColor(
            QPalette::Base,
            m_syntaxStyle->getFormat("Text").background().color()
        );

        // Setting selection color
        currentPalette.setColor(
            QPalette::Highlight,
            m_syntaxStyle->getFormat("Selection").background().color()
        );

        setPalette(currentPalette);
    }

    updateExtraSelection();
}

void QCodeEditor::resizeEvent(QResizeEvent* e)
{
    QTextEdit::resizeEvent(e);
    updateLineGeometry();
}

void QCodeEditor::updateLineGeometry()
{
    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(
        QRect(cr.left(),
              cr.top(),
              m_lineNumberArea->sizeHint().width(),
              cr.height()
        )
    );
}

void QCodeEditor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(m_lineNumberArea->sizeHint().width(), 0, 0, 0);
}

void QCodeEditor::updateLineNumberArea(const QRect& rect)
{
    m_lineNumberArea->update(
        0,
        rect.y(),
        m_lineNumberArea->sizeHint().width(),
        rect.height()
    );
    updateLineGeometry();

    if (rect.contains(viewport()->rect()))
    {
        updateLineNumberAreaWidth(0);
    }
}

void QCodeEditor::updateExtraSelection()
{
    QList<QTextEdit::ExtraSelection> extra;

    highlightSearch(extra);
    highlightCurrentLine(extra);
    highlightParenthesis(extra, false);

    setExtraSelections(extra);
}

void QCodeEditor::highlightParenthesis(
        QList<QTextEdit::ExtraSelection>& extraSelection,
        bool selectBlock)
{
    auto currentSymbol = charUnderCursor();
    auto prevSymbol = charUnderCursor(-1);

    m_highlightedBlock.clear();

    for (auto& pair : parentheses) {
        int direction;

        QChar counterSymbol;
        QChar activeSymbol;
        auto position = textCursor().position();

        if (pair.first == currentSymbol) {
            direction = 1;
            counterSymbol = pair.second[0];
            activeSymbol = currentSymbol;
        } else if (pair.second == prevSymbol) {
            direction = -1;
            counterSymbol = pair.first[0];
            activeSymbol = prevSymbol;
            position--;
        } else {
            continue;
        }

        auto counter = 1;

        while (counter != 0 &&
               position > 0 &&
               position < (document()->characterCount() - 1))
        {
            // Moving position
            position += direction;

            auto character = document()->characterAt(position);
            // Checking symbol under position
            if (character == activeSymbol) {
                ++counter;
            } else if (character == counterSymbol) {
                --counter;
            }
        }

        auto format = m_syntaxStyle->getFormat("Parentheses");

        QList<QTextEdit::ExtraSelection> selectedParantheses;

        // Found
        if (counter == 0) {
            ExtraSelection selection{};

            auto directionEnum =
                 direction < 0 ?
                 QTextCursor::MoveOperation::Left
                 :
                 QTextCursor::MoveOperation::Right;

            selection.format = format;
            selection.cursor = textCursor();
            selection.cursor.clearSelection();
            selection.cursor.movePosition(
                directionEnum,
                QTextCursor::MoveMode::MoveAnchor,
                std::abs(textCursor().position() - position)
            );

            selection.cursor.movePosition(
                QTextCursor::MoveOperation::Right,
                QTextCursor::MoveMode::KeepAnchor,
                1
            );

            selectedParantheses.append(selection);

            selection.cursor = textCursor();
            selection.cursor.clearSelection();
            selection.cursor.movePosition(
                directionEnum,
                QTextCursor::MoveMode::KeepAnchor,
                1
            );

            selectedParantheses.append(selection);
        }

        extraSelection.append(selectedParantheses);

        // Block contained in parantheses
        if (selectedParantheses.length() == 2) {
            int first = selectedParantheses.first().cursor.position();
            int second = selectedParantheses.at(1).cursor.position();

            if (first > second) {
                int temp = first;
                first = second - 1;
                second = temp;
            } else {
                first--;
                second++;
            }

            auto tc = textCursor();
            tc.setPosition(first);
            tc.movePosition(
                        QTextCursor::MoveOperation::Right,
                        QTextCursor::MoveMode::KeepAnchor,
                        second - first
                        );

            if (selectBlock) {
                QTextEdit::ExtraSelection selection{};
                selection.format = m_syntaxStyle->getFormat("HighlightedBlock");
                selection.cursor = tc;
                extraSelection.append(selection);
                m_highlightedBlock = tc.selectedText().replace("\u2029", "\n");
            }
        }

        break;
    }
}

void QCodeEditor::highlightSearch(QList<QTextEdit::ExtraSelection> &extraSelection)
{
    if (m_searchStrNow.isEmpty()) {
        return;
    }

    auto tc = textCursor();
    tc.setPosition(0);
    auto format = m_syntaxStyle->getFormat("SearchResult");
    auto flags = m_searchIsCaseSensitive ? QTextDocument::FindCaseSensitively : QTextDocument::FindFlags();

    auto tc2 = document()->find(m_searchStrNow, tc, flags);
    auto tcLast = tc2;
    auto tcFirst = tc2;

    bool lastAbort = false;
    m_searchMatches = 0;

    while (!tc2.isNull()) {
        ExtraSelection sel;
        sel.cursor = tc2;
        sel.format = format;
        extraSelection.append(sel);
        m_searchMatches++;

        if (m_searchSelectNext && tc2.position() > textCursor().position()) {
            m_searchSelectNext = false;
            setTextCursor(tc2);
        }

        if (!lastAbort && m_searchSelectPrev &&
                tc2.position() >= textCursor().position()) {

            if (!tcLast.isNull()) {
                if (tcLast.position() == textCursor().position()) {
                    lastAbort = true;
                } else {
                    m_searchSelectPrev = false;
                    setTextCursor(tcLast);
                }
            }
        }

        tcLast = tc2;
        tc2 = document()->find(m_searchStrNow, tc2, flags);
    }

    if (m_searchSelectNext && !tcFirst.isNull()) {
        m_searchSelectNext = false;

        if (!tcFirst.isNull()) {
            setTextCursor(tcFirst);
        }
    }

    if (m_searchSelectPrev) {
        m_searchSelectPrev = false;

        if (!tcLast.isNull()) {
            setTextCursor(tcLast);
        }
    }
}

void QCodeEditor::highlightCurrentLine(QList<QTextEdit::ExtraSelection>& extraSelection)
{
    if (!isReadOnly())
    {
        QTextEdit::ExtraSelection selection{};

        selection.format = m_syntaxStyle->getFormat("CurrentLine");
        selection.format.setForeground(QBrush());
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();

        extraSelection.append(selection);
    }
}

void QCodeEditor::paintEvent(QPaintEvent* e)
{
    updateLineNumberArea(e->rect());
    QTextEdit::paintEvent(e);
}

int QCodeEditor::getFirstVisibleBlock()
{
    // Detect the first block for which bounding rect - once translated
    // in absolute coordinated - is contained by the editor's text area

    // Costly way of doing but since "blockBoundingGeometry(...)" doesn't
    // exists for "QTextEdit"...

    QTextCursor curs = QTextCursor(document());
    curs.movePosition(QTextCursor::Start);
    for(int i=0; i < document()->blockCount(); ++i)
    {
        QTextBlock block = curs.block();

        QRect r1 = viewport()->geometry();
        QRect r2 = document()
            ->documentLayout()
            ->blockBoundingRect(block)
            .translated(
                viewport()->geometry().x(),
                viewport()->geometry().y() - verticalScrollBar()->sliderPosition()
            ).toRect();

        if (r1.intersects(r2))
        {
            return i;
        }

        curs.movePosition(QTextCursor::NextBlock);
    }

    return 0;
}

bool QCodeEditor::proceedCompleterBegin(QKeyEvent *e)
{
    if (m_completer &&
        m_completer->popup()->isVisible())
    {
        switch (e->key())
        {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            e->ignore();
            return true; // let the completer do default behavior
        default:
            break;
        }
    }

    // todo: Replace with modifiable QShortcut
    auto isShortcut = ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_Space);

    return !(!m_completer || !isShortcut);

}

void QCodeEditor::proceedCompleterEnd(QKeyEvent *e)
{
    auto ctrlOrShift = e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);

    auto tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    auto word = tc.selectedText();

    if (!m_completer ||
            (ctrlOrShift && e->text().isEmpty()) ||
            e->key() == Qt::Key_Delete)
    {
        return;
    }

    bool isComment = false;
    for (auto f: textCursor().block().layout()->formats()) {
        int pos = textCursor().positionInBlock();
        if (pos >= f.start && pos <= (f.start + f.length)) {
            if (f.format == m_syntaxStyle->getFormat("Comment") ||
                    f.format == m_syntaxStyle->getFormat("String")) {
                isComment = true;
            }
        }
    }

    auto isShortcut = ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_Space);
    auto isDot = charUnderCursor(-1) == ".";
    auto isMinus = charUnderCursor(-1) == "-";
    auto isSpace = charUnderCursor(-1) == " " || charUnderCursor(-1) == "\t";

    if (isSpace || (word.isEmpty() && !isDot && !(!m_separateMinus && isMinus))) {
        m_completer->popup()->hide();
        return;
    }

    QString completionPrefix = getCompletionWordNow(0, 0);

    QString eow;
    if (m_separateMinus) {
        eow = R"(~!@#$%^&*()-+{}|:"<>?,/;'[]\=)";
    } else {
        eow = R"(~!@#$%^&*()+{}|:"<>?,/;'[]\=)";
    }

    if (!isShortcut &&
            (isComment ||
             e->text().isEmpty() ||
             completionPrefix.length() < 2 ||
             eow.contains(e->text().right(1))))
    {
        m_completer->popup()->hide();
        return;
    }

    if (completionPrefix != m_completer->completionPrefix())
    {
        m_completer->setCompletionPrefix(completionPrefix);
        m_completer->popup()->setCurrentIndex(m_completer->completionModel()->index(0, 0));
    }

    auto cursRect = cursorRect();
    cursRect.setWidth(
        m_completer->popup()->sizeHintForColumn(0) +
        m_completer->popup()->verticalScrollBar()->sizeHint().width()
    );

    m_completer->complete(cursRect);
}

void QCodeEditor::keyPressEvent(QKeyEvent* e) {
    const int defaultIndent = 4;

    if (e->key() == Qt::Key_Control && m_highlightBlocks) {
        QList<QTextEdit::ExtraSelection> extra;
        highlightParenthesis(extra, true);

        if (extra.length() > 1) {
            setExtraSelections(extra);
        }
    }

    if (e->key() == Qt::Key_R && e->modifiers() & Qt::ControlModifier) {
        if (!m_highlightedBlock.isEmpty()) {
            emit runBlockTriggered(m_highlightedBlock);
        }
    }

    auto completerSkip = proceedCompleterBegin(e);

    if (!completerSkip) {
        auto tcStart = textCursor();
        auto tcEnd = textCursor();
        tcStart.setPosition(textCursor().selectionStart());
        tcEnd.setPosition(textCursor().selectionEnd());

        int lineStart = tcStart.blockNumber();
        int lineEnd = tcEnd.blockNumber();

        // Toggle block comment
        if (e->text() == "\u001F" ||
                (e->modifiers() & Qt::ControlModifier) && e->text() == "#") {
            for (int i = lineStart;i <= lineEnd;i++) {
                auto tc = textCursor();

                int posStart = 0;
                tc.setPosition(posStart++);
                while (!tc.atEnd()) {
                    if (tc.blockNumber() == i) {
                        tc.select(QTextCursor::LineUnderCursor);
                        auto line = tc.selectedText();
                        auto line2 = line;
                        line2.replace(" ", "");
                        line2.replace("\t", "");
                        if (line2.startsWith(m_commentStr)) {
                            line.replace(line.indexOf(m_commentStr.at(0)), m_commentStr.size(), "");
                        } else {
                            line.prepend(m_commentStr);
                        }

                        tc.insertText(line);
                        break;
                    }

                    tc.setPosition(posStart++);
                }
            }

            return;
        }

        if (e->modifiers() == Qt::ControlModifier) {
            if (e->key() == Qt::Key_S) {
                emit saveTriggered();
                updateExtraSelection();
                return;
            } else if (e->key() == Qt::Key_E) {
                emit runEmbeddedTriggered();
                updateExtraSelection();
                return;
            } else if (e->key() == Qt::Key_W) {
                emit runWindowTriggered();
                updateExtraSelection();
                return;
            } else if (e->key() == Qt::Key_Q) {
                emit stopTriggered();
                updateExtraSelection();
                return;
            } else if (e->key() == Qt::Key_D) {
                emit clearConsoleTriggered();
                updateExtraSelection();
                return;
            } else if (e->key() == Qt::Key_F) {
                emit searchTriggered();
                updateExtraSelection();
                return;
            }
        }

        if (e->key() == Qt::Key_Plus && e->modifiers() == Qt::ControlModifier) {
            auto f = font();
            if (f.pointSize() < 50) {
                f.setPointSize(f.pointSize() + 1);
                setFont(f);
            }
            return;
        }

        if (e->key() == Qt::Key_Minus && e->modifiers() == Qt::ControlModifier) {
            auto f = font();
            if (f.pointSize() > 4) {
                f.setPointSize(f.pointSize() - 1);
                setFont(f);
            }
            return;
        }

        // Have Qt Edior like behaviour, if {|} and enter is pressed indent the two
        // parenthesis
        bool indentNext = false;
        auto charBefore = charUnderCursor(-1);
        auto charAfter = charUnderCursor();
        if (m_autoIndentation && (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) &&
                m_indentStartStr.contains(charBefore) && m_indentEndStr.contains(charAfter)) {
            insertPlainText("\n\n");
            moveCursor(QTextCursor::MoveOperation::Left);
            indentNext = true;

            auto tcStart = textCursor();
            auto tcEnd = textCursor();
            tcStart.setPosition(textCursor().selectionStart());
            tcEnd.setPosition(textCursor().selectionEnd());
            lineStart = tcStart.blockNumber();
            lineEnd = tcEnd.blockNumber() + 1;
        }

        // Auto-indent selected line or block
        if (indentNext || (e->key() == Qt::Key_I && e->modifiers() == Qt::ControlModifier)) {
            auto txtOld = toPlainText();
            int indentNow = 0;
            bool isComment = false;

            int lineNum = -1;
            foreach (auto line, txtOld.split("\n")) {
                lineNum++;

                bool indent = true;

                if (m_indentStartStr.isEmpty()) {
                    indent = false;
                }

                bool removeTrailing = true;

                if (lineNum < lineStart || lineNum > lineEnd) {
                    indent = false;
                    removeTrailing = false;
                }

                if (isComment) {
                    if (line.contains("*/") && !line.contains("/*")) {
                        isComment = false;
                    }

                    indent = false;
                } else {
                    if (line.contains("/*") && !line.contains("*/")) {
                        isComment = true;
                    }
                }

                if (indent) {
                    while (line.startsWith(" ") || line.startsWith("\t")) {
                        line.remove(0, 1);
                    }
                }

                if (removeTrailing) {
                    while (line.endsWith(" ") || line.endsWith("\t")) {
                        line.remove(line.size() - 1, 1);
                    }
                }

                int indentLine = 0;
                for (auto c: line) {
                    if (m_indentStartStr.contains(c)) {
                        indentNow++;
                        indentLine++;
                    }
                }

                for (auto c: line) {
                    if (m_indentEndStr.contains(c)) {
                        indentNow--;
                        indentLine--;
                    }
                }

                if (indentLine < 0) {
                    indentLine = 0;
                }

                if (indent) {
                    for (int i = 0;i < (indentNow - indentLine);i++) {
                        line.prepend(m_tabReplace);
                    }
                }

                if (indent || removeTrailing) {
                    auto tc = textCursor();
                    int posStart = 0;
                    tc.setPosition(posStart++);
                    while (!tc.atEnd()) {
                        if (tc.blockNumber() == lineNum) {
                            tc.select(QTextCursor::LineUnderCursor);
                            if (tc.selectedText() != line) {
                                tc.insertText(line);
                            }
                            break;
                        }
                        tc.setPosition(posStart++);
                    }
                }
            }

            return;
        }

        if (m_replaceTab && e->key() == Qt::Key_Tab &&
                e->modifiers() == Qt::NoModifier) {

            // Make indentation multiple of defaultIndent
            auto tc = textCursor();
            int col = tc.columnNumber();
            int cursorPos = tc.position();
            tc.select(QTextCursor::LineUnderCursor);
            QString line = tc.selectedText();
            int removePos = 0;
            while (line.startsWith(" ") || line.startsWith("\t")) {
                line.remove(0, 1);
                removePos++;
            }

            if (removePos >= col) {
                int indPos = 0;
                for (int i = 0; i < (col / defaultIndent);i++) {
                    line.prepend(m_tabReplace);
                    indPos += defaultIndent;
                }
                line.prepend(m_tabReplace);
                indPos += defaultIndent;
                tc.insertText(line);
                tc.setPosition(cursorPos + indPos - col);
                setTextCursor(tc);
                return;
            }

            insertPlainText(m_tabReplace);
            return;
        }

        // Auto indentation
        int indentationLevel = getIndentationSpaces();

#if QT_VERSION >= 0x050A00
        int tabCounts =
                indentationLevel * fontMetrics().averageCharWidth() / tabStopDistance();
#else
        int tabCounts =
                indentationLevel * fontMetrics().averageCharWidth() / tabStopWidth();
#endif

        // Shortcut for moving line to left
        if (m_replaceTab && e->key() == Qt::Key_Backtab) {
            indentationLevel = std::min(indentationLevel, m_tabReplace.size());

            auto cursor = textCursor();

            cursor.movePosition(QTextCursor::MoveOperation::StartOfLine);
            cursor.movePosition(QTextCursor::MoveOperation::Right,
                                QTextCursor::MoveMode::KeepAnchor, indentationLevel);

            cursor.removeSelectedText();
            return;
        }

        QTextEdit::keyPressEvent(e);

        if (m_autoIndentation && (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)) {
            if (m_replaceTab)
                insertPlainText(QString(indentationLevel, ' '));
            else
                insertPlainText(QString(tabCounts, '\t'));
        }

        if (m_autoParentheses)
        {
            for (auto&& el : parentheses)
            {
                // Inserting closed brace
                if (el.first == e->text())
                {
                    insertPlainText(el.second);
                    moveCursor(QTextCursor::MoveOperation::Left);
                    break;
                }
            }
        }
    }

    proceedCompleterEnd(e);
}

void QCodeEditor::keyReleaseEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Control && m_highlightBlocks) {
        updateExtraSelection();
    }
}

void QCodeEditor::setAutoIndentation(bool enabled)
{
    m_autoIndentation = enabled;
}

bool QCodeEditor::autoIndentation() const
{
    return m_autoIndentation;
}

void QCodeEditor::setAutoParentheses(bool enabled)
{
    m_autoParentheses = enabled;
}

bool QCodeEditor::autoParentheses() const
{
    return m_autoParentheses;
}

void QCodeEditor::setTabReplace(bool enabled)
{
    m_replaceTab = enabled;
}

bool QCodeEditor::tabReplace() const
{
    return m_replaceTab;
}

void QCodeEditor::setTabReplaceSize(int val)
{
    m_tabReplace.clear();

    m_tabReplace.fill(' ', val);
}

int QCodeEditor::tabReplaceSize() const
{
    return m_tabReplace.size();
}

void QCodeEditor::setCompleter(QCompleter *completer)
{
    if (m_completer)
    {
        disconnect(m_completer, nullptr, this, nullptr);
    }

    m_completer = completer;

    if (!m_completer)
    {
        return;
    }

    m_completer->setWidget(this);
    m_completer->setCompletionMode(QCompleter::CompletionMode::PopupCompletion);

    connect(
        m_completer,
        QOverload<const QString&>::of(&QCompleter::activated),
        this,
        &QCodeEditor::insertCompletion
    );
}

void QCodeEditor::focusInEvent(QFocusEvent *e)
{
    if (m_completer)
    {
        m_completer->setWidget(this);
    }

    QTextEdit::focusInEvent(e);
}

void QCodeEditor::insertCompletion(QString s)
{
    if (m_completer->widget() != this) {
        return;
    }

    int posStart = 0;
    int posEnd = 0;
    getCompletionWordNow(&posStart, &posEnd);
    auto tc = textCursor();
    tc.select(QTextCursor::LineUnderCursor);
    auto line = tc.selectedText();

    QString lineNew = line.mid(0, posStart + 1) + s + line.mid(posEnd + 1);
    tc.insertText(lineNew);
    tc.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
    tc.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, posStart + s.size() + 1);
    setTextCursor(tc);
}

QCompleter *QCodeEditor::completer() const
{
    return m_completer;
}

void QCodeEditor::searchForString(QString str)
{
    m_searchStrNow = str;
    m_searchSelectNext = false;
    m_searchSelectPrev = false;
    updateExtraSelection();
}

void QCodeEditor::searchNextResult()
{
    m_searchSelectNext = true;
    updateExtraSelection();
}

void QCodeEditor::searchPreviousResult()
{
    m_searchSelectPrev = true;
    updateExtraSelection();
}

void QCodeEditor::searchSetCaseSensitive(bool isCaseSensitive)
{
    m_searchIsCaseSensitive = isCaseSensitive;
    updateExtraSelection();
}

QChar QCodeEditor::charUnderCursor(int offset) const
{
    auto block = textCursor().blockNumber();
    auto index = textCursor().positionInBlock();
    auto text = document()->findBlockByNumber(block).text();

    index += offset;

    if (index < 0 || index >= text.size())
    {
        return {};
    }

    return text[index];
}

QString QCodeEditor::wordUnderCursor() const
{
    auto tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}

void QCodeEditor::insertFromMimeData(const QMimeData* source)
{
    insertPlainText(source->text());
}

int QCodeEditor::getIndentationSpaces()
{
    auto blockText = textCursor().block().text();

    int indentationLevel = 0;

    for (auto i = 0;
         i < blockText.size() && QString("\t ").contains(blockText[i]);
         ++i)
    {
        if (blockText[i] == ' ')
        {
            indentationLevel++;
        }
        else
        {
#if QT_VERSION >= 0x050A00
            indentationLevel += tabStopDistance() / fontMetrics().averageCharWidth();
#else
            indentationLevel += tabStopWidth() / fontMetrics().averageCharWidth();
#endif
        }
    }

    return indentationLevel;
}

QString QCodeEditor::getCompletionWordNow(int *linePosStart, int *linePosEnd)
{
    QString lineStartChars = " \t[{\"!'";
    QString lineEndChars = " \t)]}\"!";
    auto posStart = textCursor().positionInBlock() - 1;
    auto posEnd = posStart;
    auto line = textCursor().block().text();
    int rightPCnt = 0;
    while (posStart >= 0 && !lineStartChars.contains(line.at(posStart))) {
        if (line.at(posStart) == '(') {
            if (rightPCnt == 0) {
                break;
            } else {
                rightPCnt--;
            }
        }
        if (line.at(posStart) == ')') {
            rightPCnt++;
        }
        posStart--;
    }
    while ((posEnd + 1) < line.size() && !lineEndChars.contains(line.at(posEnd + 1))) {
        posEnd++;
    }

    QString res = "";
    if (posStart >= -1 && posEnd < line.size()) {
        res = line.mid(posStart + 1, posEnd - posStart);
    }

    if (linePosStart) {
        *linePosStart = posStart;
    }

    if (linePosEnd) {
        *linePosEnd = posEnd;
    }

    return res;
}

int QCodeEditor::searchMatches() const
{
    return m_searchMatches;
}

bool QCodeEditor::highlightBlocks() const
{
    return m_highlightBlocks;
}

void QCodeEditor::setHighlightBlocks(bool newHighlightBlocks)
{
    m_highlightBlocks = newHighlightBlocks;
}

bool QCodeEditor::getSeparateMinus() const
{
    return m_separateMinus;
}

void QCodeEditor::setSeparateMinus(bool separateMinus)
{
    m_separateMinus = separateMinus;
}

void QCodeEditor::setIndentStrs(const QString &start, const QString &end)
{
    m_indentStartStr = start;
    m_indentEndStr = end;
}

QString QCodeEditor::getCommentStr() const
{
    return m_commentStr;
}

void QCodeEditor::setCommentStr(const QString &commentStr)
{
    m_commentStr = commentStr;
}
