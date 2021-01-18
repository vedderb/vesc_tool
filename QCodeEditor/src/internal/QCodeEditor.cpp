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

static QVector<QPair<QString, QString>> parentheses = {
    {"(", ")"},
    {"{", "}"},
    {"[", "]"},
    {"\"", "\""},
    {"'", "'"}
};

QCodeEditor::QCodeEditor(QWidget* widget) :
    QTextEdit(widget),
    m_highlighter(nullptr),
    m_syntaxStyle(nullptr),
    m_lineNumberArea(new QLineNumberArea(this)),
    m_completer(nullptr),
    m_autoIndentation(true),
    m_autoParentheses(true),
    m_replaceTab(true),
    m_tabReplace(QString(4, ' '))
{
    initFont();
    performConnections();

    setSyntaxStyle(QSyntaxStyle::defaultStyle());
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
    if (m_highlighter)
    {
        m_highlighter->setDocument(nullptr);
    }

    m_highlighter = highlighter;

    if (m_highlighter)
    {
        m_highlighter->setSyntaxStyle(m_syntaxStyle);
        m_highlighter->setDocument(document());
    }
}

void QCodeEditor::setSyntaxStyle(QSyntaxStyle* style)
{
    m_syntaxStyle = style;

    m_lineNumberArea->setSyntaxStyle(m_syntaxStyle);

    if (m_highlighter)
    {
        m_highlighter->setSyntaxStyle(m_syntaxStyle);
    }

    updateStyle();
}

void QCodeEditor::updateStyle()
{
    if (m_highlighter)
    {
        m_highlighter->rehighlight();
    }

    if (m_syntaxStyle)
    {
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

    highlightCurrentLine(extra);
    highlightParenthesis(extra);

    setExtraSelections(extra);
}

void QCodeEditor::highlightParenthesis(QList<QTextEdit::ExtraSelection>& extraSelection)
{
    auto currentSymbol = charUnderCursor();
    auto prevSymbol = charUnderCursor(-1);

    for (auto& pair : parentheses)
    {
        int direction;

        QChar counterSymbol;
        QChar activeSymbol;
        auto position = textCursor().position();

        if (pair.first == currentSymbol)
        {
            direction = 1;
            counterSymbol = pair.second[0];
            activeSymbol = currentSymbol;
        }
        else if (pair.second == prevSymbol)
        {
            direction = -1;
            counterSymbol = pair.first[0];
            activeSymbol = prevSymbol;
            position--;
        }
        else
        {
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
            if (character == activeSymbol)
            {
                ++counter;
            }
            else if (character == counterSymbol)
            {
                --counter;
            }
        }

        auto format = m_syntaxStyle->getFormat("Parentheses");

        // Found
        if (counter == 0)
        {
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

            extraSelection.append(selection);

            selection.cursor = textCursor();
            selection.cursor.clearSelection();
            selection.cursor.movePosition(
                directionEnum,
                QTextCursor::MoveMode::KeepAnchor,
                1
            );

            extraSelection.append(selection);
        }

        break;
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
    auto isSpace = charUnderCursor(-1) == " " || charUnderCursor(-1) == "\t";

    if (isSpace || (word.isEmpty() && !isDot)) {
        m_completer->popup()->hide();
        return;
    }

    static QString eow(R"(~!@#$%^&*()_+{}|:"<>?,/;'[]\-=)");

    tc.select(QTextCursor::LineUnderCursor);
    QString completionPrefix = "";
    auto splitted = tc.selectedText().split(" ");
    if (!splitted.isEmpty()) {
        completionPrefix = splitted.last();
    }

    if (!completionPrefix.endsWith(word)) {
        completionPrefix = word;
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

    auto completerSkip = proceedCompleterBegin(e);

    if (!completerSkip) {
        auto tcStart = textCursor();
        auto tcEnd = textCursor();
        tcStart.setPosition(textCursor().selectionStart());
        tcEnd.setPosition(textCursor().selectionEnd());

        int lineStart = tcStart.blockNumber();
        int lineEnd = tcEnd.blockNumber();

        // Toggle block comment
        if (e->text() == "\u001F") {
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
                        if (line2.startsWith("//")) {
                            line.replace(line.indexOf("/"), 2, "");
                        } else {
                            line.prepend("//");
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
                return;
            } else if (e->key() == Qt::Key_E) {
                emit runEmbeddedTriggered();
                return;
            } else if (e->key() == Qt::Key_W) {
                emit runWindowTriggered();
                return;
            } else if (e->key() == Qt::Key_Q) {
                emit stopTriggered();
                return;
            } else if (e->key() == Qt::Key_D) {
                emit clearConsoleTriggered();
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

        // Auto-indent selected line or block
        if (e->key() == Qt::Key_I && e->modifiers() == Qt::ControlModifier) {
            auto txtOld = toPlainText();
            int indentNow = 0;
            bool isComment = false;

            int lineNum = -1;
            for (auto line: txtOld.split("\n")) {
                lineNum++;

                bool indent = true;
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

                for (auto c: line) {
                    if (c == '}') {
                        indentNow--;
                    }

                    if (indentNow < 0) {
                        indentNow = 0;
                    }
                }

                if (indent) {
                    for (int i = 0;i < indentNow;i++) {
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

                for (auto c: line) {
                    if (c == '{') {
                        indentNow++;
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

        // Have Qt Edior like behaviour, if {|} and enter is pressed indent the two
        // parenthesis
        auto charAfter = charUnderCursor();

        if (m_autoIndentation &&
                (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) &&
                charUnderCursor(-1) == '{')
        {
            int charsBack = 0;
            insertPlainText("\n");

            if (m_replaceTab)
                insertPlainText(QString(indentationLevel + defaultIndent, ' '));
            else
                insertPlainText(QString(tabCounts + 1, '\t'));

            if (charAfter == '}') {
                insertPlainText("\n");
                charsBack++;

                if (m_replaceTab)
                {
                    insertPlainText(QString(indentationLevel, ' '));
                    charsBack += indentationLevel;
                }
                else
                {
                    insertPlainText(QString(tabCounts, '\t'));
                    charsBack += tabCounts;
                }
            }

            while (charsBack--)
                moveCursor(QTextCursor::MoveOperation::Left);
            return;
        }

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

                // If it's close brace - check parentheses
                if (el.second == e->text())
                {
                    auto symbol = charUnderCursor();

                    if (symbol == el.second)
                    {
                        textCursor().deletePreviousChar();
                        moveCursor(QTextCursor::MoveOperation::Right);
                    }

                    break;
                }
            }
        }
    }

    proceedCompleterEnd(e);
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
    if (m_completer->widget() != this)
    {
        return;
    }

    auto tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    auto word = tc.selectedText();

    tc.select(QTextCursor::LineUnderCursor);
    auto line = tc.selectedText();

    QString completionPrefix = "";
    auto splitted = line.split(" ");
    if (!splitted.isEmpty()) {
        completionPrefix = splitted.last();
    }

    if (completionPrefix.endsWith(word) && completionPrefix != word) {
        line.replace(line.lastIndexOf(completionPrefix), completionPrefix.size(), s);
        tc.select(QTextCursor::LineUnderCursor);
        tc.insertText(line);
    } else {
        tc = textCursor();
        tc.select(QTextCursor::WordUnderCursor);
        tc.insertText(s);
    }

    setTextCursor(tc);
}

QCompleter *QCodeEditor::completer() const
{
    return m_completer;
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
