#pragma once

// Qt
#include <QTextEdit> // Required for inheritance

class QCompleter;
class QLineNumberArea;
class QSyntaxStyle;
class QStyleSyntaxHighlighter;
class QFramedTextAttribute;

/**
 * @brief Class, that describes code editor.
 */
class QCodeEditor : public QTextEdit
{
    Q_OBJECT

public:
    /**
     * @brief Constructor.
     * @param widget Pointer to parent widget.
     */
    explicit QCodeEditor(QWidget* widget=nullptr);

    // Disable copying
    QCodeEditor(const QCodeEditor&) = delete;
    QCodeEditor& operator=(const QCodeEditor&) = delete;

    int getFirstVisibleBlock();

    void setHighlighter(QStyleSyntaxHighlighter* highlighter);
    void setSyntaxStyle(QSyntaxStyle* style);

    void setAutoParentheses(bool enabled);
    bool autoParentheses() const;

    void setTabReplace(bool enabled);
    bool tabReplace() const;
    void setTabReplaceSize(int val);
    int tabReplaceSize() const;

    void setAutoIndentation(bool enabled);
    bool autoIndentation() const;

    void setCompleter(QCompleter* completer);
    QCompleter* completer() const;

    void searchForString(QString str);
    void searchNextResult();
    void searchPreviousResult();
    void searchSetCaseSensitive(bool isCaseSensitive);

    QString getCommentStr() const;
    void setCommentStr(const QString &commentStr);

    void setIndentStrs(const QString &start, const QString &end);

    bool getSeparateMinus() const;
    void setSeparateMinus(bool separateMinus);

    bool highlightBlocks() const;
    void setHighlightBlocks(bool newHighlightBlocks);

    int searchMatches() const;

signals:
    void saveTriggered();
    void runEmbeddedTriggered();
    void runWindowTriggered();
    void stopTriggered();
    void clearConsoleTriggered();
    void searchTriggered();
    void runBlockTriggered(QString text);

public Q_SLOTS:

    /**
     * @brief Slot, that performs insertion of
     * completion info into code.
     * @param s Data.
     */
    void insertCompletion(QString s);

    /**
     * @brief Slot, that performs update of
     * internal editor viewport based on line
     * number area width.
     */
    void updateLineNumberAreaWidth(int);

    /**
     * @brief Slot, that performs update of some
     * part of line number area.
     * @param rect Area that has to be updated.
     */
    void updateLineNumberArea(const QRect& rect);

    /**
     * @brief Slot, that will proceed extra selection
     * for current cursor position.
     */
    void updateExtraSelection();

    /**
     * @brief Slot, that will update editor style.
     */
    void updateStyle();

protected:
    /**
     * @brief Method, that's called on any text insertion of
     * mimedata into editor. If it's text - it inserts text
     * as plain text.
     */
    void insertFromMimeData(const QMimeData* source) override;

    /**
     * @brief Method, that's called on editor painting. This
     * method if overloaded for line number area redraw.
     */
    void paintEvent(QPaintEvent* e) override;

    /**
     * @brief Method, that's called on any widget resize.
     * This method if overloaded for line number area
     * resizing.
     */
    void resizeEvent(QResizeEvent* e) override;

    /**
     * @brief Method, that's called on any key press, posted
     * into code editor widget. This method is overloaded for:
     *
     * 1. Completion
     * 2. Tab to spaces
     * 3. Low indentation
     * 4. Auto parenthesis
     */
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;

    /**
     * @brief Method, that's called on focus into widget.
     * It's required for setting this widget to set
     * completer.
     */
    void focusInEvent(QFocusEvent *e) override;

private:

    /**
     * @brief Method for initializing default
     * monospace font.
     */
    void initFont();

    /**
     * @brief Method for performing connection
     * of objects.
     */
    void performConnections();

    /**
     * @brief Method for updating geometry of line number area.
     */
    void updateLineGeometry();

    /**
     * @brief Method, that performs completer processing.
     * Returns true if event has to be dropped.
     * @param e Pointer to key event.
     * @return Shall event be dropped.
     */
    bool proceedCompleterBegin(QKeyEvent *e);
    void proceedCompleterEnd(QKeyEvent* e);

    /**
     * @brief Method for getting character under
     * cursor.
     * @param offset Offset to cursor.
     */
    QChar charUnderCursor(int offset = 0) const;

    /**
     * @brief Method for getting word under
     * cursor.
     * @return Word under cursor.
     */
    QString wordUnderCursor() const;

    /**
     * @brief Method, that adds highlighting of
     * currently selected line to extra selection list.
     */
    void highlightCurrentLine(QList<QTextEdit::ExtraSelection>& extraSelection);

    /**
     * @brief Method, that adds highlighting of
     * parenthesis if available.
     */
    void highlightParenthesis(
            QList<QTextEdit::ExtraSelection>& extraSelection,
            bool selectBlock);

    void highlightSearch(QList<QTextEdit::ExtraSelection>& extraSelection);

    /**
     * @brief Method for getting number of indentation
     * spaces in current line. Tabs will be treated
     * as `tabWidth / spaceWidth`
     */
    int getIndentationSpaces();

    QString getCompletionWordNow(int *linePosStart, int *linePosEnd);

    QStyleSyntaxHighlighter* m_highlighter;
    QSyntaxStyle* m_syntaxStyle;
    QLineNumberArea* m_lineNumberArea;
    QCompleter* m_completer;
    QString m_searchStrNow;
    int m_searchMatches;
    bool m_searchSelectNext;
    bool m_searchSelectPrev;
    bool m_searchIsCaseSensitive;

    bool m_autoIndentation;
    bool m_autoParentheses;
    bool m_replaceTab;
    QString m_tabReplace;
    QString m_commentStr;
    QString m_indentStartStr;
    QString m_indentEndStr;
    bool m_separateMinus;
    bool m_highlightBlocks;
    QString m_highlightedBlock;

};

