#pragma once

// QCodeEditor
#include <QStyleSyntaxHighlighter> // Required for inheritance
#include <QHighlightRule>

// Qt
#include <QRegularExpression>
#include <QVector>

// std
#include <functional>

class QSyntaxStyle;

/**
 * @brief Class, that describes C++ code
 * highlighter.
 */
class LispHighlighter : public QStyleSyntaxHighlighter
{
    Q_OBJECT
public:

    /**
     * @brief Constructor.
     * @param document Pointer to document.
     */
    explicit LispHighlighter(QTextDocument* document=nullptr);

    /**
     * @brief Calls apply(start, length, formatName) for every match in text.
     * Later matches override earlier ones where they overlap.
     */
    void forEachMatch(const QString& text,
                      const std::function<void(int, int, const QString&)>& apply) const;

protected:
    void highlightBlock(const QString& text) override;

private:
    QVector<QHighlightRule> m_highlightRules;
    QVector<QHighlightRule> m_highlightRulesLang;
    QRegularExpression m_keyRegex;

};

