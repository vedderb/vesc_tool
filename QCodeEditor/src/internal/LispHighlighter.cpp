// QCodeEditor
#include <LispHighlighter>
#include <QSyntaxStyle>
#include <QLanguage>

// Qt
#include <QFile>


LispHighlighter::LispHighlighter(QTextDocument* document) :
    QStyleSyntaxHighlighter(document),
    m_highlightRules(),
    m_highlightRulesLang(),
    m_keyRegex(R"(("[^\r\n:]+?")\s*:)")
{
    Q_INIT_RESOURCE(qcodeeditor_resources);
    QFile fl(":/languages/lisp.xml");

    if (!fl.open(QIODevice::ReadOnly))
    {
        return;
    }

    QLanguage language(&fl);

    if (!language.isLoaded())
    {
        return;
    }

    auto keys = language.keys();
    for (auto&& key : keys)
    {
        auto names = language.names(key);
        for (auto&& name : names)
        {
            m_highlightRulesLang.append({
                QRegularExpression(QString(R"(\b%1\b)").arg(name.replace("-", "_").replace("?", "_"))),
                key
            });
        }
    }

    // Numbers
    m_highlightRules.append({
        QRegularExpression(R"((?<=\b|\s|^)(?i)(?:(?:(?:(?:(?:\d+(?:'\d+)*)?\.(?:\d+(?:'\d+)*)(?:e[+-]?(?:\d+(?:'\d+)*))?)|(?:(?:\d+(?:'\d+)*)\.(?:e[+-]?(?:\d+(?:'\d+)*))?)|(?:(?:\d+(?:'\d+)*)(?:e[+-]?(?:\d+(?:'\d+)*)))|(?:0x(?:[0-9a-f]+(?:'[0-9a-f]+)*)?\.(?:[0-9a-f]+(?:'[0-9a-f]+)*)(?:p[+-]?(?:\d+(?:'\d+)*)))|(?:0x(?:[0-9a-f]+(?:'[0-9a-f]+)*)\.?(?:p[+-]?(?:\d+(?:'\d+)*))))[lf]?)|(?:(?:(?:[1-9]\d*(?:'\d+)*)|(?:0[0-7]*(?:'[0-7]+)*)|(?:0x[0-9a-f]+(?:'[0-9a-f]+)*)|(?:0b[01]+(?:'[01]+)*))(?:u?l{0,2}|l{0,2}u?)))(?=\b|\s|$))"),
        "Number"
    });

    // Strings
    m_highlightRules.append({
        QRegularExpression(R"("[^\n"]*")"),
        "String"
    });

    // Single line
    m_highlightRules.append({
        QRegularExpression(R"(;[^\n]*)"),
        "Comment"
    });
}

void LispHighlighter::highlightBlock(const QString& text)
{
    for (auto&& rule : m_highlightRulesLang) {
        auto matchIterator = rule.pattern.globalMatch(QString(text).replace("-", "_").replace("#", "_").replace("?", "_"));

        while (matchIterator.hasNext()) {
            auto match = matchIterator.next();

            setFormat(
                match.capturedStart(),
                match.capturedLength(),
                syntaxStyle()->getFormat(rule.formatName)
            );
        }
    }

    for (auto&& rule : m_highlightRules) {
        auto matchIterator = rule.pattern.globalMatch(text);

        while (matchIterator.hasNext()) {
            auto match = matchIterator.next();

            setFormat(
                match.capturedStart(),
                match.capturedLength(),
                syntaxStyle()->getFormat(rule.formatName)
            );
        }
    }

    // Special treatment for key regex
    auto matchIterator = m_keyRegex.globalMatch(text);

    while (matchIterator.hasNext()) {
        auto match = matchIterator.next();

        setFormat(
            match.capturedStart(1),
            match.capturedLength(1),
            syntaxStyle()->getFormat("Keyword")
        );
    }
}
